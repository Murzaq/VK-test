#include <iostream>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <ctime>

class TaskScheduler {
public:
    TaskScheduler() : stop_flag(false) {
        worker_thread = std::thread(&TaskScheduler::workerFunction, this);
    }

    ~TaskScheduler() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }
        condition.notify_one();
        worker_thread.join();
    }

    void Add(std::function<void()> task, std::time_t timestamp) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        task_queue.push(TaskItem{ timestamp, task });
        condition.notify_one();
    }

private:
    struct TaskItem {
        std::time_t timestamp;
        std::function<void()> task;

        bool operator>(const TaskItem& other) const {
            return timestamp > other.timestamp;
        }
    };

    std::priority_queue<TaskItem, std::vector<TaskItem>, std::greater<TaskItem>> task_queue;
    std::thread worker_thread;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop_flag;

    void workerFunction() {
        while (true) {
            std::function<void()> current_task;
            std::time_t current_time = std::time(nullptr);

            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                if (stop_flag && task_queue.empty()) {
                    break;
                }

                if (task_queue.empty()) {
                    condition.wait(lock);
                    continue;
                }

                TaskItem next_task = task_queue.top();

                if (next_task.timestamp <= current_time) {
                    current_task = next_task.task;
                    task_queue.pop();
                }
                else {
                    condition.wait_until(lock, std::chrono::system_clock::from_time_t(next_task.timestamp));
                    continue;
                }
            }

            if (current_task) {
                current_task();
            }
        }
    }
};

int main() {
    setlocale(LC_ALL, "Russian");
    TaskScheduler scheduler;

    scheduler.Add([]() {
        std::cout << "Задача 1 выполнена!\n";
        }, std::time(nullptr) + 5); 

    scheduler.Add([]() {
        std::cout << "Задача 2 выполнена!\n";
        }, std::time(nullptr) + 2); 

    std::this_thread::sleep_for(std::chrono::seconds(7));
    return 0;
}
