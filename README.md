EventBus

A lightweight, type-safe event bus written in C++20. Handlers can be registered for any event type and emit events that get dispatched
to all registered handlers in priority order. There are three priority levels (DEFAULT, IMPORTANT, VIMPORTANT) and handlers are sorted upon registration
so emitting is cheap. You can also unregister individual handlers by ID or clear all handlers for a given event type.
Thread safety is handled via std::shared_mutex — multiple emits can run concurrently, while registering or unregistering takes an exclusive lock. 
