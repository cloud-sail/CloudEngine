#pragma once
//-----------------------------------------------------------------------------------------------
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

//-----------------------------------------------------------------------------------------------
#define UNUSED(x) (void)(x)
#define STATIC

//-----------------------------------------------------------------------------------------------
class NamedStrings;
class EventSystem;
class DevConsole;
class InputSystem;

//-----------------------------------------------------------------------------------------------
extern NamedStrings g_gameConfigBlackboard;
extern EventSystem* g_theEventSystem;
extern DevConsole*	g_theDevConsole;
extern InputSystem* g_theInput;
