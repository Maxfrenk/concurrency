#include <functional>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <list>
#include <algorithm>
#include <vector>

template<class key, class value, class hash_fct = std::hash<key> >
class lockfree_multimap
{
public:
    using map_type = std::unordered_multimap<key, value, hash_fct >;
    using garbage_collector = std::list<map_type *>;

    class HPList {
    public:
        // Hazard pointer record
        class HPRecType {
            HPRecType * pNext_;
            std::atomic_int active_;
            
        public:
            // Can be used by the thread
            // that acquired it
            void * pHazard_;

            HPRecType * Next() {
                return pNext_;
            }

            void Next(HPRecType *pNext) {
                pNext_ = pNext;
            }

            std::atomic_int& Active() {
                return active_;
            }
        };

        // Global header of the HP list
        std::atomic<HPRecType *> pHead_;
        // The length of the list
        std::atomic_int listLen_;

        HPList() : pHead_(nullptr), listLen_(0) {
        }

        ~HPList() {
            // Free the list
			HPRecType* old_head;
			while(old_head = pHead_.load()) {
				HPRecType* next = old_head->Next();
				if (pHead_.compare_exchange_weak(old_head, next))
					// If the head pointer advances to next, can free the old head ptr
					delete old_head;
			};
		}

        HPRecType * Head() {
            return pHead_.load();
        }

        int ListLen() {
            return listLen_.load();
        }

        // Acquires one hazard pointer
        HPRecType * AcquireHP() {
            // Try to reuse a retired HP record
            HPRecType *p = pHead_.load();
            int expected=0;
            for (; p; p = p->Next()) {
                if (p->Active().load() || !p->Active().compare_exchange_weak(expected, 1))
                    continue;

                // Got one!
                return p;
            }

            // Increment the list length
            listLen_++;

            // Allocate a new one
            p = new HPRecType;
            HPRecType * old;
            p->Active().store(1);
            p->pHazard_ = nullptr;
            // Push it to the front
            do {
                old = pHead_.load();
                p->Next(old);
            } while (!pHead_.compare_exchange_weak(old, p));

            return p;
        }

        // Releases a hazard pointer
        static void ReleaseHP(HPRecType* p) {
            p->pHazard_ = nullptr;
            p->Active().store(0);
        }
    };

    lockfree_multimap()
        : s_map_(nullptr), hp_list_()
    {
    }

    virtual ~lockfree_multimap()
    {
        map_type *map_ptr = s_map_.load();
        delete map_ptr;
    }

    void update(const key& k, const value& v, garbage_collector& gc)
    {
        map_type *p_new = nullptr;
        map_type *p_old;
        do {
            p_old = s_map_.load();
            delete p_new;

            // Make a copy of the map
            if(p_old)
                p_new = new map_type(*p_old);
            else
                p_new = new map_type();

            // Update the copy
            p_new->insert(typename map_type::value_type(k, v));
        }
        // CAS the new copy with the class's map
        while (!s_map_.compare_exchange_weak(p_old, p_new));

        retire(gc, p_old);
    }

    size_t erase(const key& k, garbage_collector& gc)
    {
        map_type *p_new = nullptr;
        map_type *p_old;
        size_t result;
        do {
            p_old = s_map_.load();
            delete p_new;

            // Make a copy of the map
            if(p_old)
                p_new = new map_type(*p_old);
            else
                // nothing to delete
                return 0;

            // Update the copy
            result = p_new->erase(k);
        }
        // CAS the new copy with the class's map
        while (!s_map_.compare_exchange_weak(p_old, p_new));

        retire(gc, p_old);

        return result;
    }

    value lookup(const key& k, const value& not_found) const
    {
        typename HPList::HPRecType * pRec = hp_list_.AcquireHP();

        map_type * ptr;
        do {
            ptr = s_map_.load();
            pRec->pHazard_ = ptr;
        }
        while (s_map_ != ptr);

        // Save Willy
        value result=not_found;
        if(ptr) {
            typename map_type::const_iterator found = ptr->find(k);

            if(found != ptr->end())
                result = found->second;
        }

        // pRec can be released now
        // because it's not used anymore
        hp_list_.ReleaseHP(pRec);

        return result;
    }

private:

    void retire(garbage_collector& gc, map_type * pOld)
    {
        // put it in the retired list
        if(pOld)
            gc.push_back(pOld);

        // clean up the gc from time to time
        if (gc.size() >= 1.25 * hp_list_.ListLen()) 
            scan(gc);
    }

    void scan(garbage_collector& gc)
    {
        // Stage 1: Scan hazard pointers list
        // collecting all non-null ptrs
        std::vector<void*> hp;

        typename HPList::HPRecType * head = hp_list_.Head();
        while (head) {
            void * p = head->pHazard_;
            if (p) hp.push_back(p);
            head = head->Next();
        }

        // Stage 2: sort the non-null hazard pointers
        std::sort(hp.begin(), hp.end(), std::less<void*>());

        // Stage 3: Go through gc, looking for those non-null hazard pointers in hp
        typename garbage_collector::iterator i = gc.begin();
        while (i != gc.end()) {
            if ( !std::binary_search(hp.begin(), hp.end(), *i) ) {
                // Aha!
                delete *i;

				typename garbage_collector::iterator itmp = i;
				itmp++;
                gc.erase(i);
				i = itmp;
            }
            else
                ++i;
        }
    }

    std::atomic<map_type *> s_map_;  // pointer to the map

    mutable HPList hp_list_;
};
