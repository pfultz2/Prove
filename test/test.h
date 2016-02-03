
#include <prove.hpp>

int main() 
{
    if (prove::test_case_register::get_cases().empty()) std::cout << "***** FAILED: No test cases" << std::endl;
    prove::run();
}
