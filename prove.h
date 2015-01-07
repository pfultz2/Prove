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

#ifndef PROVE_NO_MOVABLE_STREAMS
#define PROVE_NO_MOVABLE_STREAMS 1
#endif

namespace prove {

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

#define PROVE_CONTEXT(...) prove::context(prove_callback, #__VA_ARGS__, __FILE__, __LINE__)

// TODO: Add not operator
class predicate_result
{
    bool r;
#if PROVE_NO_MOVABLE_STREAMS
    std::unique_ptr<std::stringstream> ss;
    std::stringstream& stream()
    {
        return *this->ss;
    }

    const std::stringstream& stream() const
    {
        return *this->ss;
    }
#else
    std::stringstream ss;
    std::stringstream& stream()
    {
        return this->ss;
    }

    const std::stringstream& stream() const
    {
        return this->ss;
    }
#endif
public:
    predicate_result(bool result) : r(result)
#if PROVE_NO_MOVABLE_STREAMS
    , ss(new std::stringstream())
#endif
    {};

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
};

predicate_result as_predicate_result(predicate_result&& expr)
{
    return std::move(expr);
}
template<class T>
predicate_result as_predicate_result(const T& expr)
{
    predicate_result pr(expr.value());
    pr << expr;
    return pr;
}

void check_predicate(const context& ctx, const predicate_result& pr)
{
    if (not pr.result()) ctx.fail(pr.message()); 
}

#define PROVE_CHECK_PREDICATE(...) prove::check_predicate(PROVE_CONTEXT(__VA_ARGS__), __VA_ARGS__)

template<class F>
predicate_result check_expression(F f)
{
    predicate_result pr(false);
    try
    {
        auto result = f();
        return as_predicate_result(result);
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

#define PROVE_CHECK(...) PROVE_CHECK_PREDICATE(prove::check_expression([&]{ return prove::capture() ->* __VA_ARGS__; }))

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

    expression(T lhs, U rhs) : lhs(lhs), rhs(rhs)
    {}

    template<class Stream>
    friend Stream& operator<<(Stream& s, const expression& self)
    {
        s << " [ " << self.lhs << " " << Operator::as_string() << " " << self.rhs << " ]";
        return s;
    }

    auto value() const PROVE_RETURNS(Operator::call(lhs, rhs));
};

template<class T, class U, class Operator>
expression<T, U, Operator> make_expression(const T& rhs, const U& lhs, Operator)
{
    return expression<T, U, Operator>(rhs, lhs);
}

template<class T>
struct lhs_expression;

template<class T>
lhs_expression<T> make_lhs_expression(const T& lhs)
{
    return lhs_expression<T>(lhs);
}

template<class T>
struct lhs_expression
{
    T lhs;
    lhs_expression(T e) : lhs(e)
    {}

    template<class Stream>
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


typedef std::function<void(prove::context::callback_function prove_callback)> test_case;
static std::vector<std::pair<std::string, test_case> > test_cases;

struct auto_register
{
    auto_register(std::string name, test_case tc)
    {
        test_cases.push_back(std::make_pair(name, tc));
    }
};

#define PROVE_CASE(name) \
struct name \
{ void operator()(prove::context::callback_function prove_callback) const; }; \
static prove::auto_register name ## _register = prove::auto_register(#name, name()); \
void name::operator()(prove::context::callback_function prove_callback) const


void run()
{
    bool failed = false;
    for(const auto& tc: test_cases)
    {
        tc.second([&](const context& ctx, const std::string& message)
        {
            std::cout << "*****FAILED: " << tc.first << " at: " << std::endl << ctx.file << ":" << ctx.line << ": " << ctx.text << std::endl 
                      << message << std::endl;
            failed = true;
        });
    }
    if (not failed) std::cout << "All " << test_cases.size() << " test cases passed." << std::endl;
}

}

#endif
