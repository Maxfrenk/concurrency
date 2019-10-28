#include <functional>
#include <utility>
#include <algorithm>
#include <list>
#include <shared_mutex>

template<typename Key, typename Value, typename Hash=std::hash<Key> >
class threadsafe_hashmap {
private: 
    class bucket {
    private:
        using bucket_value = std::pair<Key, Value>;
        using bucket_data = std::list<bucket_value>;
        using iterator = typename bucket_data::iterator;
        using const_iterator = typename bucket_data::const_iterator;

        bucket_data data;
        mutable std::shared_timed_mutex mutex;

        iterator find(const Key& key) const {
            return std::find_if(data.begin(), data.end(), [&](const bucket_value& item) {
                return key == item.first;
            });
        }

    public:
        Value get_value(const Key& key, const Value& default_value) const {
            // Use shared lock to allow multiple readers
            std::shared_lock<std::shared_timed_mutex> lock(mutex);
            const_iterator found_entry = find(key);
            return (found_entry==data.cend()) ? default_value : found_entry->second;
        }

        void add_or_update(const Key& key, const Value& value) {
            // Use unique lock for exclusive writing
            std::unique_lock<std::shared_timed_mutex> lock(mutex);
            const iterator found_entry = find(key);
            if(found_entry == data.end()) 
                // Adding
                data.push_back(bucket_value(key, value));
            else
                // Updating
                found_entry->second = value;
        }


    };
};