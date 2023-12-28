#include "threadsafe_hashmap.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <string>
#include <cassert>

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

void read_from_to(int from, int to) {
    int i=from;
    for(; i<to; ++i) {
        std::string key = "key_" + std::to_string(i);
        auto val = hashmap.get_value(key, -1);
        //cout << key << ": " << val << endl;
        //std::this_thread::sleep_for(0.1s);
    }
    //return 2*i;
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

    /////////////// Concurrent reads/writes /////////////////
    cout << "Hashmap created, spawning threads..." << endl;
    cout << "Hardware threads available: " << std::thread::hardware_concurrency << endl;
    auto start = std::chrono::high_resolution_clock::now();
    auto fut_wr_1 = std::async(std::launch::async, write_from_to, 0, 1000);
    //auto fut_rd_1 = std::async(std::launch::async, read_from_to, 0, 500);
    //auto fut_rd_2 = std::async(read_from_to, 334, 666);
    //auto fut_rd_3 = std::async(read_from_to, 501, 750);
    //auto fut_rd_4 = std::async(read_from_to, 601, 800);
    ////////////////
    // Perform more reads than writes
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
    //fut_rd_1.get();
    //cout << 200 << " reads performed." << endl;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end-start;
    std::cout << elapsed.count() << " ms passed\n";
    
    return 0;
}