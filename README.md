Prove
=====

Lightweight C++ test framework

Test cases
----------

A test case can be created by using the `prove::test_case` class with a `test()` method:

```cpp
struct my_case : prove::test_case<my_case>
{
    void test()
    {
        // Assertions
    }
};
```

The library will construct the object and then run the `test()` method. The constructor and destructor can be used for setup and teardown of each test. 

For convience the `PROVE_CASE` macro can be used to define a class with a `test()` method to reduce the boilerplate:

```cpp
PROVE_CASE(my_case)
{
    // Assertions
}
```

Also, the `PROVE_CASE_CLASS` will define the class:

```cpp
PROVE_CASE_CLASS(my_case)
{
    void test()
    {
        // Assertions
    }
};
```

Also, the name can be left out of the macros, and it will choose a unique name. 

Finally, to run all test cases, use `prove::run()`:

```cpp
int main() 
{
    prove::run();
}
```

Assertions
----------

The `this->check()` method can be used to check assertions:

```cpp
struct my_case : prove::test_case<my_case>
{
    void test()
    {
        this->check(true);
    }
};
```

A custom message can be provided for the assertion as well:

```cpp
struct my_case : prove::test_case<my_case>
{
    void test()
    {
        this->check(true, "True is now false");
    }
};
```

When checking expressions, it is useful to show the values that were used in the expression. The `PROVE_CHECK` macro can parse the expression and display the values that were used:

```cpp
PROVE_CASE()
{
    int i = 5;
    PROVE_CHECK(i == 5);
}
```

It will also display the source file and line number in the output.

Custom predicates
-----------------

The `prove::predicate_result` can be used to display a custom message. For example,

```cpp
prove::predicate_result equal(int x, int y)
{
    prove::predicate_result r(x == y);
    r << x << " == " << y << " failed";
    return r;
}

struct my_case : prove::test_case<my_case>
{
    void test()
    {
        int i = 5;
        this->check(equal(i, 5));
    }
};
```

Template cases
--------------

Cases can be templated or be defined as nested classes in a template class. For example, when defining a template case:

```cpp
template<class T>
struct template_case : prove::test_case<template_case<T>>
{
    void test()
    {
        T i = 5;
        PROVE_CHECK(i == 5);
    }
};
```

Each template parameter can be used by the following:

```cpp
template struct template_case<int>;
template struct template_case<float>;
```

If there is many test case that are parameterized over the same template parameter, then each test case can be added as a nested class:

```cpp
template<class T>
struct template_suite
{
    struct template_case1 : prove::test_case<template_case1<T>>
    {
        void test()
        {
            T i = 5;
            PROVE_CHECK(i == 5);
        }
    };

    struct template_case2 : prove::test_case<template_case2<T>>
    {
        void test()
        {
            T i = 3;
            PROVE_CHECK(i == 3);
        }
    };
};

template struct template_suite<int>;
template struct template_suite<float>;
```
