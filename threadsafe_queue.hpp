#include <queue>
#include <memory>
#include <condition_variable>

template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T> > data_q;
    std::condition_variable cond_var;

public:
    threadsafe_queue() {}

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mut);
        cond_var.wait(lk, [this] {
            return !data_q.empty();
        });
        value = std::move(*data_q.front());
        data_q.pop();
    }

    bool try_pop(T& value) {
        if(data_q.empty())
            return false;
        std::lock_guard<std::mutex> lk(mut);
        value = std::move(*data_q.front());
        data_q.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        cond_var.wait(lk, [this] {
            return !data_q.empty();
        });
        std::shared_ptr<T> res = data_q.front();
        data_q.pop();
        return res;
    }

    std::shared_ptr<T> try_pop() {
        if(data_q.empty())
            return std::shared_ptr<T>();
        std::lock_guard<std::mutex> lk(mut);
        std::shared_ptr<T> res = data_q.front();
        data_q.pop();
        return res;
    }

    void push(T&& new_value) {
        std::shared_ptr<T> data(
            std::make_shared<T>(std::move(new_value))
        );
        std::lock_guard<std::mutex> lk(mut);
        data_q.push(data);
        cond_var.notify_one();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_q.empty();
    }

    int size() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_q.size();
    }
};