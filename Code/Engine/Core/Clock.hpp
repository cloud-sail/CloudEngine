#pragma once
#include <vector>


//-----------------------------------------------------------------------------------------------
// Hierarchical clock that inherits time scale. Parent clocks pass scaled delta seconds down to 
// child clocks to be used as their base delta seconds. Child clocks in turn scale that time and 
// pass that down to their children. There is one system clock at the root of the hierarchy.
//
class Clock
{
public:
	// Default constructor, uses the system clock as the parent of the new clock.
	Clock();

	// Constructor to specify a parent clock for the new clock.
	explicit Clock(Clock& parent);

	// Destructor, unparents ourself and our children to avoid crashes but does not otherwise try
	// to fix up the clock hierarchy. That is the responsibility of the user of this class.
	~Clock();
	Clock(Clock const& copy) = delete;

	// Reset all book keeping variables back to zero and then set the last update time
	// to be the current system time
	void	Reset();

	bool	IsPaused() const;
	void	Pause();
	void	Unpause();
	void	TogglePause();

	// Unpause for frame then pause again the next frame
	void	StepSingleFrame();

	// Set and get the value by which this clock scales delta seconds
	void	SetTimeScale(double timeScale);
	double	GetTimeScale() const;
	void	SetMinDeltaSeconds(double minDeltaSeconds);

	double	GetDeltaSeconds() const;
	double	GetTotalSeconds() const;
	double	GetFrameRate() const;
	int		GetFrameCount() const;

public:
	// Returns a reference to a static system clock that by default will be the parent of all
	// other clocks if a parent is not specified.
	static Clock& GetSystemClock();

	// Called in BeginFrame to Tick the system clock, which will in turn Advance the system
	// clock, which will in turn Advance all of its children, thus updating the entire hierarchy.
	static void TickSystemClock();

protected:
	// Calculates the current delta seconds and clamps it to the max delta time, sets the last
	// updated time, then calls Advance, passing down the delta seconds.
	void	Tick();

	// Calculates delta seconds based on pausing and time scale, updates all remaining book
	// keeping variables, call Advance on all child clocks and passes down our delta seconds,
	// and handles pausing after frames for stepping single frames.
	void	Advance(double deltaTimeSeconds);

	// Add a child clock as one of our children. Does not handle case where the child clock
	// already has a parent.
	void	AddChild(Clock* childClock);

	// Removes a child clock from our children if it is a child, otherwise does nothing.
	void	RemoveChild(Clock* childClock);

protected:
	// Parent clock. Will be nullptr for the root clock.
	Clock* m_parent = nullptr;

	// All children of this clock.
	std::vector<Clock*> m_children;

	// Book keeping variables.
	double m_lastUpdateTimeInSeconds = 0.0f;
	double m_totalSeconds = 0.0f;
	double m_deltaSeconds = 0.0f;
	int m_frameCount = 0;

	// Time scale for this clock.
	double m_timeScale = 1.0f;

	// Pauses the clock completely.
	bool m_isPaused = false;

	// For single stepping frames.
	bool m_stepSingleFrame = false;

	// Max delta time. Useful for preventing large time steps when stepping in a debugger.
	double m_maxDeltaSeconds = 0.1;

	double m_minDeltaSeconds = 0.0; // 1.0 / 120.0

protected:
	static Clock* s_systemClock;
};

// Clock pointer s_systemClock = new Clock