//
// Created by lovro on 14/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef EVENT_HPP
#define EVENT_HPP

#include <vector>

typedef void (*EventFunc)();

class Event
{
    public:
    static std::vector<Event *> instances;

    const char *           name;
    std::vector<EventFunc> calls;

    explicit Event(const char *pName = "NULL");

    void registerCall(EventFunc pCall);
    void trigger() const;

    ~Event();
};

#endif //EVENT_HPP
