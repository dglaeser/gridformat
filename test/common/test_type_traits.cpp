#include <concepts>
#include <vector>
#include <array>

#include <gridformat/common/type_traits.hpp>

int main() {

    static_assert(std::same_as<int, GridFormat::MDRangeScalar<std::array<int, 2>>>);
    static_assert(std::same_as<int, GridFormat::MDRangeScalar<std::vector<int>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::array<double, 2>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::vector<double>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::array<std::array<double, 2>, 2>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::vector<std::vector<double>>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::vector<std::vector<std::vector<double>>>>>);

    static_assert(GridFormat::mdrange_dimension<std::array<int, 2>> == 1);
    static_assert(GridFormat::mdrange_dimension<std::vector<std::vector<int>>> == 2);
    static_assert(GridFormat::mdrange_dimension<std::vector<std::vector<std::vector<int>>>> == 3);

    return 0;
}