#include <iostream>
#include <vector>
#include <chrono>
#include <future>

// #include "other_thread_pool.h"
#include "thread_pool.h"

using namespace std::chrono_literals;

int main() {
    ThreadPool pool;
    for(int i = 0; i < 8; ++i) {
        pool.push([i] {
            std::cout << i << '\n';
            std::this_thread::sleep_for(1s);
            std::cout << i << '\n';
        });
    }
}