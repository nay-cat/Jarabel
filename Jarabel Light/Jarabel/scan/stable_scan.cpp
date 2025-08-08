#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>
#include <windows.h>
#include <stdexcept>

#include "stable_scan.hpp"

namespace fs = std::filesystem;

class ThreadPool {
public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                [this] {
                    for (;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                [this] { return this->stop || !this->tasks.empty(); });
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                }
            );
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                return;
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

class SearchCheck {
public:
    SearchCheck(FOUND_JAR_CALLBACK cb, void* ctx)
        : pool(std::thread::hardware_concurrency()), callback(cb), context(ctx) {
    }

    void execute() {
        auto drives = getLogicalDrives();
        for (const auto& drive : drives) {
            pool.enqueue([this, drive_str = drive] {
                collectJarFiles(drive_str);
            });
        }
    }

private:
    ThreadPool pool;
    FOUND_JAR_CALLBACK callback;
    void* context;

    void collectJarFiles(const std::string& rootPath) {
        fs::path safeRoot = rootPath;
        if (!fs::exists(safeRoot) || !fs::is_directory(safeRoot)) {
            return;
        }

        fs::recursive_directory_iterator it(safeRoot, fs::directory_options::skip_permission_denied);
        fs::recursive_directory_iterator end;

        while (it != end) {
            try {
                if (it->path().extension() == ".jar") {
                    callback(it->path().c_str(), context);
                }
                ++it;
            }
            catch (const fs::filesystem_error& e) {
                UNREFERENCED_PARAMETER(e);
            }
        }
    }

    std::vector<std::string> getLogicalDrives() {
        std::vector<std::string> drives;
        DWORD driveMask = GetLogicalDrives();
        for (char i = 0; i < 26; ++i) {
            if ((driveMask >> i) & 1) {
                std::string drive_path = "";
                drive_path += ('A' + i);
                drive_path += ":\\";
                drives.push_back(drive_path);
            }
        }
        return drives;
    }
};

extern "C" void RunStableScan(FOUND_JAR_CALLBACK callback, void* context) {
        SearchCheck searchCheck(callback, context);
        searchCheck.execute();
}