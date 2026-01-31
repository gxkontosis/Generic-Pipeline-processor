#include <iostream>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <any>

class EventBus
{
public:
   EventBus() {};

   template<typename EventType, typename... EventHandlers>
   void registerEvent(EventHandlers&&... Handlers);

   template<typename EventType>
   void unregisterEvent();

   template<typename EventType>
   void emit(const EventType& event);

   template<typename EventType>
   void unregisterHandler(std::function<void(const EventType&)> func);

   private:
   std::unordered_map<std::type_index, std::any> m_eventHandlersMap;
};

template<typename EventType, typename... EventHandlers>
void EventBus::registerEvent(EventHandlers&&... Handlers)
{
   std::vector<std::function<void(const EventType&)>> handlers{};

   const std::type_index eventIndex = std::type_index(typeid(EventType));

   if (m_eventHandlersMap.find(eventIndex) != m_eventHandlersMap.end())
   {
      handlers = any_cast<std::vector<std::function<void(const EventType&)>>&>( m_eventHandlersMap[eventIndex] );
   }

   ( handlers.push_back(std::forward<EventHandlers>(Handlers)) , ... );

   m_eventHandlersMap[eventIndex] = handlers;
}

template<typename EventType>
void EventBus::unregisterEvent()
{
   m_eventHandlersMap.erase(std::type_index(typeid(EventType)));
}

template<typename EventType>
void EventBus::unregisterHandler(std::function<void(const EventType&)> func)
{
   std::type_index eventIndex = std::type_index(typeid(EventType));

   auto& handlers = any_cast<std::vector<std::function<void(const EventType&)>>&>(m_eventHandlersMap[eventIndex]);

   for (int i=0; i<handlers.size(); i++) 
   {
      if (handlers[i] == func)
      {
         handlers.erase(handlers[i]);
      }
   }
}

template<typename EventType>
void EventBus::emit(const EventType& param)
{
   const std::type_index eventIndex = std::type_index(typeid(EventType));

   if (m_eventHandlersMap.find(eventIndex) != m_eventHandlersMap.end())
   {
      const std::vector<std::function<void(const EventType&)>>& handlers = any_cast<std::vector<std::function<void(const EventType&)>>&>(m_eventHandlersMap[eventIndex]);

      for (const auto& handler : handlers)
      {
         handler(param);
      }
   }
}

int main()
{
    EventBus bus;

    auto out = [](int val) {std::cout << val << std::endl;};
    auto ptrRet = [](const std::shared_ptr<int> myPtr) {std::cout << *myPtr << std::endl;};

    auto ptr = std::make_shared<int>(32);

    bus.registerEvent<int>(out, out);
    bus.registerEvent<std::shared_ptr<int>>(ptrRet);

    bus.emit<int>(5);
    bus.emit<std::shared_ptr<int>>(ptr);

    bus.unregisterEvent<int>();
    bus.emit<int>(5);

    return 0;
}