#pragma once

#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <unordered_map>
#include <string>

namespace Prisma {

// 任务优先级
enum class TaskPriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
};

// 任务状态
enum class TaskState {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

// 任务结果
struct TaskResult {
    bool success = false;
    std::string error_message;
    std::any data; // 任务特定数据
};

// 基础任务接口
class ITask {
public:
    virtual ~ITask() = default;
    
    virtual std::string GetId() const = 0;
    virtual std::string GetName() const = 0;
    virtual TaskPriority GetPriority() const = 0;
    
    virtual TaskResult Execute() = 0;
    virtual void Cancel() = 0;
    
    virtual bool IsCancellable() const = 0;
    virtual bool SupportsProgress() const = 0;
    
    // 进度报告 (0.0 - 1.0)
    virtual float GetProgress() const { return 0.0f; }
    virtual std::string GetProgressMessage() const { return ""; }
};

// 具体任务类型
using SimpleTask = std::function<TaskResult()>;
using TaskCallback = std::function<void(const TaskResult&)>;

// 任务包装器
class TaskWrapper : public ITask {
public:
    TaskWrapper(std::string id, std::string name, SimpleTask task, 
                TaskPriority priority = TaskPriority::NORMAL)
        : id_(std::move(id))
        , name_(std::move(name))
        , task_(std::move(task))
        , priority_(priority)
        , state_(TaskState::PENDING) {}
    
    std::string GetId() const override { return id_; }
    std::string GetName() const override { return name_; }
    TaskPriority GetPriority() const override { return priority_; }
    
    TaskResult Execute() override {
        state_ = TaskState::RUNNING;
        try {
            result_ = task_();
            state_ = result_.success ? TaskState::COMPLETED : TaskState::FAILED;
            return result_;
        } catch (const std::exception& e) {
            result_.success = false;
            result_.error_message = e.what();
            state_ = TaskState::FAILED;
            return result_;
        }
    }
    
    void Cancel() override {
        if (state_ == TaskState::PENDING || state_ == TaskState::RUNNING) {
            state_ = TaskState::CANCELLED;
            result_.success = false;
            result_.error_message = "Task cancelled";
        }
    }
    
    bool IsCancellable() const override { return true; }
    bool SupportsProgress() const override { return false; }
    
    TaskState GetState() const { return state_; }
    const TaskResult& GetResult() const { return result_; }
    
private:
    std::string id_;
    std::string name_;
    SimpleTask task_;
    TaskPriority priority_;
    std::atomic<TaskState> state_;
    TaskResult result_;
};

// 进度报告任务
class ProgressTask : public TaskWrapper {
public:
    ProgressTask(std::string id, std::string name, SimpleTask task,
                 TaskPriority priority = TaskPriority::NORMAL,
                 std::function<float()> progress_func = nullptr,
                 std::function<std::string()> progress_msg_func = nullptr)
        : TaskWrapper(std::move(id), std::move(name), std::move(task), priority)
        , progress_func_(std::move(progress_func))
        , progress_msg_func_(std::move(progress_msg_func)) {}
    
    bool SupportsProgress() const override { return true; }
    float GetProgress() const override { 
        return progress_func_ ? progress_func_() : 0.0f; 
    }
    std::string GetProgressMessage() const override { 
        return progress_msg_func_ ? progress_msg_func_() : ""; 
    }
    
private:
    std::function<float()> progress_func_;
    std::function<std::string()> progress_msg_func_;
};

// 任务组
class TaskGroup {
public:
    TaskGroup(std::string id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {}
    
    void AddTask(std::shared_ptr<ITask> task) {
        tasks_.push_back(std::move(task));
    }
    
    void AddDependency(const std::string& task_id, const std::string& depends_on_id) {
        dependencies_[task_id].push_back(depends_on_id);
    }
    
    const std::vector<std::shared_ptr<ITask>>& GetTasks() const { return tasks_; }
    const std::unordered_map<std::string, std::vector<std::string>>& GetDependencies() const { 
        return dependencies_; 
    }
    
    std::string GetId() const { return id_; }
    std::string GetName() const { return name_; }
    
    size_t GetCompletedCount() const {
        size_t count = 0;
        for (const auto& task : tasks_) {
            auto wrapper = std::dynamic_pointer_cast<TaskWrapper>(task);
            if (wrapper && wrapper->GetState() == TaskState::COMPLETED) {
                count++;
            }
        }
        return count;
    }
    
    size_t GetFailedCount() const {
        size_t count = 0;
        for (const auto& task : tasks_) {
            auto wrapper = std::dynamic_pointer_cast<TaskWrapper>(task);
            if (wrapper && wrapper->GetState() == TaskState::FAILED) {
                count++;
            }
        }
        return count;
    }
    
    float GetProgress() const {
        if (tasks_.empty()) return 1.0f;
        
        size_t completed = 0;
        float total_progress = 0.0f;
        
        for (const auto& task : tasks_) {
            if (task->SupportsProgress()) {
                total_progress += task->GetProgress();
            } else {
                auto wrapper = std::dynamic_pointer_cast<TaskWrapper>(task);
                if (wrapper) {
                    switch (wrapper->GetState()) {
                        case TaskState::COMPLETED:
                        case TaskState::FAILED:
                        case TaskState::CANCELLED:
                            total_progress += 1.0f;
                            break;
                        case TaskState::RUNNING:
                            total_progress += 0.5f; // 估计值
                            break;
                        default:
                            total_progress += 0.0f;
                            break;
                    }
                }
            }
        }
        
        return total_progress / tasks_.size();
    }
    
private:
    std::string id_;
    std::string name_;
    std::vector<std::shared_ptr<ITask>> tasks_;
    std::unordered_map<std::string, std::vector<std::string>> dependencies_;
};

// 并行任务系统
class ParallelTaskSystem {
public:
    ParallelTaskSystem(size_t thread_count = std::thread::hardware_concurrency());
    ~ParallelTaskSystem();
    
    // 单例访问
    static std::shared_ptr<ParallelTaskSystem> Get();
    
    // 系统控制
    void Start();
    void Stop();
    bool IsRunning() const { return running_; }
    
    // 任务提交
    std::future<TaskResult> SubmitTask(std::shared_ptr<ITask> task);
    std::future<TaskResult> SubmitTask(std::string id, std::string name, SimpleTask task, 
                                       TaskPriority priority = TaskPriority::NORMAL);
    
    // 任务组提交
    void SubmitTaskGroup(std::shared_ptr<TaskGroup> task_group, TaskCallback callback = nullptr);
    
    // 任务管理
    bool CancelTask(const std::string& task_id);
    bool CancelTaskGroup(const std::string& group_id);
    
    std::shared_ptr<ITask> GetTask(const std::string& task_id) const;
    std::shared_ptr<TaskGroup> GetTaskGroup(const std::string& group_id) const;
    
    // 状态查询
    size_t GetPendingTaskCount() const;
    size_t GetRunningTaskCount() const;
    size_t GetCompletedTaskCount() const;
    
    float GetSystemLoad() const; // 0.0 - 1.0
    
    // 统计信息
    struct SystemStats {
        size_t total_tasks_processed = 0;
        size_t tasks_succeeded = 0;
        size_t tasks_failed = 0;
        size_t tasks_cancelled = 0;
        double total_execution_time_ms = 0.0;
        double avg_task_time_ms = 0.0;
        
        void Reset() {
            total_tasks_processed = 0;
            tasks_succeeded = 0;
            tasks_failed = 0;
            tasks_cancelled = 0;
            total_execution_time_ms = 0.0;
            avg_task_time_ms = 0.0;
        }
    };
    
    const SystemStats& GetStats() const { return stats_; }
    
private:
    // 工作线程函数
    void WorkerThread();
    
    // 任务调度
    std::shared_ptr<ITask> GetNextTask();
    
    // 任务执行
    void ExecuteTask(std::shared_ptr<ITask> task);
    void ExecuteTaskGroup(std::shared_ptr<TaskGroup> task_group, TaskCallback callback);
    
    // 依赖检查
    bool CheckDependencies(const std::string& task_id, 
                          const std::unordered_map<std::string, std::vector<std::string>>& deps);
    
private:
    std::atomic<bool> running_{false};
    size_t thread_count_;
    
    // 工作线程
    std::vector<std::thread> worker_threads_;
    
    // 任务队列
    struct QueuedTask {
        std::shared_ptr<ITask> task;
        std::promise<TaskResult> promise;
        std::chrono::steady_clock::time_point enqueue_time;
        
        bool operator<(const QueuedTask& other) const {
            // 高优先级任务先执行
            return static_cast<int>(task->GetPriority()) < static_cast<int>(other.task->GetPriority());
        }
    };
    
    std::priority_queue<QueuedTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // 任务跟踪
    std::unordered_map<std::string, std::shared_ptr<ITask>> active_tasks_;
    std::unordered_map<std::string, std::shared_ptr<TaskGroup>> active_groups_;
    mutable std::mutex task_mutex_;
    
    // 统计
    SystemStats stats_;
    mutable std::mutex stats_mutex_;
    
    // 性能监控
    std::chrono::steady_clock::time_point system_start_time_;
};

} // namespace Prisma