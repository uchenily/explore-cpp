#include "rang.hpp"

using namespace rang;

auto main() -> int {
    std::cout << style::reversed << "Test Text" << style::reset << '\n';
    std::cout << fg::green << "Test Text" << fg::reset << '\n';
    std::cout << bg::cyan << "Test Text" << bg::reset << '\n';
}
