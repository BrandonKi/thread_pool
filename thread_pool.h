#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <concepts>

class ThreadPool {
  public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency()): pool{}, task_queue{}, queue_mutex{}, manager{}, stop{false} {

        auto worker =   [this] {
                            for(;;) {
                                std::function<void()> task;
                                {
                                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                                    this->manager.wait(lock, [this]{ return this->stop || !this->task_queue.empty(); });
                                    if(this->stop && this->task_queue.empty())
                                    return;
                                    task = std::move(this->task_queue.front());
                                    this->task_queue.pop();
                                }
                                task();
                            }
                        };

        for(size_t i = 0; i < num_threads; ++i)
            pool.emplace_back(worker);
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        manager.notify_all();
        for(auto& thread: pool)
            thread.join();
    }

    template <std::invocable T>
    void push(T&& task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            task_queue.emplace(task);
        }
        manager.notify_one();
    }

  private:
    std::vector<std::thread> pool;
    std::queue<std::function<void()>> task_queue;

    std::mutex queue_mutex;
    std::condition_variable manager;
    bool stop;
};


#endif //THREAD_POOL_H