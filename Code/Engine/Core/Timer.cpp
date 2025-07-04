#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"

Timer::Timer()
{
	m_clock = &Clock::GetSystemClock();
}

Timer::Timer(double period, const Clock* clock /*= nullptr*/)
	: m_clock(clock)
	, m_period(period)
{
	if (m_clock == nullptr)
	{
		m_clock = &Clock::GetSystemClock();
	}
}

void Timer::Start(double initialStartDelay /*= 0.0*/)
{
	m_startTime = m_clock->GetTotalSeconds() + initialStartDelay; // before clock tick it returns 0
}

void Timer::Stop()
{
	m_startTime = -1.0;
}

double Timer::GetElapsedTime() const
{
	if (IsStopped())
	{
		return 0.0;
	}
	return (m_clock->GetTotalSeconds() - m_startTime);
}

double Timer::GetElapsedFraction() const
{
	return (GetElapsedTime() / m_period);
}

bool Timer::IsStopped() const
{
	return (m_startTime < 0.0);
}

bool Timer::HasPeriodElapsed() const
{
	if (IsStopped())
	{
		return false;
	}

	return (GetElapsedTime() > m_period);
}

bool Timer::DecrementPeriodIfElapsed()
{
	if (IsStopped())
	{
		return false;
	}
	if (!HasPeriodElapsed())
	{
		return false;
	}
	m_startTime += m_period;
	return true;
}
