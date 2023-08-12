// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::BufferField
 */
#ifndef GRIDFORMAT_COMMON_BUFFER_FIELD_HPP_
#define GRIDFORMAT_COMMON_BUFFER_FIELD_HPP_

#include <ranges>
#include <utility>

#include <gridformat/common/field.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

//! Field implementation around a flat buffer and corresponding layout
template<Concepts::Scalar T>
class BufferField : public Field {
 public:
    template<Concepts::MDRange<1> R>
        requires(Concepts::Scalar<std::ranges::range_value_t<R>>)
    explicit BufferField(R&& data, MDLayout&& layout)
    : _serialization(layout.number_of_entries()*sizeof(T))
    , _md_layout{std::move(layout)},
    _span{_serialization.template as_span_of<T>()} {
        if (data.size() != _md_layout.number_of_entries())
            throw SizeError("Given buffer size does not match layout");
        std::ranges::move(std::move(data), _serialization.as_span_of<T>().data());
    }

    std::size_t number_of_entries() const {
        return _span.size();
    }

 private:
    MDLayout _layout() const {
        return _md_layout;
    }

    DynamicPrecision _precision() const {
        return {Precision<T>{}};
    }

    Serialization _serialized() const {
        return _serialization;
    }

    Serialization _serialization;
    MDLayout _md_layout;
    std::span<T> _span;
};

template<Concepts::MDRange<1> R>
    requires(Concepts::Scalar<std::ranges::range_value_t<R>>)
BufferField(R&&, MDLayout&&) -> BufferField<std::ranges::range_value_t<R>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_BUFFER_FIELD_HPP_
