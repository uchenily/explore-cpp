#include "debug.hpp"
#include "print.hpp"

#include <format>
#include <list>
#include <map>
#include <numbers>
#include <optional>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

struct Person {
    std::string name;
    int         age;
};

namespace print_custom {
template <>
class printer<Person> {
public:
    auto print(const Person &person, std::ostream &out) {
        out << '{';
        print_to(out, person.name);
        out << ", ";
        print_to(out, person.age);
        out << '}';
    }
};
} // namespace print_custom

template <typename T>
struct std::formatter<std::vector<T>> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const auto &vector, std::format_context &ctx) const {
        std::ostringstream oss;

        bool first = true;
        for (const auto &item : vector) {
            if (first) {
                first = false;
            } else {
                oss << ", ";
            }
            oss << std::format("{}", item);
        }
        return std::format_to(ctx.out(), "[{}]", oss.str());
    }
};

using namespace print_hpp;

auto main() -> int {
    print("hello");
    print(std::string{"hello"});
    print(std::string_view{"hello"});
    print(true);
    print(3);
    print(3.14);
    print(std::vector{1, 2, 3});
    print(std::list{1.0, 2.0, 3.0});
    print(std::map<std::string, std::variant<std::string, int>>{
        {"hello", "world"},
        { "work",     996},
    });
    print(std::optional<int>{3});
    print(std::optional<int>{std::nullopt});
    print(std::nullopt);
    print(std::tuple{"red", false, 3.14});
    print(Person{.name = "zhangsan", .age = 18});

    // 需要定义 std::formatter<T>
    LOG_INFO("std::formatter<T>: {}", std::vector{"R", "G", "B", "Y"});

    // 自定义类型需要定义 print_custom::printer<T>
    LOG_INFO("print_custom::printer<T>: {}",
             P(std::vector{"R", "G", "B", "Y"}));
    LOG_INFO("print_custom::printer<T>: {}",
             P(Person{.name = "wangmazi", .age = 36}));

    print("hello", 'a', std::numbers::pi);
}
