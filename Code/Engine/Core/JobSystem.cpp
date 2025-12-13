#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//-----------------------------------------------------------------------------------------------
JobWorkerThread::JobWorkerThread(int threadId, JobType workerType, JobSystem* jobSystem)
	: m_threadID(threadId)
	, m_workerType(workerType)
	, m_jobSystem(jobSystem)
{
	m_thread = new std::thread(&JobWorkerThread::ThreadMain, this);
}

JobWorkerThread::~JobWorkerThread()
{
	if (m_thread && m_thread->joinable())
	{
		m_thread->join();
	}
	delete m_thread;
	m_thread = nullptr;
}

void JobWorkerThread::ThreadMain()
{
	//DebuggerPrintf("Worker Thread %d starts, Job Type: %u\n", m_threadID, static_cast<uint32_t>(m_workerType));
	while (m_jobSystem->IsRunning())
	{
		Job* job = m_jobSystem->ClaimJob(m_workerType);

		if (job)
		{
			job->Execute();
			m_jobSystem->CompleteJob(job);
		}
		else
		{
			// should double check IsRunning?
			m_jobSystem->WaitForJobs();
		}
	}
	//DebuggerPrintf("Worker Thread %d ends, Job Type: %u\n", m_threadID, static_cast<uint32_t>(m_workerType));
}

//-----------------------------------------------------------------------------------------------
JobSystem::JobSystem(JobSystemConfig const& config)
	: m_config(config)
{
}

void JobSystem::Startup()
{
	bool expected = false;
	if (!m_isRunning.compare_exchange_strong(expected, true))
	{
		ERROR_RECOVERABLE("JobSystem has been started up.");
		return;
	}

	m_pendingJobs.reserve(100);
	m_executingJobs.reserve(20);
	m_completedJobs.reserve(100);


	int genericWorkers = m_config.numGenericWorkers;
	if (genericWorkers <= 0)
	{
		// Auto Detect
		int totalThreads = std::thread::hardware_concurrency();
		if (totalThreads <= 0)
		{
			genericWorkers = 4;
		}
		else
		{
			genericWorkers = GetEstimatedPCoreThreads(totalThreads);
		}
	}

	// Add more types of Worker Threads here
	CreateWorkerThreads(genericWorkers, JobType::GENERIC);
	CreateWorkerThreads(m_config.numFileIOWorkers, JobType::FILE_IO);
}

void JobSystem::Shutdown()
{
	bool expected = true;
	if (!m_isRunning.compare_exchange_strong(expected, false))
	{
		return;
	}

	WakeAllWorkers();

	for (int i = 0; i < (int)m_workerThreads.size(); ++i)
	{
		delete m_workerThreads[i];
		m_workerThreads[i] = nullptr;
	}
	m_workerThreads.clear();

	// Clear all Job Queues, and warn the user about remaining jobs
	{
		std::scoped_lock lock(m_jobQueueMutex);

		if (!m_pendingJobs.empty())
		{
			DebuggerPrintf("[JobSystem] WARNING: %d pending jobs left\n", m_pendingJobs.size());
		}

		if (!m_executingJobs.empty())
		{
			DebuggerPrintf("[JobSystem] WARNING: %d executing jobs left\n", m_executingJobs.size());
		}

		if (!m_completedJobs.empty())
		{
			DebuggerPrintf("[JobSystem] WARNING: %d completed jobs left\n", m_completedJobs.size());
		}

		m_pendingJobs.clear();
		m_executingJobs.clear();
		m_completedJobs.clear();
	}

}

void JobSystem::BeginFrame()
{

}

void JobSystem::EndFrame()
{

}

void JobSystem::AddJob(Job* job)
{
	if (job == nullptr || !IsRunning()) return;

	if (!m_acceptingJobs.load())
	{
		ERROR_RECOVERABLE("The job system is currently not accepting jobs.");
		return;
	}

	{
		std::scoped_lock lock(m_jobQueueMutex);
		m_pendingJobs.push_back(job);
	}

	m_jobAvailableCV.notify_one();
}

void JobSystem::AddJobs(std::vector<Job*> const& jobs)
{
	if (jobs.empty() || !IsRunning()) return;

	if (!m_acceptingJobs.load())
	{
		ERROR_RECOVERABLE("The job system is currently not accepting jobs.");
		return;
	}

	{
		std::scoped_lock lock(m_jobQueueMutex);
		m_pendingJobs.insert(m_pendingJobs.end(), jobs.begin(), jobs.end());
	}

	m_jobAvailableCV.notify_all();
}

std::vector<Job*> JobSystem::RetrieveCompletedJobs()
{
	std::scoped_lock lock(m_jobQueueMutex);
	// Swap internal pointer, not copying
	std::vector<Job*> result;
	result.swap(m_completedJobs);
	return result;
}


void JobSystem::Flush(int priorityThreshold /*= JobPriority::TRIVIAL*/)
{
	StopAcceptingJobs();
	// Only main thread push new jobs, no need to stop accepting jobs (it may be needed in future)
	size_t cancelledCount = CancelJobsBelowPriority(priorityThreshold);
	if (cancelledCount > 0)
	{
		DebuggerPrintf("[JobSystem] Log: %d jobs with lower priority have been cancelled\n", cancelledCount);
	}

	// Use Condition Variable / sleep_for / yield to wait
	std::unique_lock lock(m_jobQueueMutex);
	m_queuesEmptyCV.wait(lock, [this]() {return m_pendingJobs.empty() && m_executingJobs.empty(); });

}

Job* JobSystem::ClaimJob(JobType workerType)
{
	std::scoped_lock lock(m_jobQueueMutex);

	for (auto it = m_pendingJobs.begin(); it != m_pendingJobs.end(); ++it)
	{
		Job* job = *it;


		if (JobTypeUtils::HasCommonType(job->GetType(),workerType))
		{
			m_pendingJobs.erase(it);
			m_executingJobs.push_back(job);
			return job;
		}
	}

	// no matching job to claim
	return nullptr;
}

void JobSystem::CompleteJob(Job* job)
{
	if (job == nullptr) return;

	std::scoped_lock lock(m_jobQueueMutex);

	auto it = std::find(m_executingJobs.begin(), m_executingJobs.end(), job);
	if (it != m_executingJobs.end())
	{
		m_executingJobs.erase(it);

		m_completedJobs.push_back(job);

		m_totalJobsProcessed.fetch_add(1, std::memory_order_relaxed);

		if (m_pendingJobs.empty() && m_executingJobs.empty())
		{
			m_queuesEmptyCV.notify_all();
		}
	}
	else
	{
		ERROR_RECOVERABLE("Trying to complete a job which is not in the executing job list!");
	}
}

void JobSystem::WaitForJobs()
{
	std::unique_lock lock(m_jobQueueMutex);

	m_jobAvailableCV.wait(lock, [this]() {
		return !m_pendingJobs.empty() || !m_isRunning.load(std::memory_order_acquire);
		});
}

size_t JobSystem::GetPendingJobCount() const
{
	std::scoped_lock lock(m_jobQueueMutex);
	return m_pendingJobs.size();
}

size_t JobSystem::GetExecutingJobCount() const
{
	std::scoped_lock lock(m_jobQueueMutex);
	return m_executingJobs.size();
}

size_t JobSystem::GetCompletedJobCount() const
{
	std::scoped_lock lock(m_jobQueueMutex);
	return m_completedJobs.size();
}

size_t JobSystem::GetProcessedJobCount() const
{
	return m_totalJobsProcessed.load(std::memory_order_relaxed);
}

bool JobSystem::IsJobPending(Job* job) const
{
	std::scoped_lock lock(m_jobQueueMutex);
	return std::find(m_pendingJobs.begin(), m_pendingJobs.end(), job)
		!= m_pendingJobs.end();
}

bool JobSystem::IsJobExecuting(Job* job) const
{
	std::scoped_lock lock(m_jobQueueMutex);
	return std::find(m_executingJobs.begin(), m_executingJobs.end(), job)
		!= m_executingJobs.end();
}

JobSystemStatus JobSystem::GetJobSystemStatus() const
{
	std::scoped_lock lock(m_jobQueueMutex);
	JobSystemStatus status;
	status.m_pendingJobCount = m_pendingJobs.size();
	status.m_executingJobCount = m_executingJobs.size();
	status.m_completedJobCount = m_completedJobs.size();
	status.m_totalProcessedJobCount = m_totalJobsProcessed.load();

	return status;
}

int JobSystem::GetEstimatedPCoreThreads(int totalThreads)
{
	if (totalThreads <= 8) {
		return totalThreads;
	}
	else if (totalThreads <= 16) {
		return 12;
	}
	else if (totalThreads <= 20) {
		return 12;
	}
	else if (totalThreads <= 24) {
		return 16;
	}
	else if (totalThreads <= 32) {
		return 16;
	}
	else
	{
		return totalThreads / 2;
	}
}

void JobSystem::CreateWorkerThreads(int count, JobType type)
{
	static int nextThreadID = 0;
	for (int i = 0; i < count; ++i)
	{
		JobWorkerThread* worker = new JobWorkerThread(nextThreadID, type, this);
		m_workerThreads.push_back(worker);
		nextThreadID++;
	}
}

void JobSystem::WakeAllWorkers()
{
	m_jobAvailableCV.notify_all();
}

size_t JobSystem::CancelJobsBelowPriority(int priorityThreshold)
{
	std::scoped_lock lock(m_jobQueueMutex);

	size_t cancelledCount = 0;

	// Pending list, move canceled jobs to completed list
	auto it = m_pendingJobs.begin();
	while (it != m_pendingJobs.end())
	{
		Job* job = *it;
		if (job && job->GetPriority() < priorityThreshold)
		{
			job->Cancel();
			++cancelledCount;

			m_completedJobs.push_back(job);

			it = m_pendingJobs.erase(it);

		}
		else
		{
			++it;
		}
	}

	// Executing list
	for (Job* job : m_executingJobs)
	{
		if (job && job->GetPriority() < priorityThreshold)
		{
			job->Cancel();
			++cancelledCount;
		}
	}

	// Notify Waited
	if (m_pendingJobs.empty() && m_executingJobs.empty())
	{
		m_queuesEmptyCV.notify_all();
	}

	return cancelledCount;
}

