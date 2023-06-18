// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::RangeField
 */
#ifndef GRIDFORMAT_COMMON_RANGE_FIELD_HPP_
#define GRIDFORMAT_COMMON_RANGE_FIELD_HPP_

#include <concepts>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <iterator>

#include <gridformat/common/field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/type_traits.hpp>

namespace GridFormat {

template<std::ranges::forward_range R, Concepts::Scalar T = MDRangeScalar<R>>
class RangeField : public Field {
 public:
    template<std::convertible_to<R> _R>
    explicit RangeField(_R&& r, const Precision<T>& = {})
    : _range{std::forward<_R>(r)}
    {}

 private:
    MDLayout _layout() const {
        return get_md_layout<std::ranges::range_value_t<R>>(Ranges::size(_range));
    }

    DynamicPrecision _precision() const {
        return {Precision<T>{}};
    }

    Serialization _serialized() const {
        const std::size_t num_bytes = _layout().number_of_entries()*sizeof(T);
        Serialization result(num_bytes);
        auto it = result.as_span().begin();
        _fill(it, _range);
        return result;
    }

    template<std::output_iterator<std::byte> It, std::ranges::range _R>
    void _fill(It& out, _R&& r) const {
        std::ranges::for_each(r, [&] (const auto& entry) { _fill(out, entry); });
    }

    template<std::output_iterator<std::byte> It, Concepts::Scalar RV>
    void _fill(It& out, const RV& range_value) const {
        const T value = static_cast<T>(range_value);
        const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
        out = std::copy_n(bytes, sizeof(T), out);
    }

    R _range;
};

template<std::ranges::range R> requires(std::is_lvalue_reference_v<R>)
RangeField(R&&) -> RangeField<std::add_const_t<R>>;
template<std::ranges::range R, Concepts::Scalar T> requires(std::is_lvalue_reference_v<R>)
RangeField(R&&, const Precision<T>&) -> RangeField<std::add_const_t<R>, T>;

template<std::ranges::range R> requires(!std::is_lvalue_reference_v<R>)
RangeField(R&&) -> RangeField<std::remove_cvref_t<R>>;
template<std::ranges::range R, Concepts::Scalar T> requires(!std::is_lvalue_reference_v<R>)
RangeField(R&&, const Precision<T>&) -> RangeField<std::remove_cvref_t<R>, T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_RANGE_FIELD_HPP_
