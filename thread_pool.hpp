#include "threadsafe_queue.hpp"
#include <functional>
#include <future>

class thread_pool 
{
    threadsafe_queue<std::move_only_function<void()> > work_q;
    std::vector<std::future<void> > futures;

    void worker_thread() 
    {
        do{
            std::move_only_function<void()> task;
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
            // wait for the thread pool to be done
            for(std::future<void>& fut: futures) {
                fut.get();
            }
            throw;
        }
    }

    ~thread_pool() {
        // wait for the thread pool to be done
        for(std::future<void>& fut: futures) {
            fut.get();
        }
    }

    /*std::vector<std::future<void> >&& get_futures() {
        return std::move(futures);
    }*/

    template<typename FuncType>
    auto submit(FuncType f) {
        using result_of_f = std::result_of<FuncType()>::type;

        std::packaged_task<result_of_f()> task(std::move(f));
        std::future<result_of_f> fut = task.get_future();

        work_q.push(std::move(task));

        return fut;
    }
};