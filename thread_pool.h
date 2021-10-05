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
#include <future>

class ThreadPool {
  public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency()): pool{}, task_queue{}, queue_mutex{}, manager{}, exit{false} {

        auto worker =   [this] {
                            for(;;) {
                                std::function<void()> task;
                                {
                                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                                    this->manager.wait(lock, [this]{ return this->exit || !this->task_queue.empty(); });
                                    if(this->exit && this->task_queue.empty())
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
            exit = true;
        }
        manager.notify_all();
        for(auto& thread: pool)
            thread.join();
    }

    template <std::invocable T, typename... Args>
    void push_work(T&& t, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            task_queue.push([t](){t(args...);});
        }
        manager.notify_one();
    }

    template <typename F, typename... Args>
    [[nodiscard]] auto push_task(F&& f, Args&&... args)-> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            task_queue.push([task](){(*task)();});
        }
        manager.notify_one();
        return result;
    }

  private:
    std::vector<std::thread> pool;
    std::queue<std::function<void()>> task_queue;

    std::mutex queue_mutex;
    std::condition_variable manager;
    bool exit;
};


#endif //THREAD_POOL_H