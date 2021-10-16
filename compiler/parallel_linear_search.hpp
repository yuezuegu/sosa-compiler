/**
 * @file parallel_linear_search.hpp
 * @brief Parallelizes the traiditional linear search algorithm
 * without altering its results.
 * @author Canberk Sonmez <canberk.sonmez@epfl.ch>
 * 
 */

#ifndef MULTITHREADING_HPP_INCLUDED
#define MULTITHREADING_HPP_INCLUDED

#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <vector>
#include <sstream>
#include <memory>
#include <functional>
#include <type_traits>

namespace multithreading {

/**
 *
 * Implements a parallelized version of the linear search algorithm.
 * It outputs exactly the same result as the sequential linear search.
 * 
 */
struct ParallelLinearSearch {
    ParallelLinearSearch(std::size_t num_workers):
        shared_state_{num_workers},
        num_workers_{num_workers},
        next_worker_{0},
        num_tasks_{0} {
        // generate the workers
        for (std::size_t i = 0; i < num_workers_; ++i) {
            workers_.emplace_back(std::make_unique<Worker>(shared_state_));
        }
    }

    // Assigns a new job to workers.
    void append_job(std::function<bool (std::size_t)> f) {
        workers_[next_worker_]->emplace_task(std::move(f), num_tasks_);
        num_tasks_++;
        next_worker_ = (next_worker_ + 1) % num_workers_;
    }

    // Starts execution and waits until completion.
    bool execute(std::size_t &idx) {
        shared_state_.reset(num_tasks_);

        for (auto &worker: workers_) worker->start();

        shared_state_.join_workers();

        // reset
        next_worker_ = 0;
        num_tasks_ = 0;

        idx = shared_state_.success_idx();
        return shared_state_.success();
    }

    ~ParallelLinearSearch() {
        for (auto &worker: workers_) worker->quit();
    }
private:
    struct SharedState {
        SharedState(std::size_t num_workers):
            num_workers_{num_workers} {
        }

        // Resets the shared state
        void reset(std::size_t num_tasks) {
            num_tasks_ = num_tasks;
            completion_array_ = std::vector<int>(num_tasks, -1);
            num_contiguous_completed_from_beginning_ = 0;
            done_ = false;
            success_ = false;
            success_idx_ = 0;
            num_remaining_workers_ = num_workers_;
        }

        // informs the shared state that one of the workers have found the result
        // sets the done flag if it can judge the execution is complete
        void inform_completion(std::size_t idx, bool r) {
            std::lock_guard<std::mutex> lock{mtx_};
            completion_array_[idx] = r;

            if (r) {
                if (!success_) {
                    success_ = true;
                    success_idx_ = idx;
                }

                if (success_idx_ > idx) {
                    success_idx_ = idx;
                }
            }

            if (
                // case 1: search completed successfully, and a contiguous range is covered
                (success_ && num_contiguous_completed_from_beginning_ == idx) ||
                // case 2: the whole range is covered with no success
                (num_contiguous_completed_from_beginning_ == num_tasks_)) {
                done_ = true;
                return ;
            }

            while (completion_array_[num_contiguous_completed_from_beginning_] >= 0)
                ++num_contiguous_completed_from_beginning_;
        }

        bool done() const {
            // no need for locks here
            return done_;
        }

        bool success() const {
            // no need for locks here
            return success_;
        }

        std::size_t success_idx() const {
            // no need for locks here
            return success_idx_;
        }

        void join_workers() {
            std::unique_lock<std::mutex> lock{mtx_join_};
            cv_join_.wait(lock, [this] { return num_remaining_workers_ == 0; });
        }

        void inform_worker_done() {
            std::unique_lock<std::mutex> lock{mtx_join_};
            --num_remaining_workers_;
            cv_join_.notify_all();
        }

    private:
        std::size_t num_workers_;
        std::size_t num_tasks_;

        // -1 --> Not initialized
        // 0  --> Completed, not successful
        // 1  --> Completed, successful
        std::vector<int> completion_array_;

        std::size_t num_contiguous_completed_from_beginning_;
        bool done_;
        bool success_;
        std::size_t success_idx_;
        std::mutex mtx_;

        // for joining the workers
        std::size_t num_remaining_workers_;
        std::condition_variable cv_join_;
        std::mutex mtx_join_;
    };

    // For synchronization among workers.
    SharedState shared_state_;

    struct Task {
        std::function<bool (std::size_t)> f;
        std::size_t idx;
    };

    struct Worker {
        Worker(SharedState &shared_state): start_{false}, quit_{false} {
            thread_ = std::thread([&] {
                while (true) {
                    // wait until started or quit requested
                    std::unique_lock<std::mutex> lock{mtx_};
                    cv_.wait(lock, [this] { return start_ || quit_; });

                    if (quit_)
                        return ;
                    
                    for (auto &task: tasks_) {
                        bool r = task.f(task.idx);
                        if (r) {
                            shared_state.inform_completion(task.idx, r);
                            break ;
                        }
                    }

                    start_ = false;
                    tasks_.clear();
                    shared_state.inform_worker_done();
                }
            });
        }

        // Emplaces a new task for the worker.
        void emplace_task(std::function<bool (std::size_t)> f, std::size_t idx) {
            tasks_.emplace_back(Task{ std::move(f), idx });
        }

        // Starts the worker.
        void start() {
            std::lock_guard<std::mutex> lock{mtx_};
            start_ = true;
            cv_.notify_all();
        }

        // Signals that the worker should quit.
        void quit() {
            std::lock_guard<std::mutex> lock{mtx_};
            quit_ = true;
            cv_.notify_all();
        }

        ~Worker() {
            if (thread_.joinable())
                thread_.join();
        }
    private:
        std::list<Task> tasks_;
        std::thread thread_;
        std::mutex mtx_;
        std::condition_variable cv_;

        // start flag
        // we need this in case of spurious wake ups
        bool start_;

        // quit flag
        bool quit_;
    };

    std::size_t num_workers_;
    std::vector<std::unique_ptr<Worker>> workers_;
    
    // task management
    std::size_t next_worker_;
    std::size_t num_tasks_;
};

}

#endif // MULTITHREADING_HPP_INCLUDED
