#include <functional>
#include <vector>
#include <utility>
#include <algorithm>
#include <list>
#include <shared_mutex>
#include <iterator>

constexpr unsigned DEFAULT_NUM_BUCKETS = 19;

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

    public:
        Value get_value(const Key& key, const Value& default_value) const {
            // Use shared lock to allow multiple readers
            std::shared_lock<std::shared_timed_mutex> lock(mutex);
            const_iterator found_entry = std::find_if(std::begin(data), std::end(data), [&](const bucket_value& item) {
                return (key == item.first);
            });
            return (found_entry==data.cend()) ? default_value : found_entry->second;
        }

        void add_or_update(const Key& key, const Value& value) {
            // Use unique lock for exclusive writing
            std::unique_lock<std::shared_timed_mutex> lock(mutex);
            auto found_entry = std::find_if(std::begin(data), std::end(data), [&](const bucket_value& item) {
                return (key == item.first);
            });
            if(found_entry == data.end()) 
                // Adding
                data.push_back(bucket_value(key, value));
            else
                // Updating
                found_entry->second = value;
        }

        void remove(const Key& key) {
            // Use unique lock for exclusive writing
            std::unique_lock<std::shared_timed_mutex> lock(mutex);
            auto found_entry = std::find_if(std::begin(data), std::end(data), [&](const bucket_value& item) {
                return (key == item.first);
            });
            if(found_entry != data.end()) 
                data.erase(found_entry);
        }
    };

    std::vector<std::unique_ptr<bucket>> _buckets;
    Hash _hasher;

    bucket& get_bucket(const Key& key) const {
        // No locking necessary since the size of buckets is fixed
        std::size_t const bucket_idx = _hasher(key) % _buckets.size();
        return *_buckets[bucket_idx];
    }

public:
    using key_type = Key;
    using mapped_type = Value;
    using hash_type = Hash;

    threadsafe_hashmap(unsigned table_size = DEFAULT_NUM_BUCKETS, const Hash& hasher = Hash())
    : _buckets(table_size), _hasher(hasher) {
        for(unsigned i=0; i<table_size; ++i) 
            _buckets[i].reset(new bucket);
    }

    // Disallow copy ctor and assignment operator for simplicity
    threadsafe_hashmap(const threadsafe_hashmap& other) = delete;
    threadsafe_hashmap& operator=(const threadsafe_hashmap& rhs) = delete;

    Value get_value(const Key&key, const Value& default_val = Value()) const {
        return get_bucket(key).get_value(key, default_val);
    }

    void add_or_update(const Key& key, const Value& val) {
        get_bucket(key).add_or_update(key, val);
    }

    void remove(const Key& key) {
        get_bucket(key).remove(key);
    }
};