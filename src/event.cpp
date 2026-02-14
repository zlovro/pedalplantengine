//
// Created by lovro on 14/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "event.hpp"

std::vector<Event *> Event::instances = std::vector<Event *>();

Event::Event(const char *pName)
{
    name  = pName;
    calls = std::vector<EventFunc>();

    instances.push_back(this);
}

void Event::registerCall(const EventFunc pCall)
{
    calls.push_back(pCall);
}

void Event::trigger() const
{
    for (const auto &call: calls)
    {
        call();
    }
}

Event::~Event()
{
}
