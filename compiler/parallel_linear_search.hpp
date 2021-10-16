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
#include <optional>
#include <cassert>

#ifdef PARALLEL_LINEAR_SEARCH_DEBUG
#include "ostream_mt.hpp"
#endif

namespace multithreading {

/**
 * 
 * A task queue of infinite length with cancellation option.
 * 
 */
template <typename Task>
struct CancellableTaskQueue {
    CancellableTaskQueue() {
        reset();
    }

    // should be called before execution
    void reset() {
        std::lock_guard<std::mutex> lock{mtx_};
        cancel_ = false;
        tasks_.clear();
    }

    template <typename ...Args>
    void emplace_back(Args && ...args) {
        std::lock_guard<std::mutex> lock{mtx_};
        assert(!cancel_ || "reset() the queue first!");
        tasks_.emplace_back(std::forward<Args>(args)...);
        cv_.notify_all();
    }

    std::optional<Task> pop_front() {
        std::unique_lock<std::mutex> lock{mtx_};
        cv_.wait(lock, [this]{ return tasks_.size() > 0 || cancel_; });
        if (cancel_) {
            return {};
        }
        auto popped = tasks_.front();
        tasks_.pop_front();
        return popped;
    }

    // removes all the tasks and sends a nullptr_t to waiting threads.
    void cancel() {
        std::lock_guard<std::mutex> lock{mtx_};
        tasks_.clear();
        cancel_ = true;
        cv_.notify_all();
    }
private:
    std::list<Task> tasks_;
    bool cancel_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

/**
 *
 * Implements a parallelized version of the linear search algorithm.
 * It outputs exactly the same result as the sequential linear search.
 * 
 */
struct ParallelLinearSearch {
    ParallelLinearSearch(std::size_t num_workers):
        shared_state_{num_workers},
        num_workers_{num_workers} {
        // generate the workers
        for (std::size_t i = 0; i < num_workers_; ++i) {
            workers_.emplace_back(std::make_unique<Worker>(shared_state_, i));
        }
    }

    // whether or not continue (false if success)
    bool should_continue() const {
        return !shared_state_.success();
    }

    // Assigns a new job to workers.
    void append_job(std::function<bool (std::size_t)> f) {
        shared_state_.task_queue.emplace_back(Task{std::move(f), num_tasks_});
        ++num_tasks_;
    }

    // Starts execution.
    void begin() {
        shared_state_.reset();
        num_tasks_ = 0;
        for (auto &worker: workers_) worker->start();
    }

    // Sets the end of the queue and waits for the execution to complete.
    void end() {
        shared_state_.set_num_tasks(num_tasks_);
        shared_state_.join_workers();
    }

    // Get the result
    std::optional<std::size_t> result() {
        if (shared_state_.success()) {
            return shared_state_.success_idx();
        }
        return {};
    }

    ~ParallelLinearSearch() {
        for (auto &worker: workers_) worker->quit();
    }
private:
    struct Task {
        std::function<bool (std::size_t)> f;
        std::size_t idx;
    };

    struct SharedState {
        CancellableTaskQueue<Task> task_queue;

        SharedState(std::size_t num_workers):
            num_workers_{num_workers} {
        }

        // Resets the shared state
        void reset() {
            num_tasks_set_ = false;
            completion_array_.clear();
            num_contiguous_completed_from_beginning_ = 0;
            done_ = false;
            success_ = false;
            success_idx_ = 0;
            num_remaining_workers_ = num_workers_;
            task_queue.reset();
        }

        // informs the shared state that one of the workers have found a result
        // sets the done flag if it can judge the execution is complete
        void inform_completion(std::size_t idx, bool r) {
            std::lock_guard<std::mutex> lock{mtx_};

            // append -1 until the size is good to go
            while (idx >= completion_array_.size()) {
                completion_array_.push_back(-1);
            }

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
                // case 2: the whole numbers are covered with no success
                (num_tasks_set_ && num_contiguous_completed_from_beginning_ == num_tasks_)) {
                done_ = true;
                task_queue.cancel();
            }

            while (
                // make sure that we do not exceed the array size during iteration
                num_contiguous_completed_from_beginning_ < completion_array_.size() &&
                // increase the variable as long as we have a result
                completion_array_[num_contiguous_completed_from_beginning_] >= 0)
                ++num_contiguous_completed_from_beginning_;
        }

        void set_num_tasks(std::size_t num_tasks) {
            std::lock_guard<std::mutex> lock{mtx_};
            num_tasks_set_ = true;
            num_tasks_ = num_tasks;
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
        bool num_tasks_set_;

        // task queue


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

    struct Worker {
        Worker(SharedState &shared_state, std::size_t idx): start_{false}, quit_{false} {
            thread_ = std::thread([&, idx] {
                #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                cout_mt() << "Worker idx = " << idx << " start_worker" << "\n";
                #endif

                while (true) {
                    // wait until started or quit requested
                    std::unique_lock<std::mutex> lock{mtx_};
                    cv_.wait(lock, [this] { return start_ || quit_; });

                    if (quit_) {
                        #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                        cout_mt() << "Worker idx = " << idx << " quit_" << "\n";
                        #endif
                        return ;
                    }

                    #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                    cout_mt() << "Worker idx = " << idx << " start_" << "\n";
                    #endif
                    
                    while (true) {
                        auto task = shared_state.task_queue.pop_front();
                        if (task) {
                            #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                            cout_mt() << "Worker idx = " << idx << " task idx = " << task->idx << "\n";
                            #endif

                            bool r = task->f(task->idx);
                            shared_state.inform_completion(task->idx, r);

                            #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                            cout_mt() << "Worker idx = " << idx << " task done idx = " << task->idx << "\n";
                            #endif

                            if (r) {
                                #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                                cout_mt() << "Worker idx = " << idx << " success" << "\n";
                                #endif

                                break ;
                            }
                        }
                        else {
                            #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                            cout_mt() << "Worker idx = " << idx << " !task" << "\n";
                            #endif
                            
                            // cancelled
                            break ;
                        }
                    }

                    start_ = false;

                    #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                    cout_mt() << "Worker idx = " << idx << " inform_worker_done" << "\n";
                    #endif

                    shared_state.inform_worker_done();

                    #ifdef PARALLEL_LINEAR_SEARCH_DEBUG
                    cout_mt() << "Worker idx = " << idx << " end" << "\n";
                    #endif
                }
            });
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
    std::size_t num_tasks_;
    std::vector<std::unique_ptr<Worker>> workers_;
};

}

#endif // MULTITHREADING_HPP_INCLUDED
