// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::MDLayout
 */
#ifndef GRIDFORMAT_COMMON_MD_LAYOUT_HPP_
#define GRIDFORMAT_COMMON_MD_LAYOUT_HPP_

#include <ostream>
#include <utility>
#include <vector>
#include <cassert>
#include <numeric>
#include <iterator>
#include <algorithm>
#include <initializer_list>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Represents the layout (dimension, extents) of a multi-dimensional range
 */
class MDLayout {
 public:
    MDLayout() = default;

    template<std::ranges::forward_range R>
    explicit MDLayout(R&& extents) {
        _extents.reserve(Ranges::size(extents));
        std::ranges::copy(extents, std::back_inserter(_extents));
    }

    template<std::integral T>
    explicit MDLayout(std::initializer_list<T> extents) {
        _extents.reserve(Ranges::size(extents));
        std::ranges::copy(extents, std::back_inserter(_extents));
    }

    std::size_t dimension() const {
        return _extents.size();
    }

    std::size_t extent(std::size_t codim) const {
        return _extents[codim];
    }

    std::size_t number_of_entries() const {
        return std::accumulate(
            _extents.begin(),
            _extents.end(),
            std::size_t{1},
            std::multiplies{}
        );
    }

    std::size_t number_of_entries(std::size_t codim) const {
        return sub_layout(codim).number_of_entries();
    }

    MDLayout sub_layout(std::size_t codim) const {
        assert(codim < dimension());
        return MDLayout{std::vector<std::size_t>{
            _extents.begin() + codim,
            _extents.end()
        }};
    }

    bool is_scalar() const {
        return _extents.size() == 0;
    }

    bool operator==(const MDLayout& other) const {
        return std::ranges::equal(_extents, other._extents);
    }

    friend std::ostream& operator<<(std::ostream& s, const MDLayout& layout) {
        s << "(";
        if (layout._extents.size() > 0) {
            s << layout._extents[0];
            std::ranges::for_each(layout._extents | std::views::drop(1), [&] (const auto ext) {
                s << "," << ext;
            });
        }
        s << ")";
        return s;
    }

 private:
    std::vector<std::size_t> _extents;
};


#ifndef DOXYGEN
namespace Detail {

    template<std::ranges::range R, std::size_t size>
    constexpr void set_sub_extents(std::array<std::size_t, size>& extents, std::size_t dim) {
        assert(dim < size);
        if constexpr (Concepts::StaticallySizedRange<R>)
            extents[dim] = StaticSize<R>::value;
        else
            extents[dim] = 0;
        if constexpr (has_sub_range<R>)
            set_sub_extents<std::ranges::range_value_t<R>>(extents, dim + 1);
    }

    template<std::ranges::range R, std::size_t size>
    constexpr void set_sub_extents_from_instance(R&& r, std::array<std::size_t, size>& extents, std::size_t dim) {
        assert(dim < size);
        if constexpr (Concepts::StaticallySizedRange<R>)
            extents[dim] = StaticSize<R>::value;
        else
            extents[dim] = Ranges::size(r);
        if constexpr (has_sub_range<R>) {
            if (extents[dim] == 0)
                set_sub_extents<std::ranges::range_value_t<R>>(extents, dim + 1);
            else {
                set_sub_extents_from_instance(*std::ranges::begin(r), extents, dim + 1);
                assert(std::ranges::all_of(r, [&] (const std::ranges::range auto& sub_range) {
                    return static_cast<std::size_t>(Ranges::size(sub_range)) == extents[dim + 1];
                }));
            }
        }
    }

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Get the multi-dimensional layout for the given range
 */
template<std::ranges::range R>
MDLayout get_md_layout(R&& r) {
    std::array<std::size_t, mdrange_dimension<R>> extents;
    Detail::set_sub_extents_from_instance(r, extents, 0);
    return MDLayout{extents};
}

/*!
 * \ingroup Common
 * \brief Overload for scalars
 */
template<Concepts::Scalar T>
MDLayout get_md_layout(const T&) {
    return MDLayout{};
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MD_LAYOUT_HPP_
