#ifndef SRCCTL_INPUT_EVENT_H
#define SRCCTL_INPUT_EVENT_H

#include "UI/UIEvent.h"

namespace CTL {
    class KeyEvent : public Event {
    public:
        KeyEvent(int k) : Key(k) {}
        int Key;
    };

    class KeyPressedEvent : public KeyEvent {
    public:
        KeyPressedEvent(int k) : KeyEvent(k) {}
        EventType GetType() const override { return EventType::KeyPressed; }
    };
}

#endif
