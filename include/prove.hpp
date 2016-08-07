/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    prove.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PROVE_GUARD_PROVE_TEST_H
#define PROVE_GUARD_PROVE_TEST_H

#include <functional>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <memory>
#include <iostream>
#ifdef _MSC_VER
#include <iso646.h>
#endif


#define PROVE_PRIMITIVE_CAT(x, ...) x ## __VA_ARGS__
#define PROVE_CAT(x, ...) PROVE_PRIMITIVE_CAT(x, __VA_ARGS__)


#define PROVE_PRIMITIVE_STRINGIZE(...) #__VA_ARGS__
#define PROVE_STRINGIZE(...) PROVE_PRIMITIVE_STRINGIZE(__VA_ARGS__)

namespace prove {

template<class T, class... Ts>
struct head
{ typedef T type; };

struct context
{
    typedef std::function<void(const context& ctx, const std::string& message)> callback_function;
    context(const callback_function& cf, const std::string& t, const std::string& f, int l) 
    : callback(cf), text(t), file(f), line(l)
    {};
    const callback_function& callback;
    const std::string& text;
    const std::string& file;
    int line;

    void fail(const std::string& message) const
    {
        callback(*this, message);
    }
};

template<class T, class=void>
struct stream_base
{
    stream_base() : ss()
    {}
    T ss;
    T& stream()
    {
        return this->ss;
    }

    const T& stream() const
    {
        return this->ss;
    }
};

template<class T>
struct is_movable
#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7
: std::false_type
#else
: std::is_move_constructible<T>
#endif
{};

template<class T>
struct stream_base<T, typename std::enable_if<!is_movable<T>::value>::type>
{
    stream_base() : ss(new T())
    {}
    std::unique_ptr<T> ss;
    T& stream()
    {
        return *this->ss;
    }

    const T& stream() const
    {
        return *this->ss;
    }
};

// TODO: Add not operator
class predicate_result : stream_base<std::stringstream>
{
    bool r;
    typedef stream_base<std::stringstream> base;
public:
    predicate_result() : base(), r(false)
    {};

    predicate_result(bool result) : base(), r(result)
    {}

    predicate_result(bool result, std::string message) : base(), r(result)
    {
        *this << message;
    }

    template<class T>
    predicate_result& operator<<(const T& x)
    {
        stream() << x;
        return *this;
    }

    std::string message() const
    {
        return stream().str();
    }

    operator bool() const
    {
        return r;
    }

    bool result() const
    {
        return r;
    }

    bool& result()
    {
        return r;
    }
};

predicate_result as_predicate_result(predicate_result expr)
{
    return expr;
}
template<class T>
auto as_predicate_result(const T& expr) 
    -> typename head<predicate_result, decltype(expr.value())>::type
{
    predicate_result pr(expr.value());
    pr << expr;
    return pr;
}

template<class F>
predicate_result check_expression(F f)
{
    predicate_result pr(false);
    try
    {
        auto result = f();
        return as_predicate_result(std::move(result));
    }
    catch(const std::exception& ex)
    {
        pr << "Exception thrown: " << ex.what();
        // throw;
    }
    catch(...)
    {
        pr << "An unknown exception has occured";
        // throw;
    }
    return pr;
}

#define PROVE_RETURNS(...) -> decltype(__VA_ARGS__) { return (__VA_ARGS__); } static_assert(true, "")

#define PROVE_FOREACH_OPERATOR(m) \
m(==, equal) \
m(!=, not_equal) \
m(<=, less_than_equal) \
m(>=, greater_than_equal) \
m(<, less_than) \
m(>, greater_than)

namespace detail {

#define PROVE_EACH_OPERATOR_OBJECT(op, name) \
constexpr struct name ## _f \
{ \
    static std::string as_string() { return #op; } \
    template<class T, class U> \
    static auto call(T&& x, U&& y) PROVE_RETURNS(x op y); \
} name {};

PROVE_FOREACH_OPERATOR(PROVE_EACH_OPERATOR_OBJECT)

template<class T, class U, class Operator>
struct expression
{
    T lhs;
    U rhs;

    expression(T lhs_, U rhs_) : lhs(lhs_), rhs(rhs_)
    {}

    template<class Stream, class=typename std::enable_if<!std::is_same<Stream, predicate_result>::value>::type>
    friend Stream& operator<<(Stream& s, const expression& self)
    {
        s << " [ " << self.lhs << " " << Operator::as_string() << " " << self.rhs << " ]";
        return s;
    }

    auto value() const PROVE_RETURNS(Operator::call(lhs, rhs));
};

template<class T, class U, class Operator>
expression<typename std::decay<T>::type, typename std::decay<U>::type, Operator> 
make_expression(T&& rhs, U&& lhs, Operator)
{
    return { std::forward<T>(rhs), std::forward<U>(lhs) };
}

template<class T>
struct lhs_expression;

template<class T>
lhs_expression<typename std::decay<T>::type> make_lhs_expression(T&& lhs)
{
    return lhs_expression<typename std::decay<T>::type>{ std::forward<T>(lhs) };
}

template<class T>
struct lhs_expression
{
    T lhs;
    explicit lhs_expression(T e) : lhs(e)
    {}

    template<class Stream, class=typename std::enable_if<!std::is_same<Stream, predicate_result>::value>::type>
    friend Stream& operator<<(Stream& s, const lhs_expression& self)
    {
        s << self.lhs;
        return s;
    }

    T value() const
    {
        return lhs;
    }

#define PROVE_LHS_OPERATOR(op, name) \
    template<class U> \
    auto operator op(const U& rhs) const PROVE_RETURNS(make_expression(lhs, rhs, name)); 

PROVE_FOREACH_OPERATOR(PROVE_LHS_OPERATOR)

#define PROVE_LHS_REOPERATOR(op) \
    template<class U> auto operator op(const U& rhs) const PROVE_RETURNS(make_lhs_expression(lhs op rhs));
PROVE_LHS_REOPERATOR(+)
PROVE_LHS_REOPERATOR(-)
PROVE_LHS_REOPERATOR(*)
PROVE_LHS_REOPERATOR(/)
PROVE_LHS_REOPERATOR(%)
PROVE_LHS_REOPERATOR(&)
PROVE_LHS_REOPERATOR(|)
PROVE_LHS_REOPERATOR(&&)
PROVE_LHS_REOPERATOR(||)

};

}

struct capture 
{
    predicate_result operator->* (predicate_result&& x)
    {
        return std::move(x);
    }
    template<class T>
    auto operator->* (const T& x) PROVE_RETURNS(detail::make_lhs_expression(x));
};

template<class Prove_TypeName_>
const std::string& get_type_name()
{
    static std::string name;

    if (name.empty())
    {
#ifdef _MSC_VER
        name = typeid(Prove_TypeName_).name();
        name = name.substr(7);
#else
        const char parameter_name[] = "Prove_TypeName_ =";

        name = __PRETTY_FUNCTION__;

        auto begin = name.find(parameter_name) + sizeof(parameter_name);
#if (defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7)
        auto length = name.find_last_of(",") - begin;
#else
        auto length = name.find_first_of("];", begin) - begin;
#endif
        name = name.substr(begin, length);
#endif
    }

    return name;
}

template<class T, class F>
struct auto_register_factory
{
    auto_register_factory()
    {
        F::template apply<T>();
    }
};

template<class T, class F>
struct auto_register
{
    static auto_register_factory<T, F> static_register_;

    auto_register()
    {
        this->register_();
    }

    bool register_()
    {
        (void)&static_register_;
        return true;
    }
};

template<class T, class F>
auto_register_factory<T, F> auto_register<T, F>::static_register_ = auto_register_factory<T, F>();

struct test_case_register
{
    typedef std::function<void(prove::context::callback_function prove_callback)> run_case;
    static std::vector<std::pair<std::string, run_case>>& get_cases()
    {
        static std::vector<std::pair<std::string, run_case>> x;
        return x;
    }

    template<class T>
    static void apply()
    {
        register_case(get_type_name<T>(), [](prove::context::callback_function prove_callback)
        {
            T c;
            c.prove_callback = prove_callback;
            c.test();
        });
    }

    static void register_case(const std::string& name, run_case rc)
    {
        get_cases().push_back(std::make_pair(name, rc));
    }
};

template<class Derived>
struct test_case : auto_register<Derived, test_case_register>
{
    prove::context::callback_function prove_callback;

    context create_context(const std::string& t, const std::string& f, int l)
    {
        this->register_();
        return context(prove_callback, t, f, l);
    }

    context create_context(const std::string& t)
    {
        return this->create_context(t, "", 0);
    }

    context create_context()
    {
        return this->create_context("", "", 0);
    }

    void check(const predicate_result& pr, const context& ctx)
    {
        if (not pr.result()) ctx.fail(pr.message());
    }

    void check(const predicate_result& pr, const std::string& t="")
    {
        if (not pr.result()) this->create_context(t).fail(pr.message());
    }

    void check(bool b, const std::string& message="")
    {
        if (not b) this->create_context().fail(message);
    }
};

template<class F>
predicate_result throws(F f, std::string s="")
{
    try { f(); }
    catch(...) { return {true, s}; }
    return {false, s};
}

template<class Exception, class F>
predicate_result throws(F f, std::string s="")
{
    try { f(); }
    catch(const Exception&) { return {true, s}; }
    return {false, s};
}

#define PROVE_CONTEXT(...) this->create_context(#__VA_ARGS__, __FILE__, __LINE__)
#define PROVE_CHECK(...) this->check(prove::check_expression([&]{ return prove::capture() ->* __VA_ARGS__; }), PROVE_CONTEXT(__VA_ARGS__))
#define PROVE_THROWS(...) this->check(prove::throws([&]{ __VA_ARGS__; }, "Failed to throw"), PROVE_CONTEXT(__VA_ARGS__))
#define PROVE_THROWS_AS(expr, ...) this->check(prove::throws<__VA_ARGS__>([&]{ expr; }, "Failed to throw exception " #__VA_ARGS__), PROVE_CONTEXT(expr))
#define PROVE_STATIC_CHECK(...) static_assert((__VA_ARGS__), #__VA_ARGS__)

#define PROVE_DETAIL_CASE_CLASS(name) \
struct name : prove::test_case<name>

#define PROVE_DETAIL_CASE(name) \
struct name : prove::test_case<name> \
{ name() {} void test(); }; \
void name::test()

#define PROVE_CASE(...) PROVE_DETAIL_CASE(PROVE_CAT(__VA_ARGS__ ## _case_, __LINE__))
#define PROVE_CASE_CLASS(...) PROVE_DETAIL_CASE_CLASS(PROVE_CAT(__VA_ARGS__ ## _case_, __LINE__))

void run()
{
    bool failed = false;
    for(const auto& tc: test_case_register::get_cases())
    {
        tc.second([&](const context& ctx, const std::string& message)
        {
            std::cout << "*****FAILED: " << tc.first << " at: " << std::endl << ctx.file << ":" << ctx.line << ": " << ctx.text << std::endl 
                      << message << std::endl;
            failed = true;
        });
    }
    if (not failed) std::cout << "All " << test_case_register::get_cases().size() << " test cases passed." << std::endl;
}

}

#endif
