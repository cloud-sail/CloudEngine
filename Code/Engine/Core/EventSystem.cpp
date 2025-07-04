#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"

//-----------------------------------------------------------------------------------------------
EventSystem* g_theEventSystem = nullptr;

//-----------------------------------------------------------------------------------------------
EventSystem::EventSystem(EventSystemConfig const& config)
	: m_config(config)
{

}

EventSystem::~EventSystem()
{

}

void EventSystem::Startup()
{

}

void EventSystem::Shutdown()
{

}

void EventSystem::BeginFrame()
{

}

void EventSystem::EndFrame()
{

}

void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr)
{
	SubscriptionList& subscriptionList = m_subscriptionListByEventName[eventName];
	subscriptionList.emplace_back(functionPtr);
}

void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr)
{
	auto found = m_subscriptionListByEventName.find(eventName);
	if (found == m_subscriptionListByEventName.end())
	{
		return;
	}
	//SubscriptionList& subscribersForThisEvent = found->second;
	//int numSubscribers = static_cast<int>(subscribersForThisEvent.size());
	//for (int i = 0; i < numSubscribers; ++i)
	//{
	//	EventSubscription*& subscriber = subscribersForThisEvent[i];
	//	if (subscriber && subscriber->m_functionPtr == func)
	//	{
	//		subscriber = nullptr;
	//	}
	//}
	SubscriptionList& subscriptionList = found->second;
	for (auto it = subscriptionList.begin(); it != subscriptionList.end();)
	{
		EventCallbackFunction* currentFunctionPtr = it->m_functionPtr;
		if (currentFunctionPtr == functionPtr)
		{
			it = subscriptionList.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
	auto found = m_subscriptionListByEventName.find(eventName);
	if (found == m_subscriptionListByEventName.end())
	{
		if (g_theDevConsole)
		{
			g_theDevConsole->AddText(DevConsole::ERROR, "Unknown Command: " + eventName + ". Type Help for commands.");
		}
		return; // nobody subscribed to this event (return int(0))
	}

	// Found a list of subscribers for this event; call each one in turn (or until someone "consumes" the event)
	SubscriptionList& subscribersForThisEvent = found->second;
	int numSubscribers = static_cast<int>(subscribersForThisEvent.size());
	for (int i = 0; i < numSubscribers; ++i)
	{
		//EventSubscription* subscriber = subscribersForThisEvent[i];
		//if (subscriber)
		//{
		//	bool wasConsumed = subscriber->m_functionPtr(args); // Execute the subscriber's callback function!
		//	if (wasConsumed)
		//	{
		//		break; // Event was "consumed" by this subscriber; stop notifying any other subscribers!
		//	}
		//}
		EventSubscription& subscriber = subscribersForThisEvent[i];
		if (subscriber.m_functionPtr)
		{
			bool wasConsumed = subscriber.m_functionPtr(args); // Execute the subscriber's callback function!
			if (wasConsumed)
			{
				break; // Event was "consumed" by this subscriber; stop notifying any other subscribers!
			}
		}
	}
	// return numSubscribers;
}

void EventSystem::FireEvent(std::string const& eventName)
{
	EventArgs args; // Temporary, but stable, fake empty args; important, as subscribers
					// may "pass" into from one to another using args.
	FireEvent(eventName, args);
}


void EventSystem::GetAllRegistedCommands(Strings& outCommandNames) const
{
	outCommandNames.clear();
	outCommandNames.reserve(m_subscriptionListByEventName.size());

	for (auto it = m_subscriptionListByEventName.cbegin(); it != m_subscriptionListByEventName.cend(); ++it)
	{
		outCommandNames.emplace_back(it->first);
	}
}

//-----------------------------------------------------------------------------------------------

void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr)
{
	g_theEventSystem->SubscribeEventCallbackFunction(eventName, functionPtr);
}

void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction* functionPtr)
{
	g_theEventSystem->UnsubscribeEventCallbackFunction(eventName, functionPtr);
}

void FireEvent(std::string const& eventName, EventArgs& args)
{
	g_theEventSystem->FireEvent(eventName, args);
}

void FireEvent(std::string const& eventName)
{
	g_theEventSystem->FireEvent(eventName);
}
