#ifndef SRCCTL_EVENT_H
#define SRCCTL_EVENT_H

namespace CTL {
    enum class EventType {
        None,
        WindowClose,
        WindowResize,
        KeyPressed,
        KeyReleased,
        MouseMoved
    };

    class Event {
    public:
        virtual ~Event() = default;
        virtual EventType GetType() const = 0;
        bool Handled = false;
    };
}

#endif
