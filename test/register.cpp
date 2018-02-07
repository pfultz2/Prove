#include "test.h"

#include <exception>
#include <stdexcept>

struct register_case : prove::test_case<register_case>
{
    void test()
    {
        if (prove::test_case_register::get_cases().size() != 1) 
        {
            printf("FAILED\n");
            std::abort();
        }
    }
};
