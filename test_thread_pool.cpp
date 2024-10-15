#include "thread_pool.hpp"
#include <mutex>
#include <iostream>
#include <numeric>

int main() 
{
    for(unsigned threads=1; threads<7; ++threads ) {
        std::cout << "Using " << threads << " threads:" << '\n';
        auto start = std::chrono::high_resolution_clock::now();

        thread_pool pool(threads-1);
        std::mutex m;
        // vector shared among threads
        constexpr unsigned vec_len = 10'000'000;
        std::vector<int> tens(vec_len, 10);
        unsigned segment_len = vec_len / threads;
        std::cout << "segment_len: " << segment_len << std::endl;
        auto begin_itr = tens.begin();
        int tot_sum = 0;
        for(int i=0; i<threads-1; ++i) {
            pool.submit([&tens, &tot_sum, &begin_itr, segment_len, &m]{
                std::lock_guard<std::mutex> lk(m);
                int sum = std::accumulate(begin_itr, begin_itr + segment_len, 0);
                tot_sum += sum;
                std::advance(begin_itr, segment_len);
            });
        }
        // wait for the thread pool to be done
        auto futures = pool.get_futures();
        for(std::future<void>& fut: futures) {
            fut.get();
        }

        // do the last segment on the main thread
        tot_sum += std::accumulate(begin_itr, tens.end(), 0);
        std::cout << "tot_sum: " << tot_sum << std::endl;

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end-start;
        std::cout << elapsed.count() << " ms passed\n";
    }

    return 0;
}