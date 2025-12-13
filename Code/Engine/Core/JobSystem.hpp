#pragma once
#include <deque>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>

/*
# JobStatus - lifecycle state of jobs within the JobSystem

CREATED,    // Job object instantiated but not submitted	// Main Thread, Client Code
QUEUED,     // Job added to pending queue
CLAIMED,    // Job claimed by worker thread
COMPLETED,  // Job execution finished
RETRIEVED   // Job retrieved by main thread					// Main Thread

*/

class Job;
class JobWorkerThread;
class JobSystem;

//-----------------------------------------------------------------------------------------------
enum class JobType : uint32_t
{
	GENERIC = 1 << 0, 
	FILE_IO = 1 << 1,
};

inline JobType operator|(JobType a, JobType b)
{
	return static_cast<JobType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline JobType operator&(JobType a, JobType b)
{
	return static_cast<JobType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline JobType operator~(JobType a) {
	return static_cast<JobType>(~static_cast<uint32_t>(a));
}

inline bool operator!=(JobType a, uint32_t b)
{
	return static_cast<uint32_t>(a) != b;
}

namespace JobTypeUtils
{
	inline bool HasType(JobType combined, JobType type)
	{
		return (combined & type) != static_cast<JobType>(0);
	}

	inline bool HasCommonType(JobType typeA, JobType typeB)
	{
		return (typeA & typeB) != 0;
	}

	inline bool IsEmpty(JobType type) {
		return static_cast<uint32_t>(type) == 0;
	}
}

namespace JobPriority
{
	constexpr int CRITICAL = 100;  // e.g. saving game
	constexpr int HIGH = 75;     
	constexpr int NORMAL = 50;
	constexpr int LOW = 25;     
	constexpr int TRIVIAL = 0;     
}

//-----------------------------------------------------------------------------------------------
// Job is canceled if m_priorty < Given Priority Threshold
class Job
{
public:
	explicit Job(JobType type = JobType::GENERIC, int priority = JobPriority::NORMAL) 
		: m_jobType(type)
		, m_priority(priority)
		, m_isCancelled(false)
	{}
	virtual ~Job() = default;

	virtual void Execute() = 0;

	void Cancel() { m_isCancelled.store(false); }

	bool IsCancelled() const { return m_isCancelled.load(); } // Use this in Execute()!

	JobType GetType() const { return m_jobType; }

	int GetPriority() const { return m_priority; }

protected:
	const JobType m_jobType;
	const int m_priority;
	std::atomic<bool> m_isCancelled;
};

//-----------------------------------------------------------------------------------------------
class JobWorkerThread
{

public:
	JobWorkerThread(int threadId, JobType workerType, JobSystem* jobSystem);
	~JobWorkerThread();

	int GetThreadId() const { return m_threadID; }

	JobType GetWorkerType() const { return m_workerType; }

private:
	void ThreadMain();

private:
	int m_threadID = 0; // Not used?
	JobType m_workerType;
	JobSystem* m_jobSystem = nullptr;
	std::thread* m_thread = nullptr;
};

//-----------------------------------------------------------------------------------------------
struct JobSystemConfig
{
	int numGenericWorkers = 0; // 0 means auto detect
	int numFileIOWorkers = 1; 
};

struct JobSystemStatus
{
	size_t m_pendingJobCount = 0;
	size_t m_executingJobCount = 0;
	size_t m_completedJobCount = 0;
	size_t m_totalProcessedJobCount = 0;
};


class JobSystem
{
public:
	JobSystem(JobSystemConfig const& config);
	~JobSystem() = default;

	JobSystem(const JobSystem&) = delete;
	JobSystem& operator=(const JobSystem&) = delete;
	JobSystem(JobSystem&&) = delete;
	JobSystem& operator=(JobSystem&&) = delete;

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();


	bool IsRunning() const { return m_isRunning.load(std::memory_order_acquire); }

	// ===== Main Thread Interfaces =====
	void AddJob(Job* job);

	void AddJobs(std::vector<Job*> const& jobs);

	std::vector<Job*> RetrieveCompletedJobs();

	// ===== Flush All Jobs, Before ShutDown/Restart Game, Main Thread will wait =====
	void StopAcceptingJobs() { m_acceptingJobs.store(false); }
	
	void StartAcceptingJobs() { m_acceptingJobs.store(true); }
	
	void Flush(int priorityThreshold = JobPriority::TRIVIAL);

	// ===== Worker Thread Interfaces =====
	Job* ClaimJob(JobType workerType);

	void CompleteJob(Job* job);

	void WaitForJobs();


	// ===== Static Auxiliary Functions =====

	static int GetEstimatedPCoreThreads(int totalThreads);


	// ===== Status Query Interfaces (Debug Only) =====

	size_t GetPendingJobCount() const;

	size_t GetExecutingJobCount() const;

	size_t GetCompletedJobCount() const;

	size_t GetProcessedJobCount() const;

	bool IsJobPending(Job* job) const;

	bool IsJobExecuting(Job* job) const;

	JobSystemStatus GetJobSystemStatus() const;

private:
	void CreateWorkerThreads(int count, JobType type);

	void WakeAllWorkers();

	size_t CancelJobsBelowPriority(int priorityThreshold);

private:
	JobSystemConfig m_config;
	std::atomic<bool> m_isRunning{ false }; // Important
	std::atomic<bool> m_acceptingJobs{ true }; // When flushing we do not want to add new jobs

	std::condition_variable m_jobAvailableCV; // Check when add job(s)
	std::condition_variable m_queuesEmptyCV; // After

	//mutable std::mutex m_pendingJobsMutex;
	//mutable std::mutex m_executingJobsMutex;
	//mutable std::mutex m_completedJobsMutex;

	mutable std::mutex m_jobQueueMutex; // 3 mutex or 1 mutex, will that cause performance issue?
	std::vector<Job*> m_pendingJobs;
	std::vector<Job*> m_executingJobs;
	std::vector<Job*> m_completedJobs;

	std::vector<JobWorkerThread*> m_workerThreads;

	std::atomic<size_t> m_totalJobsProcessed{ 0 }; // Only for debugging
};

