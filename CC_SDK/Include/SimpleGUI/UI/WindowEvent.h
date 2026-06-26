#ifndef SRCCTL_WINDOW_EVENT_H
#define SRCCTL_WINDOW_EVENT_H

#include <cstdint>
#include "UIEvent.h"

namespace CTL {
    class WindowResizeEvent : public Event {
    public:
        WindowResizeEvent(uint32_t w, uint32_t h) : Width(w), Height(h) {}
        EventType GetType() const override { return EventType::WindowResize; }
        uint32_t Width, Height;
    };

    class WindowCloseEvent : public Event {
    public:
        EventType GetType() const override { return EventType::WindowClose; }
    };
}

#endif
