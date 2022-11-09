#include <algorithm>

#include <boost/ut.hpp>

#include <gridformat/common/extended_range.hpp>

int main() {
    using namespace boost::ut;

    "extended_range"_test = [] () {
        std::vector<int> data{1, 2, 3};
        auto extended = GridFormat::make_extended<5>(std::views::all(data));
        expect(std::ranges::equal(
            extended,
            std::vector<int>{1, 2, 3, 0, 0}
        ));
    };

    return 0;
}