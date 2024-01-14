// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <ranges>
#include <cmath>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>

namespace GridFormat {

//! \addtogroup Common
//! \{

/*!
 * \brief Abstract interface for fields of values that is used by writers/readers to store fields.
 * \details Allows you to obtain information on the layout of the field, the precision of its value type,
 *          and to retrieve its values in serialized form or to export them into containers.
 */
class Field {
 public:
    //! Can be used as a flag to disable resizing upon export (e.g. to write to the beginning of larger range)
    static constexpr struct DisableResize {} no_resize{};

    virtual ~Field() = default;

    //! Return the layout of this field
    MDLayout layout() const {
        return _layout();
    }

    //! Return the precision of the scalar field values
    DynamicPrecision precision() const {
        return _precision();
    }

    //! Return the size of all field values in serialized form
    std::size_t size_in_bytes() const {
        return layout().number_of_entries()*precision().size_in_bytes();
    }

    //! Return the field values in serialized form
    Serialization serialized() const {
        auto result = _serialized();
        if (result.size() != size_in_bytes())
            throw SizeError("Serialized size does not match expected number of bytes");
        return result;
    }

    //! Visit the scalar values of the field in the form of an std::span
    template<typename Visitor>
    decltype(auto) visit_field_values(Visitor&& visitor) const {
        return precision().visit([&] <typename T> (const Precision<T>&) {
            const auto serialization = serialized();
            return visitor(serialization.template as_span_of<T>());
        });
    }

    //! Export the field values into the provided range, resize if necessary, and return it
    template<std::ranges::range R> requires(Concepts::Scalar<MDRangeValueType<R>>)
    decltype(auto) export_to(R&& output_range) const {
        return _export_to<true>(std::forward<R>(output_range));
    }

    //! Export the field values into the provided range without resizing (given range must be large enough)
    template<std::ranges::range R> requires(Concepts::Scalar<MDRangeValueType<R>>)
    decltype(auto) export_to(R&& output_range, DisableResize) const {
        return _export_to<false>(std::forward<R>(output_range));
    }

    //! Export the field as a scalar (works only if the field is a scalar field)
    template<Concepts::Scalar S>
    void export_to(S& out) const {
        const auto my_layout = layout();
        const auto serialization = serialized();
        if (my_layout.number_of_entries() != 1)
            throw TypeError("Field cannot be exported into a scalar");
        visit_field_values([&] <typename T> (std::span<const T> data) {
            out = static_cast<S>(data[0]);
        });
    }

    //! Export the field as a scalar (works only if the field is a scalar field)
    template<Concepts::Scalar S>
    S export_to() const {
        S out;
        export_to(out);
        return out;
    }

    //! Export the field into a resizable range (e.g. std::vector)
    template<Concepts::ResizableMDRange R>
        requires(Concepts::StaticallySizedMDRange<std::ranges::range_value_t<R>> or
                 Concepts::Scalar<std::ranges::range_value_t<R>>)
    R export_to() const {
        return export_to(R{});
    }

 private:
    DynamicPrecision _prec;

    virtual MDLayout _layout() const = 0;
    virtual DynamicPrecision _precision() const = 0;
    virtual Serialization _serialized() const = 0;

    //! Export the field values into the provided range and return it
    template<bool enable_resize, std::ranges::range R> requires(Concepts::Scalar<MDRangeValueType<R>>)
    decltype(auto) _export_to(R&& output_range) const {
        const auto my_layout = layout();

        if constexpr (Concepts::ResizableMDRange<R> && enable_resize) {
            const auto num_scalars = my_layout.number_of_entries();
            const auto num_sub_scalars = get_md_layout<std::ranges::range_value_t<R>>().number_of_entries();
            if (num_scalars%num_sub_scalars != 0)
                throw TypeError(
                    "Cannot export the field into the given range type. "
                    "Number of entries in the field is not divisible by the "
                    "number of entries in the value_type of the provided range."
                );

            output_range.resize(
                num_scalars/num_sub_scalars,
                DefaultValue<std::ranges::range_value_t<R>>::get()
            );
        } else {
            const auto output_range_layout = get_md_layout(output_range);
            if (output_range_layout.number_of_entries() < my_layout.number_of_entries())
                throw SizeError(
                    std::string{"Cannot fill the given range. Too few entries. "} +
                    "Number of field entries: '" + std::to_string(my_layout.number_of_entries()) + "'; " +
                    "Number of range entries: '" + std::to_string(output_range_layout.number_of_entries()) + "'"
                );
        }

        std::size_t offset = 0;
        visit_field_values([&] <typename T> (std::span<const T> data) {
            _export_to(output_range, data, offset);
        });

        return std::forward<R>(output_range);
    }

    template<std::ranges::range R, Concepts::Scalar T>
    void _export_to(R& range,
                    std::span<const T> data,
                    std::size_t& offset) const {
        if constexpr (mdrange_dimension<R> > 1)
            std::ranges::for_each(range, [&] (std::ranges::range auto& sub_range) {
                _export_to(sub_range, data, offset);
            });
        else
            std::ranges::for_each_n(
                std::ranges::begin(range),
                std::min(Ranges::size(range), data.size() - offset),
                [&] <typename V> (V& value) {
                    value = static_cast<V>(data[offset++]);
                }
            );
    }
};

//! Pointer type used by writers/readers for fields
using FieldPtr = std::shared_ptr<const Field>;

//! Factory function for field pointers
template<typename F> requires(
    std::derived_from<std::remove_cvref_t<F>, Field> and
    !std::is_lvalue_reference_v<F>)
FieldPtr make_field_ptr(F&& f) {
    return std::make_shared<std::add_const_t<F>>(std::forward<F>(f));
}

//! \}  group Common

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_
