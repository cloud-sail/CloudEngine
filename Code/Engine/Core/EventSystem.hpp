#pragma once
#include "Engine/Core/NamedStrings.hpp"
#include <vector>
#include <string>
#include <map>

//-----------------------------------------------------------------------------------------------
// Make Event Register and Fire not case sensitive
#include <cctype>
#include <algorithm>
struct CaseInsensitiveCompare 
{
	bool operator()(const std::string& a, const std::string& b) const {
		return std::lexicographical_compare(
			a.begin(), a.end(),
			b.begin(), b.end(),
			[](unsigned char ac, unsigned char bc) {
				return std::tolower(ac) < std::tolower(bc);
			}
		);
	}
};
//-----------------------------------------------------------------------------------------------
typedef NamedStrings EventArgs;
// C++ typedef for “any function which takes a (mutable) EventArgs by reference, and returns a bool”
typedef bool (EventCallbackFunction)(EventArgs& args); // or you may alternatively use the new C++ “using” syntax for type aliasing

struct EventSubscription
{
	EventSubscription(EventCallbackFunction* functionPtr)
		: m_functionPtr(functionPtr) {}

	EventCallbackFunction* m_functionPtr = nullptr;
};


//------------------------------------------------------------------------------------------------
// Similar to our EntityList typedef in Libra, this is just "a list of subscriptions"
//typedef std::vector<EventSubscription*>		SubscriptionList; // Note: a list of pointers

typedef std::vector<EventSubscription> SubscriptionList; 

//-----------------------------------------------------------------------------------------------
struct EventSystemConfig
{

};

//-----------------------------------------------------------------------------------------------
class EventSystem
{
public:
	EventSystem(EventSystemConfig const& config);
	~EventSystem();
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr);
	void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr);
	void FireEvent(std::string const& eventName, EventArgs& args);
	void FireEvent(std::string const& eventName);

	void GetAllRegistedCommands(Strings& outCommandNames) const;

protected:
	EventSystemConfig m_config;
	std::map<std::string, SubscriptionList, CaseInsensitiveCompare> m_subscriptionListByEventName;
};


// create Eventsystem in app first

//-----------------------------------------------------------------------------------------------
// Standalone global-namespace helper functions; these forward to "the" event system, if it exists
//
void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr);
void FireEvent(std::string const& eventName, EventArgs& args);
void FireEvent(std::string const& eventName);

//-----------------------------------------------------------------------------------------------
/*
Publish/Subscribe pattern

For now standalone function
next member function
counsume the event return true/false

eventSystem speed is ~ lua

button FireEvent
string-based

bool OnSunriseEventCallback(EventArgs& args)
*/


/*
standalone call member function
- global pointer
- EventArg with Pointer Value...

*/

