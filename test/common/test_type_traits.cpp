// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <concepts>
#include <vector>
#include <array>
#include <list>

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

template<typename T> struct Incomplete;
template<> struct Incomplete<Foo> {};

template<typename T>
struct IsForwardRange {
    static constexpr bool value = std::ranges::forward_range<T>;
};

template<typename T>
struct IsRandomAccessRange {
    static constexpr bool value = std::ranges::random_access_range<T>;
};

int main() {

    static_assert(GridFormat::is_scalar<int>);
    static_assert(GridFormat::is_scalar<unsigned>);
    static_assert(GridFormat::is_scalar<std::size_t>);
    static_assert(GridFormat::is_scalar<double>);
    static_assert(GridFormat::is_scalar<float>);

    static_assert(std::same_as<int, GridFormat::MDRangeScalar<std::array<int, 2>>>);
    static_assert(std::same_as<int, GridFormat::MDRangeScalar<std::vector<int>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::array<double, 2>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::vector<double>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::array<std::array<double, 2>, 2>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::vector<std::vector<double>>>>);
    static_assert(std::same_as<double, GridFormat::MDRangeScalar<std::vector<std::vector<std::vector<double>>>>>);

    static_assert(!GridFormat::has_sub_range<std::vector<int>>);
    static_assert(GridFormat::has_sub_range<std::vector<std::vector<int>>>);

    static_assert(GridFormat::mdrange_dimension<std::array<int, 2>> == 1);
    static_assert(GridFormat::mdrange_dimension<std::vector<std::vector<int>>> == 2);
    static_assert(GridFormat::mdrange_dimension<std::vector<std::vector<std::vector<int>>>> == 3);

    static_assert(GridFormat::mdrange_models<std::vector<int>, IsForwardRange>);
    static_assert(GridFormat::mdrange_models<std::vector<int>, IsRandomAccessRange>);
    static_assert(GridFormat::mdrange_models<std::list<int>, IsForwardRange>);
    static_assert(!GridFormat::mdrange_models<std::list<int>, IsRandomAccessRange>);

    static_assert(GridFormat::is_incomplete<Incomplete<double>>);
    static_assert(!GridFormat::is_complete<Incomplete<double>>);
    static_assert(!GridFormat::is_incomplete<Incomplete<Foo>>);
    static_assert(GridFormat::is_complete<Incomplete<Foo>>);

    static_assert(GridFormat::is_any_of<int, double, std::size_t, float, int>);
    static_assert(!GridFormat::is_any_of<int, double, std::size_t, float>);

    static_assert(check_unique_variant<int>(GridFormat::UniqueVariant<int>{}));
    static_assert(check_unique_variant<int, double>(GridFormat::UniqueVariant<int, double>{}));
    static_assert(check_unique_variant<int, double>(GridFormat::UniqueVariant<int, double, int>{}));
    static_assert(check_unique_variant<int, double>(GridFormat::UniqueVariant<int, double, int, int>{}));
    static_assert(check_unique_variant<int, double, Foo>(GridFormat::UniqueVariant<int, int, double, Foo, int>{}));

    static_assert(std::same_as<double, GridFormat::FieldScalar<std::vector<double>>>);
    static_assert(std::same_as<int, GridFormat::FieldScalar<std::vector<int>>>);
    static_assert(std::same_as<int, GridFormat::FieldScalar<std::vector<std::vector<int>>>>);
    static_assert(std::same_as<int, GridFormat::FieldScalar<int>>);

    return 0;
}
