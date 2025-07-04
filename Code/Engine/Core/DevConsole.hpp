#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/EventSystem.hpp" // for Command_Test
#include <string>
#include <vector>
#include <map>

//-----------------------------------------------------------------------------------------------
class BitmapFont;
class Camera;
class Renderer;
class Timer;
struct AABB2;



//-----------------------------------------------------------------------------------------------
// Dev console defaults. A Render and Camera must be provided.
struct DevConsoleConfig
{
	Renderer* m_defaultRenderer = nullptr; // Renderer created first then dev console
	Camera* m_camera = nullptr;
	std::string m_fontName = "SquirrelFixedFont"; // no name extension
	float m_linesOnScreen = 40.5; // number of lines is fixed
	float m_fontAspectScale = 0.7f;
	int m_maxCommandHistory = 128;
	bool m_startOpen = false;
};

//-----------------------------------------------------------------------------------------------
// Stores the text and color for an individual line of text
struct DevConsoleLine
{
	Rgba8 m_color;
	std::string m_text;
	//int m_frameNumberPrinted;
	//double m_timePrinted;
};

//-----------------------------------------------------------------------------------------------
enum class DevConsoleMode
{
	OPEN_FULL,
	HIDDEN,
	NUM,
};

//-----------------------------------------------------------------------------------------------
struct DevConsoleCommand
{
	std::string name;
	std::vector<std::string> args;
	std::map<std::string, std::string> kwargs;
};


//-----------------------------------------------------------------------------------------------
class CircularQueue
{
public:
	CircularQueue() = default;
	CircularQueue(int capacity);

	void Resize(int capacity);
	void Enqueue(std::string const& value);

	std::string GetHistoryData(int historyIndex);
	int GetSize() const;
	bool isFull() const;
	bool IsEmpty() const;

private:
	std::vector<std::string> m_data;
	int m_start = 0;
	int m_end = 0;
	int m_capacity = 10;
	int m_count = 0;
};

//-----------------------------------------------------------------------------------------------
class DevConsole
{
public:
	DevConsole(DevConsoleConfig const& config);
	~DevConsole();

	// Subscribes to any events needed, prints an initial line of text, and starts the blink timer.
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	// Parses the current input line and executes it using the event system. Commands and arguments
	// are delimited from each other with space (' ') and argument names and values are delimited
	// with equals ('='). Echoes the command to the dev console as well as any command output.
	void Execute(std::string const& consoleCommandText, bool echoCommand = true);

	// Adds a line of text to the current list of lines being shown. Individual lines are delimited
	// with the newline ('\n') character.
	void AddText(Rgba8 const& color, std::string const& text);
	void Render(AABB2 const& bounds, Renderer* rendererOverride = nullptr) const;


	DevConsoleMode GetMode() const;
	void SetMode(DevConsoleMode mode);
	void ToggleMode(DevConsoleMode mode);

	static const Rgba8 ERROR;
	static const Rgba8 WARNING;
	static const Rgba8 INFO_MAJOR;
	static const Rgba8 INFO_MINOR;
	static const Rgba8 INFO_TEXT;
	static const Rgba8 INFO_INSERTION_POINT;
	static const Rgba8 INFO_ENTERED_TEXT;

	// Handle key input.
	static bool Event_KeyPressed(EventArgs& args);

	// Handle char input by appending valid characters to our current input line.
	static bool Event_CharInput(EventArgs& args);
	
	// Clear all lines of text
	static bool Command_Clear(EventArgs& args);

	// Display all currently registered commands in the event system.
	static bool Command_Help(EventArgs& args);

	static void ResetInsertionPointTimer();
protected:
	static bool Command_Test(EventArgs& args);

protected:
	void SaveLine(Rgba8 const& color, std::string const& lineText);
	void SaveHistoryCommandLine(std::string const& lineText);
	void Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspectScale = 1.f) const;


protected:
	DevConsoleConfig m_config;
	// only when opened it can accept input.
	DevConsoleMode m_mode = DevConsoleMode::HIDDEN;
	std::vector<DevConsoleLine> m_lines; // #ToDo: support a max limited # of lines (e.g. fixed circular buffer)
	int m_frameNumber = 0; // what? not used now

	std::string m_fontFilePathWithoutExtension;

	// Our current line of input text.
	std::string m_inputText;

	// Index of the insertion point in our current input text.
	int m_insertionPointPosition = 0;

	// True if our insertion point is currently in the visible phase of blinking.
	bool m_insertionPointVisible = true;

	// Timer for controlling insertion point visibility.
	Timer* m_insertionPointBlinkTimer = nullptr;

	// History of all commands executed.
	//std::vector<std::string> m_commandHistory;
	CircularQueue m_commandHistory;

	// Our current index in our history of commands as we are scrolling.
	int m_historyIndex = -1;
};

