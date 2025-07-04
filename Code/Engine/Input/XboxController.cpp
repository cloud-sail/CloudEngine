#include "Engine/Input/XboxController.hpp"
#include "Engine/Math/MathUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // must #include Windows.h before #including Xinput.h
#include <Xinput.h> // include the Xinput API header file (interface)
#pragma comment( lib, "xinput" ) // Link in the xinput.lib static library


bool XboxController::IsConnected() const
{
	return m_isConnected;
}

int XboxController::GetControllerID() const
{
	return m_id;
}

AnalogJoystick const& XboxController::GetLeftStick() const
{
	return m_leftStick;
}

AnalogJoystick const& XboxController::GetRightStick() const
{
	return m_rightStick;
}

float XboxController::GetLeftTrigger() const
{
	return m_leftTrigger;
}

float XboxController::GetRightTrigger() const
{
	return m_rightTrigger;
}

KeyButtonState const& XboxController::GetButton(XboxButtonId buttonID) const
{
	return m_buttons[static_cast<int>(buttonID)];
}

bool XboxController::IsButtonDown(XboxButtonId buttonID) const
{
	return m_buttons[static_cast<int>(buttonID)].m_isPressed;
}

bool XboxController::WasButtonJustPressed(XboxButtonId buttonID) const
{
	return !m_buttons[static_cast<int>(buttonID)].m_wasPressedLastFrame &&
			m_buttons[static_cast<int>(buttonID)].m_isPressed;
}

bool XboxController::WasButtonJustReleased(XboxButtonId buttonID) const
{
	return m_buttons[static_cast<int>(buttonID)].m_wasPressedLastFrame &&
		  !m_buttons[static_cast<int>(buttonID)].m_isPressed;
}

void XboxController::Update()
{
	// Read raw controller state via XInput API
	XINPUT_STATE xboxControllerState;
	memset(&xboxControllerState, 0, sizeof(xboxControllerState));
	DWORD errorStatus = XInputGetState(m_id, &xboxControllerState);
	if (errorStatus != ERROR_SUCCESS)
	{
		Reset();
		return;
	}
	m_isConnected = true;

	// Update internal data structure(s) based on raw controller state
	XINPUT_GAMEPAD const& state = xboxControllerState.Gamepad;
	UpdateJoystick(m_leftStick, state.sThumbLX, state.sThumbLY);
	UpdateJoystick(m_rightStick, state.sThumbRX, state.sThumbRY);

	UpdateTrigger(m_leftTrigger, state.bLeftTrigger);
	UpdateTrigger(m_rightTrigger, state.bRightTrigger);

	UpdateButton(XboxButtonId::XBOX_BUTTON_UP,				state.wButtons, XINPUT_GAMEPAD_DPAD_UP       );
	UpdateButton(XboxButtonId::XBOX_BUTTON_DOWN,			state.wButtons, XINPUT_GAMEPAD_DPAD_DOWN     );
	UpdateButton(XboxButtonId::XBOX_BUTTON_LEFT,			state.wButtons, XINPUT_GAMEPAD_DPAD_LEFT     );
	UpdateButton(XboxButtonId::XBOX_BUTTON_RIGHT,			state.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT    );
	UpdateButton(XboxButtonId::XBOX_BUTTON_START,			state.wButtons, XINPUT_GAMEPAD_START         );
	UpdateButton(XboxButtonId::XBOX_BUTTON_BACK,			state.wButtons, XINPUT_GAMEPAD_BACK          );
	UpdateButton(XboxButtonId::XBOX_BUTTON_LEFT_THUMB,		state.wButtons, XINPUT_GAMEPAD_LEFT_THUMB    );
	UpdateButton(XboxButtonId::XBOX_BUTTON_RIGHT_THUMB,		state.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB   );
	UpdateButton(XboxButtonId::XBOX_BUTTON_LEFT_SHOULDER,	state.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER );
	UpdateButton(XboxButtonId::XBOX_BUTTON_RIGHT_SHOULDER,	state.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	UpdateButton(XboxButtonId::XBOX_BUTTON_A,				state.wButtons, XINPUT_GAMEPAD_A             );
	UpdateButton(XboxButtonId::XBOX_BUTTON_B,				state.wButtons, XINPUT_GAMEPAD_B             );
	UpdateButton(XboxButtonId::XBOX_BUTTON_X,				state.wButtons, XINPUT_GAMEPAD_X             );
	UpdateButton(XboxButtonId::XBOX_BUTTON_Y,				state.wButtons, XINPUT_GAMEPAD_Y             );
}

void XboxController::Reset()
{
	m_isConnected = false;
	for (int i = 0; i < static_cast<int>(XboxButtonId::NUM_XBOX_BUTTONS); ++i)
	{
		m_buttons[static_cast<int>(i)] = KeyButtonState();
	}
	m_leftTrigger = 0.f;
	m_rightTrigger = 0.f;
	m_leftStick = AnalogJoystick();
	m_rightStick = AnalogJoystick();
}

void XboxController::UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY)
{
	float rawNormalizedX = RangeMap(static_cast<float>(rawX), -32768.f, 32767.f, -1.f, 1.f);
	float rawNormalizedY = RangeMap(static_cast<float>(rawY), -32768.f, 32767.f, -1.f, 1.f);
	out_joystick.UpdatePosition(rawNormalizedX, rawNormalizedY);

}

void XboxController::UpdateTrigger(float& out_triggerValue, unsigned char rawValue)
{
	out_triggerValue = RangeMap(static_cast<float>(rawValue), 0.f, 255.f, 0.f, 1.f);
}

void XboxController::UpdateButton(XboxButtonId buttonID, unsigned short wButtons, unsigned short buttonFlag)
{
	bool isKeyDownThisFrame = ((wButtons & buttonFlag) == buttonFlag);
	
	m_buttons[static_cast<int>(buttonID)].m_wasPressedLastFrame = m_buttons[static_cast<int>(buttonID)].m_isPressed;
	m_buttons[static_cast<int>(buttonID)].m_isPressed = isKeyDownThisFrame;
}
