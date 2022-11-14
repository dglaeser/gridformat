#include <concepts>
#include <vector>
#include <array>

#include <gridformat/common/type_traits.hpp>

template<typename E1, typename... Expected, typename... Ts>
constexpr bool check_unique_variant(const std::variant<Ts...>& variant) {
    constexpr bool e1_is_contained = GridFormat::is_any_of<E1, Ts...>;
    if constexpr (sizeof...(Expected) > 0)
        return e1_is_contained && check_unique_variant<Expected...>(variant);
    else
        return e1_is_contained;
}

struct Foo {};

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

    static_assert(check_unique_variant<int>(GridFormat::UniqueVariant<int>{}));
    static_assert(check_unique_variant<int, double>(GridFormat::UniqueVariant<int, double>{}));
    static_assert(check_unique_variant<int, double>(GridFormat::UniqueVariant<int, double, int>{}));
    static_assert(check_unique_variant<int, double>(GridFormat::UniqueVariant<int, double, int, int>{}));
    static_assert(check_unique_variant<int, double, Foo>(GridFormat::UniqueVariant<int, int, double, Foo, int>{}));

    return 0;
}