
#include "test.h"

#include <exception>
#include <stdexcept>

prove::predicate_result get_predicate()
{
    prove::predicate_result r(true);
    r << "Predicate result failed";
    return r;
}

prove::predicate_result get_assign_predicate()
{
    prove::predicate_result r;
    r.result() = true;
    r << "Predicate result failed";
    return r;
}

PROVE_CASE()
{
    int i = 5;
    PROVE_CHECK(i == 5);
    PROVE_CHECK(get_predicate());
    PROVE_CHECK(get_assign_predicate());
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

struct throws_case : prove::test_case<throws_case>
{
    void simple_throw()
    {
        throw 1;
    }

    void runtime_error_throw()
    {
        throw std::runtime_error("Error");
    }
    void test()
    {
        PROVE_THROWS(this->simple_throw());
        PROVE_THROWS(this->runtime_error_throw());

        PROVE_THROWS_AS(this->simple_throw(), int);
        PROVE_THROWS_AS(this->runtime_error_throw(), std::runtime_error);
    }
};


