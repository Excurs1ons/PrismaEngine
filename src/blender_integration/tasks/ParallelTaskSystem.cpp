#include "ParallelTaskSystem.h"

#include "../Logger.h"

#include <algorithm>
#include <future>

namespace Prisma {

ParallelTaskSystem::ParallelTaskSystem(size_t thread_count)
    : thread_count_(thread_count > 0 ? thread_count : 1)
    , running_(false) {
    
    if (thread_count_ == 0) {
        thread_count_ = std::thread::hardware_concurrency();
        if (thread_count_ == 0) {
            thread_count_ = 2;
        }
    }
    
    LOG_INFO("ParallelTaskSystem", "Initialized with {} worker threads", thread_count_);
}

ParallelTaskSystem::~ParallelTaskSystem() {
    Stop();
}

std::shared_ptr<ParallelTaskSystem> ParallelTaskSystem::Get() {
    static std::shared_ptr<ParallelTaskSystem> instance = std::make_shared<ParallelTaskSystem>();
    return instance;
}

void ParallelTaskSystem::Start() {
    if (running_) return;
    
    running_ = true;
    system_start_time_ = std::chrono::steady_clock::now();
    
    // 启动工作线程
    for (size_t i = 0; i < thread_count_; ++i) {
        worker_threads_.emplace_back(&ParallelTaskSystem::WorkerThread, this);
    }
    
    LOG_INFO("ParallelTaskSystem", "Task system started with {} threads", thread_count_);
}

void ParallelTaskSystem::Stop() {
    if (!running_) return;
    
    running_ = false;
    queue_cv_.notify_all();
    
    // 等待所有工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    
    LOG_INFO("ParallelTaskSystem", "Task system stopped");
}

std::future<TaskResult> ParallelTaskSystem::SubmitTask(std::shared_ptr<ITask> task) {
    if (!running_) {
        LOG_WARNING("ParallelTaskSystem", "Cannot submit task: system not running");
        // 返回一个已经完成的future
        std::promise<TaskResult> p;
        p.set_value(TaskResult{false, "System not running"});
        return p.get_future();
    }
    
    std::promise<TaskResult> promise;
    auto future = promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        QueuedTask qt;
        qt.task = std::move(task);
        qt.promise = std::move(promise);
        qt.enqueue_time = std::chrono::steady_clock::now();
        
        task_queue_.push(std::move(qt));
    }
    
    queue_cv_.notify_one();
    
    return future;
}

std::future<TaskResult> ParallelTaskSystem::SubmitTask(
    std::string id, std::string name, SimpleTask task, TaskPriority priority) {
    
    auto task_wrapper = std::make_shared<TaskWrapper>(
        std::move(id), std::move(name), std::move(task), priority);
    
    return SubmitTask(task_wrapper);
}

void ParallelTaskSystem::SubmitTaskGroup(std::shared_ptr<TaskGroup> task_group, TaskCallback callback) {
    if (!task_group) return;
    
    // 检查依赖关系并提交任务
    const auto& deps = task_group->GetDependencies();
    const auto& tasks = task_group->GetTasks();
    
    // 跟踪已完成的任务
    std::unordered_map<std::string, bool> completed;
    for (const auto& task : tasks) {
        completed[task->GetId()] = false;
    }
    
    // 逐个检查并提交任务
    for (const auto& task : tasks) {
        const auto& task_id = task->GetId();
        
        // 检查依赖
        auto dep_it = deps.find(task_id);
        if (dep_it != deps.end()) {
            bool all_deps_done = true;
            for (const auto& dep : dep_it->second) {
                if (!completed[dep]) {
                    all_deps_done = false;
                    break;
                }
            }
            if (!all_deps_done) {
                // 依赖未满足，延迟执行
                continue;
            }
        }
        
        // 提交任务
        auto future = SubmitTask(task);
        
        // 异步处理结果
        std::thread([task_id, future = std::move(future), callback, &completed, this]() mutable {
            auto result = future.get();
            completed[task_id] = result.success;
            
            if (callback) {
                callback(result);
            }
            
            // 更新统计
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.total_tasks_processed++;
                if (result.success) {
                    stats_.tasks_succeeded++;
                } else {
                    stats_.tasks_failed++;
                }
            }
            
            // 尝试执行后续任务（简单实现）
            // 实际实现需要更复杂的依赖图处理
            
        }).detach();
    }
    
    // 保存任务组引用
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        active_groups_[task_group->GetId()] = task_group;
    }
}

bool ParallelTaskSystem::CancelTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    auto it = active_tasks_.find(task_id);
    if (it != active_tasks_.end()) {
        it->second->Cancel();
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.tasks_cancelled++;
        }
        
        return true;
    }
    
    return false;
}

bool ParallelTaskSystem::CancelTaskGroup(const std::string& group_id) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    auto it = active_groups_.find(group_id);
    if (it != active_groups_.end()) {
        const auto& tasks = it->second->GetTasks();
        for (const auto& task : tasks) {
            task->Cancel();
        }
        
        active_groups_.erase(it);
        return true;
    }
    
    return false;
}

std::shared_ptr<ITask> ParallelTaskSystem::GetTask(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    auto it = active_tasks_.find(task_id);
    if (it != active_tasks_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::shared_ptr<TaskGroup> ParallelTaskSystem::GetTaskGroup(const std::string& group_id) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    auto it = active_groups_.find(group_id);
    if (it != active_groups_.end()) {
        return it->second;
    }
    
    return nullptr;
}

size_t ParallelTaskSystem::GetPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

size_t ParallelTaskSystem::GetRunningTaskCount() const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    size_t running = 0;
    for (const auto& [id, task] : active_tasks_) {
        auto wrapper = std::dynamic_pointer_cast<TaskWrapper>(task);
        if (wrapper && wrapper->GetState() == TaskState::RUNNING) {
            running++;
        }
    }
    
    return running;
}

size_t ParallelTaskSystem::GetCompletedTaskCount() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_.tasks_succeeded + stats_.tasks_failed;
}

float ParallelTaskSystem::GetSystemLoad() const {
    size_t running = GetRunningTaskCount();
    return static_cast<float>(running) / static_cast<float>(thread_count_);
}

void ParallelTaskSystem::WorkerThread() {
    LOG_DEBUG("ParallelTaskSystem", "Worker thread started");
    
    while (running_) {
        std::shared_ptr<ITask> task = GetNextTask();
        
        if (!task) {
            // 没有任务，等待
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait_for(lock, std::chrono::milliseconds(100),
                              [this] { 
                                  return !task_queue_.empty() || !running_; 
                              });
            continue;
        }
        
        ExecuteTask(task);
    }
    
    LOG_DEBUG("ParallelTaskSystem", "Worker thread exiting");
}

std::shared_ptr<ITask> ParallelTaskSystem::GetNextTask() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (task_queue_.empty()) {
        return nullptr;
    }
    
    // 获取最高优先级任务
    auto qt = std::move(const_cast<QueuedTask&>(task_queue_.top()));
    task_queue_.pop();
    
    // 保存到活动任务映射
    {
        std::lock_guard<std::mutex> task_lock(task_mutex_);
        active_tasks_[qt.task->GetId()] = qt.task;
    }
    
    // 返回future的promise
    // 注意：这里简化处理，实际需要更复杂的future管理
    
    return qt.task;
}

void ParallelTaskSystem::ExecuteTask(std::shared_ptr<ITask> task) {
    if (!task) return;
    
    auto start_time = std::chrono::steady_clock::now();
    
    LOG_DEBUG("ParallelTaskSystem", "Executing task: {} ({})", 
              task->GetName(), task->GetId());
    
    try {
        TaskResult result = task->Execute();
        
        auto end_time = std::chrono::steady_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        // 更新统计
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_tasks_processed++;
            if (result.success) {
                stats_.tasks_succeeded++;
            } else {
                stats_.tasks_failed++;
            }
            stats_.total_execution_time_ms += duration_ms;
            stats_.avg_task_time_ms = stats_.total_execution_time_ms / stats_.total_tasks_processed;
        }
        
        LOG_DEBUG("ParallelTaskSystem", "Task completed: {} in {:.2f}ms, success: {}",
                  task->GetName(), duration_ms, result.success);
        
    } catch (const std::exception& e) {
        LOG_ERROR("ParallelTaskSystem", "Task failed with exception: {}", e.what());
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_tasks_processed++;
        stats_.tasks_failed++;
    }
    
    // 从活动任务中移除
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        active_tasks_.erase(task->GetId());
    }
}

void ParallelTaskSystem::ExecuteTaskGroup(std::shared_ptr<TaskGroup> task_group, TaskCallback callback) {
    if (!task_group) return;
    
    const auto& tasks = task_group->GetTasks();
    
    // 并行执行所有任务
    std::vector<std::future<TaskResult>> futures;
    futures.reserve(tasks.size());
    
    for (const auto& task : tasks) {
        futures.push_back(SubmitTask(task));
    }
    
    // 等待所有任务完成
    bool all_success = true;
    for (auto& future : futures) {
        auto result = future.get();
        if (!result.success) {
            all_success = false;
        }
        if (callback) {
            callback(result);
        }
    }
    
    LOG_INFO("ParallelTaskSystem", "Task group '{}' completed, success: {}",
             task_group->GetName(), all_success);
}

bool ParallelTaskSystem::CheckDependencies(
    const std::string& task_id,
    const std::unordered_map<std::string, std::vector<std::string>>& deps) {
    
    auto it = deps.find(task_id);
    if (it == deps.end()) {
        return true; // 没有依赖
    }
    
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    for (const auto& dep_id : it->second) {
        auto dep_task = active_tasks_.find(dep_id);
        if (dep_task == active_tasks_.end()) {
            continue; // 依赖任务不存在，假设已完成
        }
        
        auto wrapper = std::dynamic_pointer_cast<TaskWrapper>(dep_task->second);
        if (!wrapper) continue;
        
        if (wrapper->GetState() != TaskState::COMPLETED) {
            return false; // 依赖未完成
        }
    }
    
    return true;
}

} // namespace Prisma