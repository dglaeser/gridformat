// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \brief Traits classes for grid implementations to specialize.
 */
#ifndef GRIDFORMAT_GRID_TYPE_TRAITS_HPP_
#define GRIDFORMAT_GRID_TYPE_TRAITS_HPP_

#include <ranges>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace GridTypeTraitsDetail {

    template<typename T>
    struct CoordinateType;

    template<GridDetail::ExposesPointCoordinates T> requires(
        not GridDetail::ExposesSpacing<T> and
        not GridDetail::ExposesOrigin<T>)
    struct CoordinateType<T> {
        using type = std::ranges::range_value_t<GridDetail::PointCoordinates<T>>;
    };

    template<GridDetail::ExposesSpacing T> requires(not GridDetail::ExposesOrigin<T>)
    struct CoordinateType<T> {
        using type = std::ranges::range_value_t<GridDetail::Spacing<T>>;
    };

    template<GridDetail::ExposesOrigin T> requires(not GridDetail::ExposesSpacing<T>)
    struct CoordinateType<T> {
        using type = std::ranges::range_value_t<GridDetail::Origin<T>>;
    };

    template<typename T> requires(
        GridDetail::ExposesSpacing<T> and
        GridDetail::ExposesOrigin<T>)
    struct CoordinateType<T> {
        using type = std::common_type_t<
            std::ranges::range_value_t<GridDetail::Spacing<T>>,
            std::ranges::range_value_t<GridDetail::Origin<T>>
        >;
    };

    template<typename T>
    struct Dimension;

    template<typename T> requires(
        GridDetail::ExposesSpacing<T> or
        GridDetail::ExposesOrigin<T> or
        GridDetail::ExposesExtents<T>)
    struct Dimension<T> {
     private:
        static constexpr std::size_t _dimension() {
            if constexpr (GridDetail::ExposesSpacing<T>)
                return static_size<GridDetail::Spacing<T>>;
            if constexpr (GridDetail::ExposesOrigin<T>)
                return static_size<GridDetail::Origin<T>>;
            if constexpr (GridDetail::ExposesExtents<T>)
                return static_size<GridDetail::Extents<T>>;
        }

     public:
        static constexpr std::size_t value = _dimension();
    };

}  // namespace GridTypeTraitsDetail
#endif  // DOXYGEN

//! \addtogroup Grid
//! \{

template<GridDetail::ExposesPointRange Grid>
using Point = GridDetail::Point<Grid>;

template<GridDetail::ExposesCellRange Grid>
using Cell = GridDetail::Cell<Grid>;

template<typename T> requires(is_complete<GridTypeTraitsDetail::CoordinateType<T>>)
using CoordinateType = typename GridTypeTraitsDetail::CoordinateType<T>::type;

template<typename T> requires(is_complete<GridTypeTraitsDetail::Dimension<T>>)
inline constexpr std::size_t dimension = GridTypeTraitsDetail::Dimension<T>::value;

//! \} group Grid

}

#endif  // GRIDFORMAT_GRID_TYPE_TRAITS_HPP_
