#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"
#include <thread>

#define STATIC

Clock* Clock::s_systemClock = new Clock();

Clock::Clock()
{
	if (s_systemClock)
	{
		m_parent = s_systemClock;
		m_parent->AddChild(this);
	}
	m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
}

Clock::Clock(Clock& parent)
{
	m_parent = &parent;
	m_parent->AddChild(this);
	m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
}


Clock::~Clock()
{
	if (m_parent != nullptr)
	{
		m_parent->RemoveChild(this);
	}
	m_parent = nullptr;
	m_children.clear();
}

void Clock::Reset()
{
	m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
	m_totalSeconds = 0.0f;
	m_deltaSeconds = 0.0f;
}

bool Clock::IsPaused() const
{
	return m_isPaused;
}

void Clock::Pause()
{
	m_isPaused = true;
}

void Clock::Unpause()
{
	m_isPaused = false;
}

void Clock::TogglePause()
{
	m_isPaused = !m_isPaused;
}

void Clock::StepSingleFrame()
{
	m_isPaused = false;
	m_stepSingleFrame = true;
}

void Clock::SetTimeScale(double timeScale)
{
	m_timeScale = timeScale;
}

double Clock::GetTimeScale() const
{
	return m_timeScale;
}

void Clock::SetMinDeltaSeconds(double minDeltaSeconds)
{
	m_minDeltaSeconds = minDeltaSeconds;
}

double Clock::GetDeltaSeconds() const
{
	return m_deltaSeconds;
}

double Clock::GetTotalSeconds() const
{
	return m_totalSeconds;
}

double Clock::GetFrameRate() const
{
	if (m_deltaSeconds == 0.0)
	{
		return 0.0;
	}
	return 1.0 / m_deltaSeconds;
}

int Clock::GetFrameCount() const
{
	return m_frameCount;
}

STATIC Clock& Clock::GetSystemClock()
{
	return *s_systemClock;
}

STATIC void Clock::TickSystemClock()
{
	s_systemClock->Tick();
}

void Clock::Tick()
{
	double currentTime = GetCurrentTimeSeconds();
	double deltaSeconds = currentTime - m_lastUpdateTimeInSeconds;

	while (deltaSeconds < m_minDeltaSeconds)
	{
		std::this_thread::yield();
		currentTime = GetCurrentTimeSeconds();
		deltaSeconds = currentTime - m_lastUpdateTimeInSeconds;
	}


	deltaSeconds = (deltaSeconds > m_maxDeltaSeconds) ? m_maxDeltaSeconds : deltaSeconds;
	
	m_lastUpdateTimeInSeconds = currentTime;
	Advance(deltaSeconds);
}

void Clock::Advance(double deltaTimeSeconds)
{
	m_deltaSeconds = deltaTimeSeconds;

	m_deltaSeconds *= m_timeScale;
	if (m_isPaused)
	{
		m_deltaSeconds = 0.f;
	}

	m_totalSeconds += m_deltaSeconds;
	m_frameCount++;

	for (int childIndex = 0; childIndex < m_children.size(); childIndex++)
	{
		if (m_children[childIndex] != nullptr)
		{
			m_children[childIndex]->Advance(m_deltaSeconds);
		}
	}

	if (m_stepSingleFrame)
	{
		m_isPaused = true;
		m_stepSingleFrame = false;
	}
}

void Clock::AddChild(Clock* childClock)
{
	m_children.push_back(childClock);
}

void Clock::RemoveChild(Clock* childClock)
{
	for (int childIndex = 0; childIndex < m_children.size(); childIndex++)
	{
		if (m_children[childIndex] == childClock)
		{
			m_children[childIndex] = nullptr;
		}
	}
}
