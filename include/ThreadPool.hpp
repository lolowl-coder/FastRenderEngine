#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace fre
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t numThreads) : stop(false)
        {
            for (size_t i = 0; i < numThreads; ++i)
            {
                threads.emplace_back([this]
                {
                    while (true)
                    {
                        std::function<void()> task;
                        {
                            //std::cout << "loader tid: " << std::this_thread::get_id() << std::endl;
                            std::unique_lock<std::mutex> lock(queue_mutex);
                            condition.wait(lock, [this] { return stop || !tasks.empty(); });
                            if (stop && tasks.empty())
                                return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        template<class F>
        void enqueue(F&& f)
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.emplace(std::forward<F>(f));
            }
            condition.notify_one();
        }

        void destroy()
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            for (std::thread &worker : threads)
                worker.join();
        }

    private:
        std::vector<std::thread> threads;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
    };
}