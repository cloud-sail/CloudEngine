#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Window/Window.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> 

//-----------------------------------------------------------------------------------------------
InputSystem* g_theInput = nullptr;

//-----------------------------------------------------------------------------------------------
#pragma region KeyCode
unsigned char const KEYCODE_ESCAPE = VK_ESCAPE;
unsigned char const KEYCODE_SPACE = VK_SPACE;
unsigned char const KEYCODE_ENTER = VK_RETURN;

unsigned char const KEYCODE_LEFT_MOUSE		= VK_LBUTTON;
unsigned char const KEYCODE_RIGHT_MOUSE		= VK_RBUTTON;

unsigned char const KEYCODE_TILDE			= VK_OEM_3; // '`~' for US
unsigned char const KEYCODE_LEFTBRACKET		= VK_OEM_4; //  '[{' for US
unsigned char const KEYCODE_RIGHTBRACKET	= VK_OEM_6; //  ']}' for US

unsigned char const KEYCODE_SHIFT = VK_SHIFT;
unsigned char const KEYCODE_CONTROL = VK_CONTROL;
unsigned char const KEYCODE_ALT = VK_MENU;
unsigned char const KEYCODE_BACKSPACE = VK_BACK;
unsigned char const KEYCODE_INSERT = VK_INSERT;
unsigned char const KEYCODE_DELETE = VK_DELETE;
unsigned char const KEYCODE_HOME = VK_HOME;
unsigned char const KEYCODE_END = VK_END;

// Function keys
unsigned char const KEYCODE_F1 = VK_F1;
unsigned char const KEYCODE_F2 = VK_F2;
unsigned char const KEYCODE_F3 = VK_F3;
unsigned char const KEYCODE_F4 = VK_F4;
unsigned char const KEYCODE_F5 = VK_F5;
unsigned char const KEYCODE_F6 = VK_F6;
unsigned char const KEYCODE_F7 = VK_F7;
unsigned char const KEYCODE_F8 = VK_F8;
unsigned char const KEYCODE_F9 = VK_F9;
unsigned char const KEYCODE_F10 = VK_F10;
unsigned char const KEYCODE_F11 = VK_F11;
unsigned char const KEYCODE_F12 = VK_F12;

// Alphabet keys
unsigned char const KEYCODE_A = 'A';
unsigned char const KEYCODE_B = 'B';
unsigned char const KEYCODE_C = 'C';
unsigned char const KEYCODE_D = 'D';
unsigned char const KEYCODE_E = 'E';
unsigned char const KEYCODE_F = 'F';
unsigned char const KEYCODE_G = 'G';
unsigned char const KEYCODE_H = 'H';
unsigned char const KEYCODE_I = 'I';
unsigned char const KEYCODE_J = 'J';
unsigned char const KEYCODE_K = 'K';
unsigned char const KEYCODE_L = 'L';
unsigned char const KEYCODE_M = 'M';
unsigned char const KEYCODE_N = 'N';
unsigned char const KEYCODE_O = 'O';
unsigned char const KEYCODE_P = 'P';
unsigned char const KEYCODE_Q = 'Q';
unsigned char const KEYCODE_R = 'R';
unsigned char const KEYCODE_S = 'S';
unsigned char const KEYCODE_T = 'T';
unsigned char const KEYCODE_U = 'U';
unsigned char const KEYCODE_V = 'V';
unsigned char const KEYCODE_W = 'W';
unsigned char const KEYCODE_X = 'X';
unsigned char const KEYCODE_Y = 'Y';
unsigned char const KEYCODE_Z = 'Z';

// Arrow keys
unsigned char const KEYCODE_LEFT	= VK_LEFT;
unsigned char const KEYCODE_UP		= VK_UP;
unsigned char const KEYCODE_RIGHT	= VK_RIGHT;
unsigned char const KEYCODE_DOWN	= VK_DOWN;

#pragma endregion KeyCode

InputSystem::InputSystem(InputConfig const& config)
	: m_config(config)
{
}

void InputSystem::Startup()
{
	SubscribeEventCallbackFunction("KeyPressed", InputSystem::Event_KeyPressed);
	SubscribeEventCallbackFunction("KeyReleased", InputSystem::Event_KeyReleased);

	for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
	{
		m_controllers[i].m_id = i;
		//m_controllers[i].m_leftStick.SetDeadZoneThresholds(0.3f, 0.95f);
		//m_controllers[i].m_rightStick.SetDeadZoneThresholds(0.3f, 0.95f);
	}
}

void InputSystem::BeginFrame()
{
	// Tick controllers
	for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
	{
		m_controllers[i].Update();
	}


	CURSORINFO cursorInfo = { 0 };
	cursorInfo.cbSize = sizeof(cursorInfo);
	if (!::GetCursorInfo(&cursorInfo))
	{
		ERROR_RECOVERABLE("Fail to get cursor info!");
	}

	bool isCursorVisible = cursorInfo.flags & CURSOR_SHOWING;

	if (isCursorVisible != (m_cursorState.m_cursorMode == CursorMode::POINTER))
	{
		if (m_cursorState.m_cursorMode == CursorMode::FPS)
		{
			while (::ShowCursor(false) >= 0) {}
		}

		if (m_cursorState.m_cursorMode == CursorMode::POINTER)
		{
			while (::ShowCursor(true) < 0) {}
		}
	}


		
	IntVec2 previousCursorClientPosition = m_cursorState.m_cursorClientPosition;
	SaveCurrentCursorClientPosition();

	if (m_cursorState.m_cursorMode == CursorMode::FPS)
	{
		m_cursorState.m_cursorClientDelta = m_cursorState.m_cursorClientPosition - previousCursorClientPosition;

		// Center the mouse
		HWND windowHandle = static_cast<HWND>(Window::s_mainWindow->GetHwnd());
		RECT clientRect;
		::GetClientRect(windowHandle, &clientRect); // left and top are zero according to documents
		POINT clientCenter;
		clientCenter.x = clientRect.right / 2;
		clientCenter.y = clientRect.bottom / 2;
		::ClientToScreen(windowHandle, &clientCenter);
		::SetCursorPos(clientCenter.x, clientCenter.y);

		SaveCurrentCursorClientPosition();
	}
	else
	{
		m_cursorState.m_cursorClientDelta = IntVec2();
	}
	
}

void InputSystem::EndFrame()
{
	for (int i = 0; i < NUM_KEYCODES; ++i)
	{
		m_keyStates[i].m_wasPressedLastFrame = m_keyStates[i].m_isPressed;
	}
}

void InputSystem::Shutdown()
{
}

bool InputSystem::WasKeyJustPressed(unsigned char keyCode) const
{
	return !m_keyStates[keyCode].m_wasPressedLastFrame &&
			m_keyStates[keyCode].m_isPressed;
}

bool InputSystem::WasKeyJustReleased(unsigned char keyCode) const
{
	return m_keyStates[keyCode].m_wasPressedLastFrame &&
		  !m_keyStates[keyCode].m_isPressed;
}

bool InputSystem::IsKeyDown(unsigned char keyCode) const
{
	return m_keyStates[keyCode].m_isPressed;
}

void InputSystem::HandleKeyPressed(unsigned char keyCode)
{	
	m_keyStates[keyCode].m_isPressed = true;
}

void InputSystem::HandleKeyReleased(unsigned char keyCode)
{
	m_keyStates[keyCode].m_isPressed = false;
}

XboxController const& InputSystem::GetController(int controllerID)
{
	return m_controllers[controllerID];
}

void InputSystem::SetCursorMode(CursorMode cursorMode)
{
	m_cursorState.m_cursorMode = cursorMode;
}

Vec2 InputSystem::GetCursorClientDelta() const
{
	return Vec2(m_cursorState.m_cursorClientDelta);
}

Vec2 InputSystem::GetCursorClientPosition() const
{
	return Vec2(m_cursorState.m_cursorClientPosition);
}

Vec2 InputSystem::GetCursorNormalizedPosition() const
{
	//-----------------------------------------------------------------------------------------------
	// More accurate way, but it needs to use Windows function
	//-----------------------------------------------------------------------------------------------
	//HWND windowHandle = static_cast<HWND>(Window::s_mainWindow->GetHwnd());
	//POINT cursorCoords;
	//RECT clientRect;
	//::GetCursorPos(&cursorCoords); // in Windows screen coordinates; (0,0) is top-left
	//::ScreenToClient(windowHandle, &cursorCoords); // get relative to this window's client area
	//::GetClientRect(windowHandle, &clientRect); // dimensions of client area (0,0 to width,height)
	//float cursorX = static_cast<float>(cursorCoords.x) / static_cast<float>(clientRect.right);
	//float cursorY = static_cast<float>(cursorCoords.y) / static_cast<float>(clientRect.bottom);
	//return Vec2(cursorX, 1.f - cursorY);
	//-----------------------------------------------------------------------------------------------

	IntVec2 clientDimensions = Window::s_mainWindow->GetClientDimensions();

	float cursorX = static_cast<float>(m_cursorState.m_cursorClientPosition.x) / static_cast<float>(clientDimensions.x);
	float cursorY = static_cast<float>(m_cursorState.m_cursorClientPosition.y) / static_cast<float>(clientDimensions.y);
	return Vec2(cursorX, 1.f - cursorY);
}

void InputSystem::SaveCurrentCursorClientPosition()
{
	HWND windowHandle = static_cast<HWND>(Window::s_mainWindow->GetHwnd());
	POINT cursorCoords;
	::GetCursorPos(&cursorCoords);
	::ScreenToClient(windowHandle, &cursorCoords);
	m_cursorState.m_cursorClientPosition.x = cursorCoords.x;
	m_cursorState.m_cursorClientPosition.y = cursorCoords.y;
}

STATIC bool InputSystem::Event_KeyPressed(EventArgs& args)
{
	if (!g_theInput)
	{
		return false;
	}
	unsigned char keyCode = (unsigned char)args.GetValue("KeyCode", -1);
	g_theInput->HandleKeyPressed(keyCode);
	return true;
}

bool InputSystem::Event_KeyReleased(EventArgs& args)
{
	if (!g_theInput)
	{
		return false;
	}
	unsigned char keyCode = (unsigned char)args.GetValue("KeyCode", -1);
	g_theInput->HandleKeyReleased(keyCode);
	return true;
}


//-----------------------------------------------------------------------------------------------
//Vec2 MapMouseCursorToWorldCoords2D(AABB2 const& cameraBounds, AABB2 const& viewport)
//{
//	Vec2 clientUV = g_theWindow->GetNormalizedMouseUV();
//	Vec2 adjustUV = viewport.GetUVForPoint(clientUV);
//	return cameraBounds.GetPointAtUV(adjustUV);
//}