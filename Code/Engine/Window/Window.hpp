#pragma once
#include "Engine/Math/IntVec2.hpp"
#include <string>

//-----------------------------------------------------------------------------------------------
class InputSystem;
struct Vec2;


//-----------------------------------------------------------------------------------------------
struct WindowConfig
{
	InputSystem*	m_inputSystem = nullptr; // #ToDo remove this settings in old projects
	float			m_aspectRatio = (16.f / 9.f);
	std::string		m_windowTitle = "Unnamed SD Engine Application";
	bool			m_imGuiCaptureKeyBoardInput = true;
	bool			m_imGuiCaptureMouseInput = true;
};


//-----------------------------------------------------------------------------------------------
class Window
{
public:
	Window(WindowConfig const& config);
	~Window() = default;

	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	//Vec2 GetNormalizedMouseUV() const;

	void* GetHwnd() const;
	IntVec2 GetClientDimensions() const;

	bool IsFocused() const;
	float GetAspectRatio() const;

	//  For Windows Call Back Function
	WindowConfig const& GetConfig() const;
	void* GetDisplayContext() const;

public:
	static Window* s_mainWindow; // singleton, only one window now

private:
	void RunMessagePump();
	void CreateOSWindow();
	
private:
	WindowConfig m_config;
	void* m_windowHandle = nullptr;   // Actually a Windows HWND on the Windows platform
	void* m_displayContext = nullptr; // Actually a Windows HDC on the Windows platform // imgui may use it
	IntVec2 m_clientDimensions;


public:
	void UpdateClientDimensions(IntVec2 newDimensions);

	bool m_isMinimized = false;
	bool m_isMaximized = false;
	bool m_isResizing = false;
};

