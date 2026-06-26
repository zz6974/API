#ifndef SRCCTL_EVENTDISPATCHER_H
#define SRCCTL_EVENTDISPATCHER_H

#include "UIEvent.h"
#include <functional>

namespace CTL {
    class EventDispatcher {
    public:
        template<typename T>
        void Dispatch(std::function<void(T&)> handler) {
            if (event.GetType() == T::StaticType()) {
                handler(static_cast<T&>(event));
                event.Handled = true;
            }
        }

        explicit EventDispatcher(Event& e) : event(e) {}
    private:
        Event& event;
    };
}

#endif
