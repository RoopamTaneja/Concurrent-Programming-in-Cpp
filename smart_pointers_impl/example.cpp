#include "shared_ptr_ctrl_blk.h"
#include <iostream>

struct base
{
    int a;
    virtual void foo()
    {
        std::cout << "base\n";
    }
};

struct der : base
{
    int b;
    void foo()
    {
        std::cout << "der\n";
    }
};

int main()
{
    roopam::shd_ptr<base> p = new der;
    p->foo();
    p = roopam::make_shared<base>();
    p->foo();
}