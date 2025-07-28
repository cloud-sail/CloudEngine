#include "Engine/Window/Window.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/Vec2.hpp"
#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)

#if defined(OPAQUE)
#undef OPAQUE
#endif

#include "ThirdParty/imgui/imgui_impl_win32.h"


#include "Engine/Renderer/Renderer.hpp"
//-----------------------------------------------------------------------------------------------
Window* Window::s_mainWindow = nullptr;

//-----------------------------------------------------------------------------------------------
Window::Window(WindowConfig const& config)
	: m_config(config)
{
	s_mainWindow = this;
}

void Window::Startup()
{
	CreateOSWindow();
}

void Window::BeginFrame()
{
	RunMessagePump();
}

void Window::EndFrame()
{
}

void Window::Shutdown()
{
}

//-----------------------------------------------------------------------------------------------
// This method has been moved to InputSystem
// Returns the mouse cursor's current position relative to the interior client area of our window,
// in normalized UV coordinates -- (0,0) bottom-left, (1,1) top-right
//Vec2 Window::GetNormalizedMouseUV() const
//{
//	HWND windowHandle = static_cast<HWND>(m_windowHandle);
//	POINT cursorCoords;
//	RECT clientRect;
//	::GetCursorPos(&cursorCoords); // in Windows screen coordinates; (0,0) is top-left
//	::ScreenToClient(windowHandle, &cursorCoords); // get relative to this window's client area
//	::GetClientRect(windowHandle, &clientRect); // dimensions of client area (0,0 to width,height)
//	float cursorX = static_cast<float>(cursorCoords.x) / static_cast<float>(clientRect.right);
//	float cursorY = static_cast<float>(cursorCoords.y) / static_cast<float>(clientRect.bottom);
//	return Vec2(cursorX, 1.f - cursorY);
//}

void* Window::GetHwnd() const
{
	return static_cast<void*>(m_windowHandle);
}

IntVec2 Window::GetClientDimensions() const
{
	return m_clientDimensions;
}

bool Window::IsFocused() const
{
	return (::GetActiveWindow() == m_windowHandle);
}

float Window::GetAspectRatio() const
{
	return (float)m_clientDimensions.x / (float)m_clientDimensions.y;
}

WindowConfig const& Window::GetConfig() const
{
	return m_config;
}

void* Window::GetDisplayContext() const
{
	return m_displayContext;
}

//-----------------------------------------------------------------------------------------------
// Handles Windows (Win32) messages/events; i.e. the OS is trying to tell us something happened.
// This function is called back by Windows whenever we tell it to (by calling DispatchMessage).

LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam)
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	if (ImGui_ImplWin32_WndProcHandler(windowHandle, wmMessageCode, wParam, lParam))
		return true;

	if (ImGui::GetCurrentContext() != nullptr && Window::s_mainWindow != nullptr)
	{
		ImGuiIO& io = ImGui::GetIO();

		WindowConfig const& config = Window::s_mainWindow->GetConfig();
		bool blockKeyboard = config.m_imGuiCaptureKeyBoardInput && io.WantCaptureKeyboard;
		bool blockMouse = config.m_imGuiCaptureMouseInput && io.WantCaptureMouse;

		if ((blockKeyboard && (wmMessageCode >= WM_KEYFIRST && wmMessageCode <= WM_KEYLAST)) ||
			(blockMouse && (wmMessageCode >= WM_MOUSEFIRST && wmMessageCode <= WM_MOUSELAST)))
		{
			return true;
		}
	}


	switch (wmMessageCode)
	{
		// App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
		case WM_CLOSE:
		{
			FireEvent("Quit");
			return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		}
		case WM_CHAR:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", (unsigned char)wParam));
			FireEvent("CharInput", args);
			return 0;
		}
		// Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
		case WM_KEYDOWN:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", (unsigned char)wParam));
			FireEvent("KeyPressed", args);
			return 0;
		}
		// Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
		case WM_KEYUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", (unsigned char)wParam));
			FireEvent("KeyReleased", args);
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", KEYCODE_LEFT_MOUSE));
			FireEvent("KeyPressed", args);
			return 0;
		}
		case WM_LBUTTONUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", KEYCODE_LEFT_MOUSE));
			FireEvent("KeyReleased", args);
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", KEYCODE_RIGHT_MOUSE));
			FireEvent("KeyPressed", args);
			return 0;
		}
		case WM_RBUTTONUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", KEYCODE_RIGHT_MOUSE));
			FireEvent("KeyReleased", args);
			return 0;
		}

		case WM_SIZE:
		{
			if (Window::s_mainWindow)
			{
				Window* window = Window::s_mainWindow;
				IntVec2 newDimensions = IntVec2(LOWORD(lParam), HIWORD(lParam));
				window->UpdateClientDimensions(newDimensions);

				if (wParam == SIZE_MINIMIZED)
				{
					window->m_isMinimized = true;
					window->m_isMaximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED)
				{
					window->m_isMinimized = false;
					window->m_isMaximized = true;
					if(Renderer::s_mainRenderer) Renderer::s_mainRenderer->OnResize();
				}
				else if (wParam == SIZE_RESTORED)
				{

					// Restoring from minimized state?
					if (window->m_isMinimized)
					{
						window->m_isMinimized = false;
						if (Renderer::s_mainRenderer) Renderer::s_mainRenderer->OnResize();
					}

					// Restoring from maximized state?
					else if (window->m_isMaximized)
					{
						window->m_isMaximized = false;
						if (Renderer::s_mainRenderer) Renderer::s_mainRenderer->OnResize();
					}
					else if (window->m_isResizing)
					{
						// If user is dragging the resize bars, we do not resize 
						// the buffers here because as the user continuously 
						// drags the resize bars, a stream of WM_SIZE messages are
						// sent to the window, and it would be pointless (and slow)
						// to resize for each WM_SIZE message received from dragging
						// the resize bars.  So instead, we reset after the user is 
						// done resizing the window and releases the resize bars, which 
						// sends a WM_EXITSIZEMOVE message.
					}
					else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
					{
						if (Renderer::s_mainRenderer) Renderer::s_mainRenderer->OnResize();
					}
				}

			}
			return 0;
		}

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		case WM_ENTERSIZEMOVE:
		{
			if (Window::s_mainWindow) Window::s_mainWindow->m_isResizing = true;
			return 0;
		}

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
		case WM_EXITSIZEMOVE:
		{
			if (Window::s_mainWindow) Window::s_mainWindow->m_isResizing = false;
			if (Renderer::s_mainRenderer) Renderer::s_mainRenderer->OnResize();
			return 0;
		}
	}

	// Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
	return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
}

//-----------------------------------------------------------------------------------------------
// Processes all Windows messages (WM_xxx) for this app that have queued up since last frame.
// For each message in the queue, our WindowsMessageHandlingProcedure (or "WinProc") function
//	is called, telling us what happened (key up/down, minimized/restored, gained/lost focus, etc.)
//
void Window::RunMessagePump()
{
	MSG queuedMessage;
	for (;; )
	{
		BOOL const wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call our "WindowsMessageHandlingProcedure" (a.k.a. "WinProc") function
	}
}

//-----------------------------------------------------------------------------------------------
void Window::CreateOSWindow()
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	HMODULE applicationInstanceHandle = ::GetModuleHandle(NULL);
	float clientAspect = m_config.m_aspectRatio;

	// Define a window style/class
	WNDCLASSEX windowClassDescription;
	memset(&windowClassDescription, 0, sizeof(windowClassDescription));
	windowClassDescription.cbSize = sizeof(windowClassDescription);
	windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
	windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
	windowClassDescription.hInstance = GetModuleHandle(NULL);
	windowClassDescription.hIcon = NULL;
	windowClassDescription.hCursor = NULL;
	windowClassDescription.lpszClassName = TEXT("Simple Window Class");
	RegisterClassEx(&windowClassDescription);

	// #SD1ToDo: Add support for fullscreen mode (requires different window style flags than windowed mode)
	//DWORD const windowStyleFlags = WS_CAPTION | WS_BORDER  | WS_SYSMENU | WS_OVERLAPPED;
	DWORD const windowStyleFlags = WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
	DWORD const windowStyleExFlags = WS_EX_APPWINDOW;

	// Get desktop rect, dimensions, aspect
	RECT desktopRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect(desktopWindowHandle, &desktopRect);
	float desktopWidth = (float)(desktopRect.right - desktopRect.left);
	float desktopHeight = (float)(desktopRect.bottom - desktopRect.top);
	float desktopAspect = desktopWidth / desktopHeight;

	// Calculate maximum client size (as some % of desktop size)
	constexpr float maxClientFractionOfDesktop = 0.90f;
	float clientWidth = desktopWidth * maxClientFractionOfDesktop;
	float clientHeight = desktopHeight * maxClientFractionOfDesktop;
	if (clientAspect > desktopAspect)
	{
		// Client window has a wider aspect than desktop; shrink client height to match its width
		clientHeight = clientWidth / clientAspect;
	}
	else
	{
		// Client window has a taller aspect than desktop; shrink client width to match its height
		clientWidth = clientHeight * clientAspect;
	}

	// Calculate client rect bounds by centering the client area
	float clientMarginX = 0.5f * (desktopWidth - clientWidth);
	float clientMarginY = 0.5f * (desktopHeight - clientHeight);
	RECT clientRect;
	clientRect.left = (int)clientMarginX;
	clientRect.right = clientRect.left + (int)clientWidth;
	clientRect.top = (int)clientMarginY;
	clientRect.bottom = clientRect.top + (int)clientHeight;
	m_clientDimensions.x = (int)clientWidth;
	m_clientDimensions.y = (int)clientHeight;

	// Calculate the outer dimensions of the physical window, including frame et. al.
	RECT windowRect = clientRect;
	AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);

	WCHAR windowTitle[1024];
	MultiByteToWideChar(GetACP(), 0, m_config.m_windowTitle.c_str(), -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]));
	HWND windowHandle = CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		windowTitle,
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		(HINSTANCE)applicationInstanceHandle,
		NULL);
	// Store the window handle
	m_windowHandle = windowHandle;

	ShowWindow(windowHandle, SW_SHOW);
	SetForegroundWindow(windowHandle);
	SetFocus(windowHandle);

	m_displayContext = GetDC(windowHandle);

	HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
	SetCursor(cursor);
}

void Window::UpdateClientDimensions(IntVec2 newDimensions)
{
	m_clientDimensions = newDimensions;
}

