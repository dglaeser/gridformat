// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <numeric>
#include <iterator>
#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <string>

#include <gridformat/common/reserved_vector.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

    template<Concepts::StaticallySizedRange R, typename Iterator>
    constexpr void set_sub_extents(Iterator it) {
        *it = static_size<R>;
        ++it;
        if constexpr (has_sub_range<R>)
            set_sub_extents<std::ranges::range_value_t<R>>(it);
    }

    template<typename T> requires(Concepts::StaticallySizedRange<T> or Concepts::Scalar<T>)
    auto get_md_extents() {
        if constexpr (Concepts::Scalar<T>)
            return std::array<std::size_t, 0>{};
        else {
            std::array<std::size_t, mdrange_dimension<T>> extents;
            extents[0] = static_size<T>;
            if constexpr (mdrange_dimension<T> > 1)
                Detail::set_sub_extents<std::ranges::range_value_t<T>>(extents.begin() + 1);
            return extents;
        }
    }

    template<typename T> requires(std::ranges::range<T> and !Concepts::StaticallySizedRange<T>)
    auto get_md_extents(const T& range) {
        static_assert(
            Concepts::StaticallySizedRange<std::ranges::range_value_t<T>>,
            "Sub-ranges must be statically sized in order to determine md layouts from md ranges"
        );
        std::array<std::size_t, mdrange_dimension<T>> extents;
        extents[0] = Ranges::size(range);
        Detail::set_sub_extents<std::ranges::range_value_t<T>>(extents.begin() + 1);
        return extents;
    }

}  // namespace Detail
#endif  // DOXYGEN

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
        if constexpr (!std::is_lvalue_reference_v<R>)
            std::ranges::move(std::move(extents), std::back_inserter(_extents));
        else
            std::ranges::copy(extents, std::back_inserter(_extents));
    }

    template<std::integral T>
    explicit MDLayout(const std::initializer_list<T>& extents) {
        _extents.reserve(Ranges::size(extents));
        std::ranges::copy(extents, std::back_inserter(_extents));
    }

    auto begin() const { return _extents.begin(); }
    auto begin() { return _extents.begin(); }

    auto end() const { return _extents.end(); }
    auto end() { return _extents.end(); }

    std::size_t dimension() const {
        return _extents.size();
    }

    std::size_t extent(unsigned int codim) const {
        return _extents.at(codim);
    }

    std::size_t number_of_entries() const {
        if (_is_scalar())
            return 1;
        return std::accumulate(
            _extents.begin(),
            _extents.end(),
            std::size_t{1},
            std::multiplies{}
        );
    }

    std::size_t number_of_entries(unsigned int codim) const {
        return sub_layout(codim).number_of_entries();
    }

    MDLayout sub_layout(unsigned int codim) const {
        if (codim >= dimension())
            throw ValueError(
                "Given codim " + std::to_string(codim)
                + " exceeds dimensions (" + std::to_string(dimension()) + ")"
            );
        return MDLayout{std::vector<std::size_t>{
            _extents.begin() + codim,
            _extents.end()
        }};
    }

    bool operator==(const MDLayout& other) const {
        return std::ranges::equal(_extents, other._extents);
    }

    template<typename T> requires(Concepts::StaticallySizedRange<T> or Concepts::Scalar<T>)
    MDLayout& with_sub_layout_from() {
        auto sub_extents = Detail::get_md_extents<T>();
        _extents.reserve(_extents.size() + sub_extents.size());
        std::ranges::move(std::move(sub_extents), std::back_inserter(_extents));
        return *this;
    }

    template<std::ranges::range T>
    MDLayout& with_sub_layout_from(T&& range) {
        auto sub_extents = Detail::get_md_extents<T>(std::forward<T>(range));
        _extents.reserve(_extents.size() + sub_extents.size());
        std::ranges::move(std::move(sub_extents), std::back_inserter(_extents));
        return *this;
    }

    MDLayout& with_sub_layout(const MDLayout& layout) {
        _extents.reserve(_extents.size() + layout.dimension());
        for (std::size_t i = 0; i < layout.dimension(); ++i)
            _extents.push_back(layout.extent(i));
        return *this;
    }

    template<std::ranges::range R>
    void export_to(R&& out) const {
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
    bool _is_scalar() const {
        return _extents.size() == 0;
    }

    ReservedVector<std::size_t, buffered_dimensions> _extents;
};


/*!
 * \ingroup Common
 * \brief Get the layout of a range (or a scalar) whose size is known at compile-time
 */
template<typename T> requires(Concepts::StaticallySizedRange<T> or Concepts::Scalar<T>)
MDLayout get_md_layout() {
    return MDLayout{Detail::get_md_extents<T>()};
}

/*!
 * \ingroup Common
 * \brief Get the layout for a range consisting of n instances of the range (or scalar)
 *        given as template argument, whose size is known at compile-time.
 */
template<typename T> requires(Concepts::StaticallySizedRange<T> or Concepts::Scalar<T>)
[[deprecated("use MDLayout{{n}}.with_sub_layout_from<T>() instead.")]]
MDLayout get_md_layout(std::size_t n) {
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
    requires(
        Concepts::StaticallySizedRange<std::ranges::range_value_t<R>> or
        Concepts::Scalar<std::ranges::range_value_t<R>>
    )
MDLayout get_md_layout(R&& r) {
    return MDLayout{{Ranges::size(r)}}.template with_sub_layout_from<std::ranges::range_value_t<R>>();
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
