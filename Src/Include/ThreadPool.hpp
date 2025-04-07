#pragma once

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
        ThreadPool(size_t numThreads) : mStop(false)
        {
            for (size_t i = 0; i < numThreads; ++i)
            {
                mThreads.emplace_back([this]
                {
                    while (true)
                    {
                        std::function<void()> task;
                        {
                            //std::cout << "loader tid: " << std::this_thread::get_id() << std::endl;
                            std::unique_lock<std::mutex> lock(mMutex);
                            mCondition.wait(lock, [this] { return mStop || !mTasks.empty(); });
                            if (mStop && mTasks.empty())
                                return;
                            task = std::move(mTasks.front());
                            mTasks.pop();
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
                std::unique_lock<std::mutex> lock(mMutex);
                mTasks.emplace(std::forward<F>(f));
            }
            mCondition.notify_one();
        }

        void destroy()
        {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mStop = true;
            }
            mCondition.notify_all();
            for (std::thread &worker : mThreads)
                worker.join();
        }

        bool isStopped()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            return mStop;
        }

    private:
        std::vector<std::thread> mThreads;
        std::queue<std::function<void()>> mTasks;
        std::mutex mMutex;
        std::condition_variable mCondition;
        bool mStop;
    };
}