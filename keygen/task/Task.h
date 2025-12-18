#pragma once

class Task
{
public:
    Task();
    ~Task();
    static void init_globe_range();
public:
    size_t count;
    short start_range[16];
    static short m_globe_range[32];
};

