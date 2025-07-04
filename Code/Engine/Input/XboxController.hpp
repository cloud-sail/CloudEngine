#pragma once
#include "Engine/Input/KeyButtonState.hpp"
#include "Engine/Input/AnalogJoystick.hpp"

enum class XboxButtonId
{
	XBOX_BUTTON_INVALID = -1,
	XBOX_BUTTON_UP,
	XBOX_BUTTON_DOWN,
	XBOX_BUTTON_LEFT,
	XBOX_BUTTON_RIGHT,
	XBOX_BUTTON_START,
	XBOX_BUTTON_BACK,
	XBOX_BUTTON_LEFT_THUMB,
	XBOX_BUTTON_RIGHT_THUMB,
	XBOX_BUTTON_LEFT_SHOULDER,
	XBOX_BUTTON_RIGHT_SHOULDER,
	XBOX_BUTTON_A,
	XBOX_BUTTON_B,
	XBOX_BUTTON_X,
	XBOX_BUTTON_Y,
	NUM_XBOX_BUTTONS
};



class XboxController
{
	friend class InputSystem;

public:
	XboxController() = default;
	~XboxController() = default;

	bool					IsConnected() const;
	int						GetControllerID() const;
	AnalogJoystick const&	GetLeftStick() const;
	AnalogJoystick const&	GetRightStick() const;
	float					GetLeftTrigger() const;
	float					GetRightTrigger() const;
	KeyButtonState const&	GetButton(XboxButtonId buttonID) const;
	bool					IsButtonDown(XboxButtonId buttonID) const;
	bool					WasButtonJustPressed(XboxButtonId buttonID) const;
	bool					WasButtonJustReleased(XboxButtonId buttonID) const;

private:
	void Update();
	void Reset();
	void UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY);
	void UpdateTrigger(float& out_triggerValue, unsigned char rawValue);
	void UpdateButton(XboxButtonId buttonID, unsigned short wButtons, unsigned short buttonFlag);

private:
	int				m_id = -1;
	bool			m_isConnected = false;
	float			m_leftTrigger = 0.f;
	float			m_rightTrigger = 0.f;
	KeyButtonState	m_buttons[static_cast<int>(XboxButtonId::NUM_XBOX_BUTTONS)];
	AnalogJoystick	m_leftStick;
	AnalogJoystick	m_rightStick;

};

