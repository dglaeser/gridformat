// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

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

template<int dim> struct StaticEnumVector { enum { size = dim }; };
template<int dim> struct StaticIntVector { static constexpr int size = dim; };
template<int dim> struct StaticFunctionVector { static constexpr int size() { return dim; } };

int main() {

    static_assert(GridFormat::all_equal<int{1}, std::size_t{1}, char{1}>);
    static_assert(GridFormat::all_equal<1, 1>);
    static_assert(GridFormat::all_equal<1>);
    static_assert(!GridFormat::all_equal<1, 1, 2>);

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

    static_assert(GridFormat::static_size<std::array<double, 2>> == 2);
    static_assert(GridFormat::static_size<std::array<double, 3>> == 3);
    static_assert(GridFormat::static_size<std::array<std::vector<int>, 3>> == 3);
    static_assert(GridFormat::static_size<StaticEnumVector<2>> == 2);
    static_assert(GridFormat::static_size<StaticIntVector<2>> == 2);
    static_assert(GridFormat::static_size<StaticFunctionVector<2>> == 2);

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

    static_assert(std::same_as<
        GridFormat::ExtendedVariant<std::variant<int, double>, char, std::string, char>,
        std::variant<int, double, char, std::string>
    >);
    static_assert(std::same_as<
        GridFormat::MergedVariant<std::variant<int, double>, std::variant<char, unsigned>>,
        std::variant<int, double, char, unsigned>
    >);
    static_assert(std::same_as<
        GridFormat::ReducedVariant<std::variant<int, double, char>, double, char>,
        std::variant<int>
    >);

    static_assert(GridFormat::default_value<double> == 0.0);
    static_assert(GridFormat::default_value<std::array<double, 1>>[0] == 0.0);
    static_assert(GridFormat::default_value<std::array<double, 2>>[1] == 0.0);

    static_assert(GridFormat::DefaultValue<double>::get() == 0.0);
    static_assert(GridFormat::DefaultValue<std::array<double, 1>>::get()[0] == 0.0);
    static_assert(GridFormat::DefaultValue<std::array<double, 2>>::get()[1] == 0.0);

    return 0;
}
