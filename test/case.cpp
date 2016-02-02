
#include "test.h"

prove::predicate_result get_predicate()
{
    prove::predicate_result r(true);
    r << "Predicate result failed";
    return r;
}

PROVE_CASE()
{
    int i = 5;
    PROVE_CHECK(i == 5);
    PROVE_CHECK(get_predicate());
    PROVE_CHECK(true);
}


struct my_case : prove::test_case<my_case>
{
    void test()
    {
        this->check(true, "True is now false");
        this->check(true);
        this->check(get_predicate(), "Failed predicate");
    }
};

struct check_typename : prove::test_case<check_typename>
{
    void test()
    {
        PROVE_CHECK(prove::get_type_name<check_typename>() == "check_typename");
    }
};
