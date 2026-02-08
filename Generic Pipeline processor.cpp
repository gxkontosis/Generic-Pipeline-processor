#include <iostream>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <any>
#include <algorithm>
#include <shared_mutex>

enum Priority
{
   DEFAULT = 0,
   IMPORTANT = 1,
   VIMPORTANT = 2
};

template<typename EventType>
struct HandlerData
{
   std::function<void(const EventType&)> handler;
   unsigned int ID;
   Priority prio;
};

template<typename T>
concept HandlerPriorityPair = requires(T t) {
    t.first;  // has .first
    t.second;
};


class EventBus
{
public:
   EventBus() {};

   template<typename EventType, typename... HandlerPairs>
   requires (HandlerPriorityPair<std::remove_cvref_t<HandlerPairs>> && ...)
   void registerHandler(HandlerPairs&&... handlerPairs);

   template<typename EventType>
   void unregisterEvent();

   template<typename EventType>
   void emit(const EventType& event);

   template<typename EventType>
   void unregisterHandler(unsigned int ID);

   private:
   std::unordered_map<std::type_index, std::any> m_eventHandlersMap;
   std::shared_mutex m_mutex;
   static unsigned int ID;
};

// Notice the way we define!
unsigned int EventBus::ID = 0;

// METHODS
template<typename EventType, typename... HandlerPairs>
requires (HandlerPriorityPair<std::remove_cvref_t<HandlerPairs>> && ...)
void EventBus::registerHandler(HandlerPairs&&... handlerPairs)
{
   std::unique_lock<std::shared_mutex> lock(m_mutex);

   std::vector<HandlerData<EventType>> handlers{};

   const std::type_index eventIndex = std::type_index(typeid(EventType));

   if (m_eventHandlersMap.find(eventIndex) != m_eventHandlersMap.end())
   {
      handlers = any_cast<std::vector<HandlerData<EventType>>&>( m_eventHandlersMap[eventIndex] );
   }

   ( handlers.push_back(HandlerData<EventType>{
      std::forward<HandlerPairs>(handlerPairs).first,  // could also be std::move(handlerPairs.first)
      ID++,                                  
      std::forward<HandlerPairs>(handlerPairs).second                         // prio
   }), ... );

   std::sort(handlers.begin(), handlers.end(), 
             [](const HandlerData<EventType>& handler1, const HandlerData<EventType>& handler2) {return handler1.prio > handler2.prio;});


   m_eventHandlersMap[eventIndex] = handlers;
}

template<typename EventType>
void EventBus::unregisterEvent()
{
   std::unique_lock<std::shared_mutex> lock(m_mutex);

   m_eventHandlersMap.erase(std::type_index(typeid(EventType)));
}

template<typename EventType>
void EventBus::unregisterHandler(unsigned int ID)
{
   std::unique_lock<std::shared_mutex> lock(m_mutex);

   std::type_index eventIndex = std::type_index(typeid(EventType));

   auto& handlers = any_cast<std::vector<HandlerData<EventType>>&>(m_eventHandlersMap[eventIndex]);

   for (auto it = handlers.begin(); it != handlers.end(); it++) 
   {
      if (it->ID == ID)
      {
         handlers.erase(it);
         std::cout << "Handler erased!" << std::endl;
         break;
      }
   }
}

template<typename EventType>
void EventBus::emit(const EventType& param)
{
   std::shared_lock<std::shared_mutex> lock(m_mutex);

   const std::type_index eventIndex = std::type_index(typeid(EventType));

   if (m_eventHandlersMap.find(eventIndex) != m_eventHandlersMap.end())
   {
      std::vector<HandlerData<EventType>>& handlers = any_cast<std::vector<HandlerData<EventType>>&>(m_eventHandlersMap[eventIndex]);

      for (const auto& hand : handlers)
      {
         hand.handler(param);
      }
   } 
}

int main()
{
    EventBus bus;

    auto out = [](int val) {std::cout << val << std::endl;};
    auto ptrRet = [](const std::shared_ptr<int> myPtr) {std::cout << *myPtr << std::endl;};

    auto ptr = std::make_shared<int>(32);

    bus.registerHandler<int>(
      std::make_pair(out, Priority::IMPORTANT),
      std::make_pair(out, Priority::DEFAULT)
    );

    bus.registerHandler<std::shared_ptr<int>>(
      std::make_pair(ptrRet, Priority::IMPORTANT),
      std::make_pair(ptrRet, Priority::DEFAULT)
    );

    bus.emit<int>(5);
    bus.emit<std::shared_ptr<int>>(ptr);

   // bus.unregisterEvent<int>();
    bus.emit<int>(5);

    bus.unregisterHandler<int>(0);

    return 0;
}