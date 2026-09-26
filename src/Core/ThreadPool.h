#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <algorithm>
#include <exception>

namespace AIDepthPro {

class ThreadPool {
public:
    static ThreadPool& getInstance() {
        static ThreadPool instance;
        return instance;
    }

    explicit ThreadPool(size_t threads = 0) : m_stop(false) {
        if (threads == 0) {
            threads = std::clamp(std::thread::hardware_concurrency(), 1u, 8u);
        }
        for (size_t i = 0; i < threads; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->m_queueMutex);
                        this->m_cv.wait(lock, [this] {
                            return this->m_stop || !this->m_tasks.empty();
                        });
                        if (this->m_stop && this->m_tasks.empty()) {
                            return;
                        }
                        task = std::move(this->m_tasks.front());
                        this->m_tasks.pop();
                    }
                    if (task) {
                        task();
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (std::thread& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_stop) return;
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

    // Wait under the same mutex used by the last worker before destroying completion state.
    template <typename Func>
    void parallelFor(int start, int end, Func func, int minChunkSize = 16) {
        int total = end - start;
        if (total <= 0) return;
        if (s_inParallel || m_workers.size() <= 1 || total <= minChunkSize) {
            for (int i=start;i<end;++i) func(i);
            return;
        }
        const int chunks=std::min(int(m_workers.size()), (total+minChunkSize-1)/minChunkSize);
        const int chunkSize=(total+chunks-1)/chunks;
        std::mutex finishMutex;
        std::condition_variable finishCv;
        int remaining=0;
        std::exception_ptr error;
        auto run=[&](int first,int last) {
            const bool previous=s_inParallel;
            s_inParallel=true;
            try { for(int i=first;i<last;++i) func(i); }
            catch (...) { std::lock_guard<std::mutex> lock(finishMutex); if(!error) error=std::current_exception(); }
            s_inParallel=previous;
        };
        for(int c=1;c<chunks;++c) {
            int first=start+c*chunkSize, last=std::min(end,first+chunkSize);
            if(first>=end) continue;
            { std::lock_guard<std::mutex> lock(finishMutex); ++remaining; }
            try {
                enqueue([&,first,last] {
                    run(first,last);
                    std::lock_guard<std::mutex> lock(finishMutex);
                    --remaining; finishCv.notify_one();
                });
            } catch (...) {
                std::lock_guard<std::mutex> lock(finishMutex);
                --remaining; if(!error) error=std::current_exception();
                break;
            }
        }
        run(start,std::min(end,start+chunkSize));
        std::unique_lock<std::mutex> lock(finishMutex);
        finishCv.wait(lock,[&]{return remaining==0;});
        if(error) std::rethrow_exception(error);
    }
    size_t getThreadCount() const {
        return m_workers.size();
    }

private:
    inline static thread_local bool s_inParallel = false;
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop;
};

} // namespace AIDepthPro
