#include "threadsafe_hashmap.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <string>
#include <cassert>
#include <random>

using std::cout;
using std::endl;
using namespace std::chrono_literals;

threadsafe_hashmap<std::string, int> hashmap(1009);

// Perform writes
void write_from_to(int from, int to) {
    int i=from;
    for(; i<to; ++i) {
        std::string key = "key_" + std::to_string(i);
        hashmap.add_or_update(key, i);
        //std::this_thread::sleep_for(0.2s);
    }
    //return i;
}

/// @brief Set to true while the reading thread is running, signalling to the writing 
/// thread that not all values have yet been tested, and hence to keep writing
std::atomic_bool keep_writing(true);

void write_random(int from, int to) {
    int i=0;
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(from, to);
    do {
        int rnd_int = distrib(gen);  // int in [from, to]
        std::string key = "key_" + std::to_string(rnd_int);
        hashmap.add_or_update(key, rnd_int);
        ++i;
    } while (keep_writing);
}

int main() {
    /////////////// Basic functionality /////////////////
    assert(hashmap.get_size() == 0);
    hashmap.add_or_update("1", 1);
    hashmap.add_or_update("2", 2);
    assert(hashmap.get_value("1") == 1);
    assert(hashmap.get_value("2") == 2);
    assert(hashmap.get_size() == 2);
    hashmap.remove("1");
    hashmap.remove("2");
    assert(hashmap.get_size() == 0);

    cout << "Hashmap created, spawning threads..." << endl;
    cout << "Hardware threads available: " << std::thread::hardware_concurrency << endl;

    /////////////// Concurrent reads/writes - sequential /////////////////
    std::cout << "Sequential reads/writes test" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    auto fut_wr_1 = std::async(std::launch::async, write_from_to, 0, 1000);
    //auto fut_rd_1 = std::async(std::launch::async, read_from_to, 0, 500);
    for(int i=0; i<1000; ++i) {
        std::string key = "key_" + std::to_string(i);
        //std::this_thread::sleep_for(0.01s);
        int val;
        do {
            val =hashmap.get_value(key, -1);
        } while(val == -1);
        assert(val == i);
        //cout << key << ": " << val << endl;
    }
    fut_wr_1.get();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end-start;
    std::cout << elapsed.count() << " ms passed\n";
    
    /////////////// Concurrent reads/writes - random /////////////////
    std::cout << "Random reads/writes test" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    // write random keys into hashmap until all 1000 of them are read successfully
    std::thread thrd([] {
        write_random(0, 999); 
    });
    for(int i=0; i<=999; ++i) {
        std::string key = "key_" + std::to_string(i);
        int val;
        do {
            val = hashmap.get_value(key, -1);
        } while(val == -1);
        assert(val == i);
        //std::cout << val << std::endl;
    }
    // if got here, all 1000 values were read successfully, can stop thrd
    keep_writing = false;
    thrd.join();
    end = std::chrono::high_resolution_clock::now();
    elapsed = end-start;
    std::cout << elapsed.count() << " ms passed\n";
    
    return 0;
}