/**
 * @file parallel_linear_search.hpp
 * @brief Parallelizes the traiditional linear search algorithm
 * without altering its results.
 * @author Canberk Sonmez <canberk.sonmez@epfl.ch>
 * 
 */

#ifndef PARALLEL_LINEAR_SEARCH_HPP_INCLUDED
#define PARALLEL_LINEAR_SEARCH_HPP_INCLUDED

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
#include <any>
#include <cassert>

/**
 * 
 * TODO I may optimize the code further by revoking the use of std::function<> and
 * std::any, both of which rely on heap allocations.
 * 
 */

#ifdef PARALLEL_LINEAR_SEARCH_DEBUG_MSGS
#include "ostream_mt.hpp"
#include "print.hpp"

#define DEBUG \
    print(cout_mt()) \
    ("[ PLS_DEBUG ] ").ln

#define DEBUG_WORKER \
    print(cout_mt()) \
    ("[ PLS_DEBUG ] worker idx = ", idx, " ").ln

#define DEBUG_WORKER_TASK \
    print(cout_mt()) \
    ("[ PLS_DEBUG ] worker idx = ", idx, " task idx = ", task->idx, " ").ln

#else
#include "print.hpp"

#define DEBUG fake_print_functor{}
#define DEBUG_WORKER fake_print_functor{}
#define DEBUG_WORKER_TASK fake_print_functor{}

#endif

namespace multithreading {

/**
 * 
 * A task queue of infinite length with cancellation option.
 * 
 */
template <typename Task>
struct CancellableQueue {
    CancellableQueue() {
        reset();
    }

    // should be called before execution
    void reset() {
        std::lock_guard<std::mutex> lock{mtx_};
        cancel_ = false;
        tasks_.clear();
    }

    // adds a new task to the queue
    template <typename ...Args>
    void emplace_back(Args && ...args) {
        std::lock_guard<std::mutex> lock{mtx_};
        assert(!cancel_ || "reset() the queue first!");
        tasks_.emplace_back(std::forward<Args>(args)...);
        cv_.notify_all();
    }

    // pops a task from the queue. if the queue is empty, waits until:
    //   1. a new item is added
    //   2. the queue is cancelled
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

    // removes all the tasks and sends an null task to the waiting threads.
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

struct Empty {};

/**
 *
 * Implements a parallelized version of the linear search algorithm.
 * It outputs exactly the same result as the sequential linear search.
 * 
 */
template <typename ClosureType, typename WorkerData = Empty>
struct ParallelLinearSearch {
    // closure type should have:
    //   - bool operator()(std::size_t idx, WorkerData const &worker_data)

    ParallelLinearSearch(std::size_t num_workers):
        shared_state_{num_workers},
        num_workers_{num_workers} {
        // generate the workers
        for (std::size_t i = 0; i < num_workers_; ++i) {
            workers_.emplace_back(std::make_unique<Worker>(shared_state_, i));
        }
    }

    // returns the number of workers.
    std::size_t num_workers() const {
        return workers_.size();
    }

    // gets the worker specific data
    WorkerData &worker_data(std::size_t i) {
        return (WorkerData &) workers_[i];
    }

    WorkerData const &worker_data(std::size_t i) const {
        return (WorkerData const &) workers_[i];
    }

    // whether or not continue (false if success)
    bool should_continue() const {
        return !shared_state_.success();
    }

    // Assigns a new job to workers.
    template <typename T>
    void append_job(T &&t) {
        shared_state_.task_queue.emplace_back(Task{std::move(t), false, num_tasks_});
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
        // add invalid jobs that stop the worker
        for (std::size_t i = 0; i < num_workers_; ++i)
            shared_state_.task_queue.emplace_back(Task{{}, true, 0});
        
        shared_state_.join_workers();
    }

    struct ResultType {
        std::size_t idx;
        ClosureType closure;
    };

    // Get the result
    std::optional<ResultType> result() {
        if (shared_state_.success()) {
            return ResultType{shared_state_.success_idx(), std::move(shared_state_.success_data())};
        }
        return {};
    }

    ~ParallelLinearSearch() {
        for (auto &worker: workers_) worker->quit();
    }
private:
    struct Task {
        // closure for the task
        ClosureType closure;

        // Flag if invalid
        bool invalid = false;

        // index of the task
        std::size_t idx = 0;
    };

    struct SharedState {
        CancellableQueue<Task> task_queue;

        SharedState(std::size_t num_workers):
            num_workers_{num_workers} {
        }

        // Resets the shared state
        void reset() {
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
        void inform_completion(std::size_t idx, ClosureType &&closure, bool r) {
            std::lock_guard<std::mutex> lock{mtx_};

            // append -1 until the size is good to go
            while (idx >= completion_array_.size()) {
                completion_array_.push_back(-1);
            }

            completion_array_[idx] = r;

            if (r) {
                if (!success_ || success_idx_ > idx) {
                    success_ = true;
                    success_idx_ = idx;
                    success_closure_ = std::move(closure);
                }
            }

            while (
                // make sure that we do not exceed the array size during iteration
                num_contiguous_completed_from_beginning_ < completion_array_.size() &&
                // increase the variable as long as we have a result
                completion_array_[num_contiguous_completed_from_beginning_] >= 0)
                ++num_contiguous_completed_from_beginning_;

            // search completed successfully, and a contiguous range is covered
            if (success_ && num_contiguous_completed_from_beginning_ > idx) {
                done_ = true;
                task_queue.cancel();
            }
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

        ClosureType &success_data() {
            // no need for locks here
            return success_closure_;
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

        // task queue


        // -1 --> Not initialized
        // 0  --> Completed, not successful
        // 1  --> Completed, successful
        std::vector<int> completion_array_;

        std::size_t num_contiguous_completed_from_beginning_;
        bool done_;
        bool success_;
        std::size_t success_idx_;
        ClosureType success_closure_;
        std::mutex mtx_;

        // for joining the workers
        std::size_t num_remaining_workers_;
        std::condition_variable cv_join_;
        std::mutex mtx_join_;
    };

    // For synchronization among workers.
    SharedState shared_state_;

    struct Worker: WorkerData {
        Worker(SharedState &shared_state, std::size_t idx): start_{false}, quit_{false} {
            thread_ = std::thread([&, idx] {
                DEBUG_WORKER("start_worker");

                while (true) {
                    // wait until started or quit requested
                    std::unique_lock<std::mutex> lock{mtx_};
                    cv_.wait(lock, [this] { return start_ || quit_; });

                    if (quit_) {
                        DEBUG_WORKER("quit");
                        return ;
                    }

                    DEBUG_WORKER("start");
                    
                    while (true) {
                        auto task = shared_state.task_queue.pop_front();
                        if (task) {
                            if (task->invalid) {
                                DEBUG_WORKER("invalid_task");
                                break ;
                            }

                            DEBUG_WORKER_TASK();

                            bool r = task->closure(task->idx, *this);
                            shared_state.inform_completion(task->idx, std::move(task->closure), r);

                            DEBUG_WORKER_TASK("task_done");

                            if (r) {
                                DEBUG_WORKER_TASK("task_success");
                                break ;
                            }
                        }
                        else {
                            DEBUG_WORKER("not_a_task");
                            
                            // cancelled
                            break ;
                        }
                    }

                    start_ = false;

                    DEBUG_WORKER("inform_worker_done");

                    shared_state.inform_worker_done();

                    DEBUG_WORKER("end");
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

#endif // PARALLEL_LINEAR_SEARCH_HPP_INCLUDED
