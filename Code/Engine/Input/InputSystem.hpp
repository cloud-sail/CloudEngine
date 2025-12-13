#pragma once
#include "Engine/Input/KeyButtonState.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/IntVec2.hpp"

//-----------------------------------------------------------------------------------------------
#pragma region KeyCode
extern unsigned char const KEYCODE_ESCAPE;
extern unsigned char const KEYCODE_SPACE;
extern unsigned char const KEYCODE_ENTER;

extern unsigned char const KEYCODE_LEFT_MOUSE;
extern unsigned char const KEYCODE_RIGHT_MOUSE;
extern unsigned char const KEYCODE_MIDDLE_MOUSE;

extern unsigned char const KEYCODE_TILDE;
extern unsigned char const KEYCODE_LEFTBRACKET;
extern unsigned char const KEYCODE_RIGHTBRACKET;

extern unsigned char const KEYCODE_SHIFT;
extern unsigned char const KEYCODE_CONTROL;
extern unsigned char const KEYCODE_ALT;
extern unsigned char const KEYCODE_BACKSPACE;
extern unsigned char const KEYCODE_INSERT;
extern unsigned char const KEYCODE_DELETE;
extern unsigned char const KEYCODE_HOME;
extern unsigned char const KEYCODE_END;


// Function keys
extern unsigned char const KEYCODE_F1;
extern unsigned char const KEYCODE_F2;
extern unsigned char const KEYCODE_F3;
extern unsigned char const KEYCODE_F4;
extern unsigned char const KEYCODE_F5;
extern unsigned char const KEYCODE_F6;
extern unsigned char const KEYCODE_F7;
extern unsigned char const KEYCODE_F8;
extern unsigned char const KEYCODE_F9;
extern unsigned char const KEYCODE_F10;
extern unsigned char const KEYCODE_F11;
extern unsigned char const KEYCODE_F12;

// Alphabet keys
extern unsigned char const KEYCODE_A;
extern unsigned char const KEYCODE_B;
extern unsigned char const KEYCODE_C;
extern unsigned char const KEYCODE_D;
extern unsigned char const KEYCODE_E;
extern unsigned char const KEYCODE_F;
extern unsigned char const KEYCODE_G;
extern unsigned char const KEYCODE_H;
extern unsigned char const KEYCODE_I;
extern unsigned char const KEYCODE_J;
extern unsigned char const KEYCODE_K;
extern unsigned char const KEYCODE_L;
extern unsigned char const KEYCODE_M;
extern unsigned char const KEYCODE_N;
extern unsigned char const KEYCODE_O;
extern unsigned char const KEYCODE_P;
extern unsigned char const KEYCODE_Q;
extern unsigned char const KEYCODE_R;
extern unsigned char const KEYCODE_S;
extern unsigned char const KEYCODE_T;
extern unsigned char const KEYCODE_U;
extern unsigned char const KEYCODE_V;
extern unsigned char const KEYCODE_W;
extern unsigned char const KEYCODE_X;
extern unsigned char const KEYCODE_Y;
extern unsigned char const KEYCODE_Z;

// Arrow keys
extern unsigned char const KEYCODE_LEFT;
extern unsigned char const KEYCODE_UP;
extern unsigned char const KEYCODE_RIGHT;
extern unsigned char const KEYCODE_DOWN;
#pragma endregion KeyCode


constexpr int NUM_KEYCODES = 256;
constexpr int NUM_XBOX_CONTROLLERS = 4;

//-----------------------------------------------------------------------------------------------
class Window;

//-----------------------------------------------------------------------------------------------
enum class CursorMode
{
	POINTER,
	FPS,
};

//-----------------------------------------------------------------------------------------------
struct CursorState
{
	IntVec2 m_cursorClientDelta;
	IntVec2 m_cursorClientPosition;

	CursorMode m_cursorMode = CursorMode::POINTER;
};


//-----------------------------------------------------------------------------------------------
struct InputConfig
{
	Window* m_window = nullptr; // not used now, only one window use Window::s_mainWindow
};


//-----------------------------------------------------------------------------------------------
class InputSystem
{
public:
	InputSystem(InputConfig const& config);
	~InputSystem() = default;
	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	// Input detection for device switching
	bool HasAnyKeyboardMouseInput() const;
	bool HasMouseMoved() const;
	bool HasAnyControllerInput(int controllerID) const;

	bool WasKeyJustPressed(unsigned char keyCode) const;
	bool WasKeyJustReleased(unsigned char keyCode) const;
	bool IsKeyDown(unsigned char keyCode) const;

	void HandleKeyPressed(unsigned char keyCode);
	void HandleKeyReleased(unsigned char keyCode);

	XboxController const& GetController(int controllerID);

	// In pointer mode, the cursor should be visible, freely able to move, and not
	// locked to the window. In FPS mode, the cursor should be hidden, reset to the
	// center of the window each frame, and record the delta each frame.
	void SetCursorMode(CursorMode cursorMode);

	// Returns the current frame cursor delta in pixel, relative to the client
	// region. This is how much the cursor moved last frame before it was reset
	// to the center of the screen. Only valid in FPS mode, will be zero otherwise.
	Vec2 GetCursorClientDelta() const;

	// Returns the cursor position, in pixels relative to the client region.
	Vec2 GetCursorClientPosition() const;

	// Returns the cusor position, normalized to the range [0, 1], relative
	// to the client region, with the y-axis inverted to map from Windows
	// conventions to game screen camera conventions
	Vec2 GetCursorNormalizedPosition() const;

	void SaveCurrentCursorClientPosition();

protected:
	bool IsGameplayKey(unsigned char keyCode) const;

	static bool Event_KeyPressed(EventArgs& args);
	static bool Event_KeyReleased(EventArgs& args);

protected:
	KeyButtonState m_keyStates[NUM_KEYCODES];
	XboxController m_controllers[NUM_XBOX_CONTROLLERS];

	InputConfig m_config;

	CursorState m_cursorState;
};


//-----------------------------------------------------------------------------------------------
/*
// Example 1: Detect Input Device Switch
void Game::DetectInputDevice()
{
	bool hasKeyboardMouse = g_theInput->HasAnyKeyboardMouseInput() ||
							g_theInput->HasMouseMoved();

	bool hasGamepad = g_theInput->HasAnyControllerInput(0);

	if (hasKeyboardMouse)
	{
		// Switch to KeyBoard&Mouse UI (WASD, Mouse Icon)
		m_currentDevice = KEYBOARD_MOUSE;
	}
	else if (hasGamepad)
	{
		// Switch to Gamepad UI (Show A/B/X/Y Icon)
		m_currentDevice = GAMEPAD;
	}
}

// Example 2: Only pause the timer when player has input
void Game::UpdateIdleTimer(float deltaSeconds)
{
	bool hasInput = g_theInput->HasAnyKeyboardMouseInput() ||
					g_theInput->HasMouseMoved() ||
					g_theInput->HasAnyControllerInput(0);

	if (hasInput)
	{
		m_idleTimer = 0.0f;
	}
	else
	{
		m_idleTimer += deltaSeconds;
		if (m_idleTimer > 300.0f) // 5 mins no input operation
		{
			ShowScreensaver();
		}
	}
}


```cpp
class Game
{
private:
	enum class InputDevice
	{
		KEYBOARD_MOUSE,
		GAMEPAD
	};

	InputDevice m_currentInputDevice = InputDevice::KEYBOARD_MOUSE;

public:
	void Update(float deltaSeconds)
	{
		DetectInputDevice();

		UpdateUIForCurrentDevice();
	}

	void DetectInputDevice()
	{
		bool hasKeyboardMouseInput = g_theInput->HasAnyKeyboardMouseInput() ||
									 g_theInput->HasMouseMoved();

		bool hasGamepadInput = false;
		for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
		{
			if (g_theInput->HasAnyControllerInput(i))
			{
				hasGamepadInput = true;
				break;
			}
		}

		if (hasKeyboardMouseInput && m_currentInputDevice != InputDevice::KEYBOARD_MOUSE)
		{
			m_currentInputDevice = InputDevice::KEYBOARD_MOUSE;
			OnInputDeviceChanged();
		}
		else if (hasGamepadInput && m_currentInputDevice != InputDevice::GAMEPAD)
		{
			m_currentInputDevice = InputDevice::GAMEPAD;
			OnInputDeviceChanged();
		}
	}

	void OnInputDeviceChanged()
	{

		if (m_currentInputDevice == InputDevice::KEYBOARD_MOUSE)
		{
			// [W][A][S][D], [LMB], etc.
			DebuggerPrintf("Switched to Keyboard & Mouse\n");
		}
		else
		{
			// [A], [B], [X], [Y], etc.
			DebuggerPrintf("Switched to Gamepad\n");
		}
	}

	void UpdateUIForCurrentDevice()
	{
		if (m_currentInputDevice == InputDevice::KEYBOARD_MOUSE)
		{
			// "Press [E] to interact"
		}
		else
		{
			// "Press [A] to interact"
		}
	}
};
*/
