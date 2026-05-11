#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include <algorithm>

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

void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr)
{
	std::unique_lock<std::shared_mutex> lock(m_subscriptionMutex);
	//SubscriptionList& subscriptionList = m_subscriptionListByEventName[eventName];
	//subscriptionList.emplace_back(functionPtr);
	HashedCaseInsensitiveString hashedKey(eventName);
	SubscriptionList& subscribers = m_subscriptionListByEventName[hashedKey];
	EventFunctionSubscription* newSub = new EventFunctionSubscription(functionPtr);
	subscribers.push_back(newSub);
}

void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr)
{
	std::unique_lock<std::shared_mutex> lock(m_subscriptionMutex);

	//auto found = m_subscriptionListByEventName.find(eventName);
	//if (found == m_subscriptionListByEventName.end())
	//{
	//	return;
	//}
	////SubscriptionList& subscribersForThisEvent = found->second;
	////int numSubscribers = static_cast<int>(subscribersForThisEvent.size());
	////for (int i = 0; i < numSubscribers; ++i)
	////{
	////	EventSubscription*& subscriber = subscribersForThisEvent[i];
	////	if (subscriber && subscriber->m_functionPtr == func)
	////	{
	////		subscriber = nullptr;
	////	}
	////}
	//SubscriptionList& subscriptionList = found->second;
	//for (auto it = subscriptionList.begin(); it != subscriptionList.end();)
	//{
	//	EventCallbackFunction* currentFunctionPtr = it->m_functionPtr;
	//	if (currentFunctionPtr == functionPtr)
	//	{
	//		it = subscriptionList.erase(it);
	//	}
	//	else
	//	{
	//		++it;
	//	}
	//}


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

		EventFunctionSubscription* asFuncSub = dynamic_cast<EventFunctionSubscription*>(subscriber);
		if (asFuncSub && asFuncSub->m_func == functionPtr)
		{
			delete subscriber;
			subscriber = nullptr;
			//break; // why need to break
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
	SubscriptionList localCopy;
	bool foundSubscribers = false;

	{
		std::shared_lock<std::shared_mutex> lock(m_subscriptionMutex);
		auto found = m_subscriptionListByEventName.find(eventName);
		if (found != m_subscriptionListByEventName.end())
		{
			localCopy = found->second;
			foundSubscribers = true;
		}
	} // Release Lock before calling callbacks

	if (!foundSubscribers)
	{
		if (g_theDevConsole)
		{
			g_theDevConsole->AddText(DevConsole::ERROR, "Unknown Command: " + eventName + ". Type Help for commands.");
		}
		return;
	}

	int numSubscribers = static_cast<int>(localCopy.size());
	for (int i = 0; i < numSubscribers; ++i)
	{
		EventSubscriptionBase* subscriber = localCopy[i];
		if (subscriber)
		{
			bool wasConsumed = subscriber->Execute(args);
			if (wasConsumed)
			{
				break; // Event was "consumed" by this subscriber; stop notifying any other subscribers!
			}
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName)
{
	EventArgs args; // Temporary, but stable, fake empty args; important, as subscribers
					// may "pass" into from one to another using args.
	FireEvent(eventName, args);
}


void EventSystem::GetAllRegistedCommands(Strings& outCommandNames, bool includeEmpty /*= false*/) const
{
	std::shared_lock<std::shared_mutex> lock(m_subscriptionMutex);

	outCommandNames.clear();
	outCommandNames.reserve(m_subscriptionListByEventName.size());

	for (auto it = m_subscriptionListByEventName.cbegin(); it != m_subscriptionListByEventName.cend(); ++it)
	{
		if (!includeEmpty)
		{
			SubscriptionList const& subscribers = it->second;
			bool hasLiveSubscriber = false;
			for (EventSubscriptionBase* subscriber : subscribers)
			{
				if (subscriber)
				{
					hasLiveSubscriber = true;
					break;
				}
			}
			if (!hasLiveSubscriber) continue;
		}
		outCommandNames.emplace_back(it->first.GetOriginalString());
	}
}

//-----------------------------------------------------------------------------------------------

void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr)
{
	if (g_theEventSystem)
	{
		g_theEventSystem->SubscribeEventCallbackFunction(eventName, functionPtr);
	}
}

void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtr functionPtr)
{
	if (g_theEventSystem)
	{
		g_theEventSystem->UnsubscribeEventCallbackFunction(eventName, functionPtr);
	}
}

void FireEvent(std::string const& eventName, EventArgs& args)
{
	if (g_theEventSystem)
	{
		g_theEventSystem->FireEvent(eventName, args);
	}
}

void FireEvent(std::string const& eventName)
{
	if (g_theEventSystem)
	{
		g_theEventSystem->FireEvent(eventName);
	}
}

EventRecipient::~EventRecipient()
{
	if (g_theEventSystem)
	{
		g_theEventSystem->UnsubscribeAllForObject(this);
	}
}
