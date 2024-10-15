#include "threadsafe_queue.hpp"
#include <functional>
#include <future>

class thread_pool 
{
    threadsafe_queue<std::function<void()> > work_q;
    std::vector<std::future<void> > futures;

    void worker_thread() 
    {
        do{
            std::function<void()> task;
            work_q.wait_and_pop(task);
            task();
        } while(!work_q.empty());
    }

public:
    thread_pool(unsigned available_threads): 
        work_q(), futures()
    {
        futures.reserve(available_threads);
        try {
            for(unsigned i=0; i<available_threads; ++i) {
                futures.push_back(
                    std::async(std::launch::async, &thread_pool::worker_thread, this));
            }
        }
        catch(...) {
            throw;
        }
    }

    ~thread_pool() {
    }

    std::vector<std::future<void> >&& get_futures() {
        return std::move(futures);
    }

    template<typename FuncType>
    void submit(FuncType f) {
        std::function<void()> f_(f);
        work_q.push(std::move(f_));
    }
};