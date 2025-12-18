#pragma once

class Helper
{
public:
    struct factor
    {
        int a;
        int b;
    };
    static factor factoring(int num, int max_factor_b);
    Helper();
    ~Helper();
};

