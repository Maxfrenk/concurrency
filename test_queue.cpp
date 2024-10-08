#include "threadsafe_queue.hpp"

#include <cassert>
#include <string>
#include <future>
#include <iostream>

int main() {

    threadsafe_queue<std::string> q;

    assert(q.empty() == true);
    std::string str1;
    assert(q.try_pop(str1) == false);
    str1 = "str1";
    q.push(std::move(str1));
    assert(q.empty() == false);
    assert(q.size() == 1);
    std::string str2;
    assert(q.try_pop(str2) == true);
    assert(str2 == "str1");
    assert(q.size() == 0);

    /////////////// Concurrent reads/writes - bool try_pop(T& value) test /////////////////
    std::cout << "bool try_pop(T& value) test" << std::endl;
    // launch push thread on q
    auto start = std::chrono::high_resolution_clock::now();
    auto fut_wr_1 = std::async(std::launch::async, [&q]{
        for(int i=0; i<1000; ++i) {
            // push "str0" .. "str1000" in a sequence
            std::string str = "str" + std::to_string(i);
            q.push(std::move(str));
        }
    });
    // pop in main thread
    int i=0;
    std::string str_golden;
    while(true) {
        if(!q.try_pop(str1))
            continue;   // q is empty, try again
        str_golden = "str" + std::to_string(i);
        // "str0" .. "str1000" should get popped while q isn't empty
        assert(str1 == str_golden);
        ++i;
        if(i==1000)
            break;
    }
    fut_wr_1.get();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end-start;
    std::cout << elapsed.count() << " ms passed\n";
    ///////////////////////////////////////////////////////////////////////////////////////

    assert(q.size() == 0);

    /////////////// Concurrent reads/writes - std::shared_ptr<T> try_pop() test /////////////////
    std::cout << "std::shared_ptr<T> try_pop() test" << std::endl;
    // launch push thread on q
    start = std::chrono::high_resolution_clock::now();
    fut_wr_1 = std::async(std::launch::async, [&q]{
        for(int i=0; i<1000; ++i) {
            // push "str0" .. "str1000" in a sequence
            std::string str = "str" + std::to_string(i);
            q.push(std::move(str));
        }
    });
    // pop in main thread
    i=0;
    std::shared_ptr<std::string> ptr_str;
    while(true) {
        ptr_str = q.try_pop();
        if(!ptr_str)
            continue;   // keep trying until successful
        str_golden = "str" + std::to_string(i);
        // "str0" .. "str1000" should get popped while q isn't empty
        assert(*ptr_str == str_golden);
        ++i;
        if(i==1000)
            break;
    }
    fut_wr_1.get();
    end = std::chrono::high_resolution_clock::now();
    elapsed = end-start;
    std::cout << elapsed.count() << " ms passed\n";
    ///////////////////////////////////////////////////////////////////////////////////////

    assert(q.size() == 0);

    /////////////// Concurrent reads/writes - void wait_and_pop(T& value) test /////////////////
    std::cout << "void wait_and_pop(T& value) test" << std::endl;
    // launch wait_and_pop thread on q first to begin waiting
    start = std::chrono::high_resolution_clock::now();
    fut_wr_1 = std::async(std::launch::async, [&q]{
        std::string str;
        for(int i=0; i<1000; ++i) {
            q.wait_and_pop(str);
            std::string str_golden = "str" + std::to_string(i);
            // "str0" .. "str999" should get popped while q isn't empty
            assert(str == str_golden);
        }
    });
    // push in main thread
    for(int i=0; i<1000; ++i) {
        // push "str0" .. "str999" in a sequence
        std::string str = "str" + std::to_string(i);
        q.push(std::move(str));
    }
    fut_wr_1.get();
    end = std::chrono::high_resolution_clock::now();
    elapsed = end-start;
    std::cout << elapsed.count() << " ms passed\n";
    ///////////////////////////////////////////////////////////////////////////////////////

    assert(q.size() == 0);

    /////////////// Concurrent reads/writes - std::shared_ptr<T> wait_and_pop() test /////////////////
    std::cout << "std::shared_ptr<T> wait_and_pop() test" << std::endl;
    // launch wait_and_pop thread on q first to begin waiting
    start = std::chrono::high_resolution_clock::now();
    fut_wr_1 = std::async(std::launch::async, [&q]{
        std::string str;
        std::shared_ptr<std::string> ptr_str;
        std::string str_golden;
        for(int i=0; i<1000; ++i) {
            ptr_str = q.wait_and_pop();
            str_golden = "str" + std::to_string(i);
            // "str0" .. "str999" should get popped while q isn't empty
            assert(*ptr_str == str_golden);
        }
    });
    // push in main thread
    for(int i=0; i<1000; ++i) {
        // push "str0" .. "str999" in a sequence
        std::string str = "str" + std::to_string(i);
        q.push(std::move(str));
    }
    fut_wr_1.get();
    end = std::chrono::high_resolution_clock::now();
    elapsed = end-start;
    std::cout << elapsed.count() << " ms passed\n";
    ///////////////////////////////////////////////////////////////////////////////////////

    return 0;
}