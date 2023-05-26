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

#include <gridformat/common/reserved_vector.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Represents the layout (dimension, extents) of a multi-dimensional range
 */
class MDLayout {
    static constexpr std::size_t buffered_dimensions = 5;

 public:
    MDLayout() = default;

    template<Concepts::MDRange<1> R>
    explicit MDLayout(R&& extents) {
        _extents.reserve(Ranges::size(extents));
        std::ranges::copy(extents, std::back_inserter(_extents));
    }

    template<std::integral T>
    explicit MDLayout(const std::initializer_list<T>& extents) {
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

    template<std::ranges::range R>
    void export_to(R& out) const {
        if (Ranges::size(out) < dimension())
            throw SizeError("Given output range is too small");
        std::ranges::copy(_extents, std::ranges::begin(out));
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
    ReservedVector<std::size_t, buffered_dimensions> _extents;
};


#ifndef DOXYGEN
namespace Detail {

    template<Concepts::StaticallySizedRange R, typename Iterator>
    constexpr void set_sub_extents(Iterator it) {
        *it = static_size<R>;
        ++it;
        if constexpr (has_sub_range<R>)
            set_sub_extents<std::ranges::range_value_t<R>>(it);
    }

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Get the layout of a range (or a scalar) whose size is known at compile-time
 */
template<typename T> requires(Concepts::StaticallySizedRange<T> or Concepts::Scalar<T>)
constexpr MDLayout get_md_layout() {
    if constexpr (Concepts::Scalar<T>)
        return MDLayout{};
    else {
        std::array<std::size_t, mdrange_dimension<T>> extents;
        extents[0] = static_size<T>;
        if constexpr (mdrange_dimension<T> > 1)
            Detail::set_sub_extents<std::ranges::range_value_t<T>>(extents.begin() + 1);
        return MDLayout{extents};
    }
}

/*!
 * \ingroup Common
 * \brief Get the layout for a range consisting of n instances of the range (or scalar)
 *        given as template argument, whose size is known at compile-time.
 */
template<typename T> requires(Concepts::StaticallySizedRange<T> or Concepts::Scalar<T>)
constexpr MDLayout get_md_layout(std::size_t n) {
    if constexpr (Concepts::Scalar<T>)
        return MDLayout{{n}};
    else {
        std::array<std::size_t, mdrange_dimension<T> + 1> extents;
        extents[0] = n;
        Detail::set_sub_extents<T>(extents.begin() + 1);
        return MDLayout{extents};
    }
}

/*!
 * \ingroup Common
 * \brief Get the multi-dimensional layout for the given range
 */
template<std::ranges::range R>
constexpr MDLayout get_md_layout(R&& r) {
    return get_md_layout<std::ranges::range_value_t<R>>(Ranges::size(r));
}

/*!
 * \ingroup Common
 * \brief Overload for scalars
 */
template<Concepts::Scalar T>
constexpr MDLayout get_md_layout(const T&) {
    return MDLayout{};
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MD_LAYOUT_HPP_
