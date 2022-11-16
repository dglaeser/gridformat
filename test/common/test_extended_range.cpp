#include <algorithm>

#include <gridformat/common/extended_range.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::expect;
    using GridFormat::Testing::operator""_test;

    "extended_range"_test = [] () {
        std::vector<int> data{1, 2, 3};
        auto extended = GridFormat::make_extended<5>(std::views::all(data));
        expect(std::ranges::equal(
            extended,
            std::vector<int>{1, 2, 3, 0, 0}
        ));
    };

    "extended_range_custom_extension_value"_test = [] () {
        std::vector<int> data{1, 2, 3};
        auto extended = GridFormat::make_extended<5>(std::views::all(data), 42);
        expect(std::ranges::equal(
            extended,
            std::vector<int>{1, 2, 3, 42, 42}
        ));
    };

    return 0;
}