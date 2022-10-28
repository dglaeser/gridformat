// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_TENSOR_FIELD_HPP_
#define GRIDFORMAT_COMMON_TENSOR_FIELD_HPP_

#include <concepts>
#include <cassert>
#include <utility>
#include <cstddef>
#include <ostream>
#include <algorithm>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/field.hpp>

namespace GridFormat {
namespace FieldType {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
class Tensor {
 public:
    using Dimensions = std::array<std::size_t, 2>;

    Tensor(const Tensor&) = default;
    Tensor(Tensor&&) = default;
    explicit Tensor(Dimensions dimensions)
    : _dimensions(std::move(dimensions)) {
        assert(is_scalar() || is_vector() || is_matrix());
    }

    const Dimensions& dimensions() const { return _dimensions; }
    bool is_scalar() const { return _dimensions[0] == 1 && _dimensions[1] == 1; }
    bool is_vector() const { return _dimensions[0] > 1 && _dimensions[1] == 1; }
    bool is_matrix() const { return _dimensions[0] > 1 && _dimensions[1] > 1; }

    std::size_t num_components() const {
        if (is_scalar()) return 1;
        else if (is_vector()) return _dimensions[0];
        else return _dimensions[0]*_dimensions[1];
    }

 private:
    Dimensions _dimensions;
};

}  // namespace FieldType

#ifndef DOXYGEN_SKIP_DETAILS
namespace Detail {

template<std::ranges::range R>
constexpr void _get_md_range_extents(const R& range,
                                     std::array<std::size_t, mdrange_dimension<R>>& extents,
                                     unsigned int current_dimension) {
    extents[current_dimension] = std::distance(std::ranges::cbegin(range), std::ranges::cend(range));
    if constexpr (has_sub_range<R>) {
        using SubRangeType = std::ranges::range_value_t<R>;
        constexpr std::size_t sub_range_dim = mdrange_dimension<SubRangeType>;

        std::array<std::size_t, sub_range_dim> sub_extents;
        _get_md_range_extents(*std::ranges::cbegin(range), sub_extents, 0);

        // this algorithm expects all entries of the range to have the same dimensions
        assert(std::ranges::all_of(range, [&] (const auto& sub_range) {
            std::array<std::size_t, sub_range_dim> _sub_extents;
            _get_md_range_extents(sub_range, _sub_extents, 0);
            return std::ranges::equal(sub_extents, _sub_extents);
        }));

        std::copy(sub_extents.begin(),
                  sub_extents.end(),
                  extents.begin() + current_dimension + 1);
    }
}

template<std::ranges::range R>
constexpr auto _get_md_range_dimensions(const R& range) {
    std::array<std::size_t, mdrange_dimension<R>> result;
    _get_md_range_extents(range, result, 0);
    return result;
}

}  // end namespace Detail
#endif  // DOXYGEN_SKIP_DETAILS


/*!
 * \ingroup Common
 * \brief TODO: Doc me.
 */
template<std::ranges::view RangeView> requires(mdrange_dimension<RangeView> <= 2)
class TensorField : public Field<GridFormat::FieldType::Tensor> {
    using FT = GridFormat::FieldType::Tensor;
    using ParentType = Field<FT>;

 public:
    explicit TensorField(RangeView&& view)
    : ParentType(_make_field_type(Detail::_get_md_range_dimensions(view)))
    , _view(std::move(view))
    {}

 private:
    template<std::integral T, std::integral auto dim>
    FT _make_field_type(const std::array<T, dim>& view_dimensions) const {
        static_assert(dim >= 0 && dim <= 3);
        if constexpr (dim == 1)
            return FT{{1, 1}};
        if constexpr (dim == 2)
            return FT{{view_dimensions[1], 1}};
        else
            return FT{{view_dimensions[1], view_dimensions[2]}};
    }

    RangeView _view;
    virtual void _stream(std::ostream&) const {}
    virtual Serialization _serialize() const { return {}; }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TENSOR_FIELD_HPP_