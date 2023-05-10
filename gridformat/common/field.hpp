// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::Field
 */
#ifndef GRIDFORMAT_COMMON_FIELD_HPP_
#define GRIDFORMAT_COMMON_FIELD_HPP_

#include <memory>
#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/ranges.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields of values.
 */
class Field {
 public:
    virtual ~Field() = default;

    MDLayout layout() const {
        return _layout();
    }

    DynamicPrecision precision() const {
        return _precision();
    }

    std::size_t size_in_bytes() const {
        return layout().number_of_entries()*precision().size_in_bytes();
    }

    Serialization serialized() const {
        auto result = _serialized();
        if (result.size() != size_in_bytes())
            throw SizeError("Serialized size does not match expected number of bytes");
        return result;
    }

    template<typename Visitor>
    decltype(auto) visit_field_values(Visitor&& visitor) const {
        return precision().visit([&] <typename T> (const Precision<T>&) {
            const auto serialization = serialized();
            return visitor(serialization.template as_span_of<T>());
        });
    }

    template<std::ranges::range R> requires(Concepts::Scalar<MDRangeValueType<R>>)
    void export_to(R&& output_range) const {
        const auto serialization = serialized();
        const auto my_layout = layout();
        const auto input_range_layout = get_md_layout(output_range);

        if (input_range_layout.number_of_entries() < my_layout.number_of_entries())
            throw TypeError(
                std::string{"Cannot fill the given range. Too few entries. "} +
                "Number of field entries: '" + std::to_string(my_layout.number_of_entries()) + "'; " +
                "Number of range entries: '" + std::to_string(input_range_layout.number_of_entries()) + "'"
            );

        std::size_t offset = 0;
        visit_field_values([&] <typename T> (std::span<const T> data) {
            _export_to(output_range, data, offset);
        });
    }

    template<Concepts::Scalar S>
    void export_to(S& out) const {
        const auto my_layout = layout();
        const auto serialization = serialized();
        visit_field_values([&] <typename T> (std::span<const T> data) {
            if (my_layout.number_of_entries() != 1)
                throw ValueError("Field cannot be exported into a scalar");
            out = static_cast<S>(data[0]);
        });
    }

 private:
    DynamicPrecision _prec;

    virtual MDLayout _layout() const = 0;
    virtual DynamicPrecision _precision() const = 0;
    virtual Serialization _serialized() const = 0;

    template<std::ranges::range R, Concepts::Scalar T>
    void _export_to(R& range,
                    std::span<const T> data,
                    std::size_t& offset) const {
        if constexpr (mdrange_dimension<R> > 1)
            std::ranges::for_each(range, [&] (std::ranges::range auto& sub_range) {
                _export_to(sub_range, data, offset);
            });
        else
            std::ranges::for_each(range, [&] <typename V> (V& value) {
                value = static_cast<V>(data[offset++]);
            });
    }
};

using FieldPtr = std::shared_ptr<const Field>;

template<typename F> requires(
    std::derived_from<std::decay_t<F>, Field> and
    !std::is_lvalue_reference_v<F>)
FieldPtr make_shared(F&& f) {
    return std::make_shared<F>(std::forward<F>(f));
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_
