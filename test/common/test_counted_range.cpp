#include <vector>
#include <algorithm>

#include <gridformat/common/counted_range.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    "counted_range"_test = [] () {
        std::vector<int> v{1, 2, 3, 4};
        expect(std::ranges::equal(
            GridFormat::CountedRange{v},
            std::vector<std::size_t>{1, 3, 6, 10}
        ));
    };

    return 0;
}