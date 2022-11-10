// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_MD_LAYOUT_HPP_
#define GRIDFORMAT_COMMON_MD_LAYOUT_HPP_

#include <utility>
#include <vector>
#include <cassert>
#include <numeric>
#include <algorithm>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>

namespace GridFormat {

class MDLayout {
 public:
    template<std::ranges::input_range R>
    explicit MDLayout(R&& extents)
    : _extents{std::ranges::cbegin(extents),
               std::ranges::cend(extents)}
    {}

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

 private:
    std::vector<std::size_t> _extents;
};

#ifndef DOXYGEN
namespace Detail {

template<Concepts::Scalar T>
constexpr auto push_extents(const T&, std::vector<std::size_t>&) {}

template<std::ranges::range R>
constexpr auto push_extents(R&& r, std::vector<std::size_t>& extents) {
    extents.push_back(Ranges::size(r));
    if constexpr (mdrange_dimension<R> > 1)
        assert(std::ranges::all_of(r,
            [&, first_size = Ranges::size(*std::ranges::begin(r))]
            (const std::ranges::range auto& sub_range) {
                return Ranges::size(sub_range) == first_size;
        }));
    push_extents(*std::ranges::begin(r), extents);
}

}  // namespace Detail
#endif  // DOXYGEN

template<Concepts::Scalar T>
MDLayout get_layout(const T&) {
    return MDLayout{std::vector<std::size_t>{1}};
}

template<std::ranges::range R>
MDLayout get_layout(const R& r) {
    std::vector<std::size_t> extents;
    Detail::push_extents(r, extents);
    return MDLayout{extents};
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MD_LAYOUT_HPP_