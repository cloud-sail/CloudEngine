#pragma once
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include <vector>
#include <string>
#include <map>
#include <shared_mutex> 

//-----------------------------------------------------------------------------------------------
// Make Event Register and Fire not case sensitive
//#include <cctype>
//#include <algorithm>
//struct CaseInsensitiveCompare 
//{
//	bool operator()(const std::string& a, const std::string& b) const {
//		return std::lexicographical_compare(
//			a.begin(), a.end(),
//			b.begin(), b.end(),
//			[](unsigned char ac, unsigned char bc) {
//				return std::tolower(ac) < std::tolower(bc);
//			}
//		);
//	}
//};


//-----------------------------------------------------------------------------------------------
typedef NamedProperties EventArgs;

//-----------------------------------------------------------------------------------------------
// Subscription base class (type erasure for standalone vs. member function)
//-----------------------------------------------------------------------------------------------
struct EventSubscriptionBase
{
	virtual ~EventSubscriptionBase() = default;
	virtual bool Execute(EventArgs& args) = 0;
	virtual void* GetObjectPtr() const { return nullptr; } // standalone returns nullptr
};

//-----------------------------------------------------------------------------------------------
// Standalone / static function subscription
//-----------------------------------------------------------------------------------------------
using EventCallbackFunction = bool(EventArgs& args);
using EventCallbackFunctionPtr = EventCallbackFunction*;

struct EventFunctionSubscription : public EventSubscriptionBase
{
	EventFunctionSubscription(EventCallbackFunctionPtr func)
		: m_func(func)
	{
	}

	EventCallbackFunctionPtr m_func = nullptr;

	virtual bool Execute(EventArgs& args) override
	{
		return m_func(args);
	}

	// GetObjectPtr() inherited from base, returns nullptr - correct
};

//-----------------------------------------------------------------------------------------------
// Object method subscription
//-----------------------------------------------------------------------------------------------
template<typename T>
struct EventObjectMethodSubscription : public EventSubscriptionBase
{
	typedef bool (T::* EventObjectMethodPtr)(EventArgs& args);

	EventObjectMethodSubscription(T* object, EventObjectMethodPtr method)
		: m_object(object)
		, m_method(method)
	{
	}

	T* m_object = nullptr;
	EventObjectMethodPtr m_method = nullptr;

	virtual bool Execute(EventArgs& args) override
	{
		return (m_object->*m_method)(args);
	}

	virtual void* GetObjectPtr() const override
	{
		// dynamic_cast<void*> returns the address of the most-derived object
		// safe even under multiple inheritance
		return dynamic_cast<void*>(m_object);
	}
};

//-----------------------------------------------------------------------------------------------
typedef std::vector<EventSubscriptionBase*> SubscriptionList;


//-----------------------------------------------------------------------------------------------
//typedef NamedStrings EventArgs;
//typedef NamedProperties EventArgs;
// C++ typedef for "any function which takes a (mutable) EventArgs by reference, and returns a bool"
//typedef bool (EventCallbackFunction)(EventArgs& args); // or you may alternatively use the new C++ "using" syntax for type aliasing

//using EventCallbackFunction = bool(EventArgs& args);
//using EventCallbackFunctionPtr = EventCallbackFunction*;


//struct EventSubscription
//{
//	EventSubscription(EventCallbackFunction* functionPtr)
//		: m_functionPtr(functionPtr) {}
//
//	EventCallbackFunction* m_functionPtr = nullptr;
//};


//------------------------------------------------------------------------------------------------
// Similar to our EntityList typedef in Libra, this is just "a list of subscriptions"
//typedef std::vector<EventSubscription*>		SubscriptionList; // Note: a list of pointers

//typedef std::vector<EventSubscription> SubscriptionList; 

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

	// TODO change all EventCallbackFunction to xxxptr
	// Standalone function
	void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr);
	void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr);

	// Object method
	template<typename T>
	void SubscribeEventCallbackObjectMethod(std::string const& eventName, T* objectPtr, bool (T::* method)(EventArgs& Args));

	template<typename T>
	void UnsubscribeEventCallbackObjectMethod(std::string const& eventName, T* objectPtr, bool (T::* method)(EventArgs& args));

	// Unsubscribe ALL subscriptions for a given object (any event)
	template<typename T>
	void UnsubscribeAllForObject(T* objectPtr);

	// Fire
	void FireEvent(std::string const& eventName, EventArgs& args);
	void FireEvent(std::string const& eventName);

	void GetAllRegistedCommands(Strings& outCommandNames, bool includeEmpty = false) const;

protected:
	EventSystemConfig m_config;
	mutable std::shared_mutex m_subscriptionMutex;
	std::map<HashedCaseInsensitiveString, SubscriptionList> m_subscriptionListByEventName;
};

template<typename T>
void EventSystem::SubscribeEventCallbackObjectMethod(std::string const& eventName, T* objectPtr, bool (T::* method)(EventArgs& Args))
{
	std::unique_lock<std::shared_mutex> lock(m_subscriptionMutex);
	HashedCaseInsensitiveString hashedKey(eventName);
	SubscriptionList& subscribers = m_subscriptionListByEventName[hashedKey];
	EventObjectMethodSubscription<T>* newSub = new EventObjectMethodSubscription<T>(objectPtr, method);
	subscribers.push_back(newSub);
}

template<typename T>
void EventSystem::UnsubscribeEventCallbackObjectMethod(std::string const& eventName, T* objectPtr, bool (T::* method)(EventArgs& args))
{
	std::unique_lock<std::shared_mutex> lock(m_subscriptionMutex);
	HashedCaseInsensitiveString hashedKey(eventName);
	auto found = m_subscriptionListByEventName.find(hashedKey);
	if (found == m_subscriptionListByEventName.end())
	{
		return;
	}

	SubscriptionList& subscribers = found->second;
	for (int i = 0; i < static_cast<int>(subscribers.size()); ++i)
	{
		EventSubscriptionBase*& subscriber = subscribers[i];
		if (!subscriber)
		{
			continue;
		}

		EventObjectMethodSubscription<T>* asObjMethod = dynamic_cast<EventObjectMethodSubscription<T>*>(subscriber);

		if (asObjMethod && asObjMethod->m_object == objectPtr && asObjMethod->m_method == method)
		{
			delete subscriber;
			subscriber = nullptr;
			break;
		}

	}
}

template<typename T>
void EventSystem::UnsubscribeAllForObject(T* objectPtr)
{
	// dynamic_cast<void*> gets the most-derived object address
	void* mostDerivedPtr = dynamic_cast<void*>(objectPtr);

	std::unique_lock<std::shared_mutex> lock(m_subscriptionMutex);
	for (auto& pair : m_subscriptionListByEventName)
	{
		SubscriptionList& subscribers = pair.second;
		for (int i = 0; i < static_cast<int>(subscribers.size()); ++i)
		{
			EventSubscriptionBase*& subscriber = subscribers[i];
			if (!subscriber)
			{
				continue;
			}

			if (subscriber->GetObjectPtr() == mostDerivedPtr)
			{
				delete subscriber;
				subscriber = nullptr;
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------
// EventRecipient: derive from this to auto-unsubscribe on destruction
//-----------------------------------------------------------------------------------------------
class EventRecipient
{
public:
	virtual ~EventRecipient();
};

//-----------------------------------------------------------------------------------------------
extern EventSystem* g_theEventSystem;

//-----------------------------------------------------------------------------------------------
// Standalone global-namespace helper functions; these forward to "the" event system, if it exists
//-----------------------------------------------------------------------------------------------
void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr);
void FireEvent(std::string const& eventName, EventArgs& args);
void FireEvent(std::string const& eventName);

template<typename T>
void SubscribeEventCallbackObjectMethod(std::string const& eventName, T* objectPtr, bool (T::* method)(EventArgs& args))
{
	if (g_theEventSystem)
	{
		g_theEventSystem->SubscribeEventCallbackObjectMethod(eventName, objectPtr, method);
	}
}

template<typename T>
void UnsubscribeEventCallbackObjectMethod(std::string const& eventName, T* objectPtr, bool (T::* method)(EventArgs& args))
{
	if (g_theEventSystem)
	{
		g_theEventSystem->UnsubscribeEventCallbackObjectMethod(eventName, objectPtr, method);
	}
}

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

