// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::LazyField
 */
#ifndef GRIDFORMAT_COMMON_LAZY_FIELD_HPP_
#define GRIDFORMAT_COMMON_LAZY_FIELD_HPP_

#include <utility>
#include <concepts>
#include <type_traits>

#include <gridformat/common/field.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Field implementation that reads values lazily from a source upon request.
 */
template<typename S>
class LazyField : public Field {
 public:
    using SourceType = std::remove_cvref_t<S>;
    using SourceReferenceType = std::add_lvalue_reference_t<std::add_const_t<SourceType>>;
    using SerializationCallBack = std::function<Serialization(SourceReferenceType)>;

    template<typename _S, typename CallBack>
        requires(std::constructible_from<SerializationCallBack, CallBack> and
                 std::convertible_to<_S, S>)
    explicit LazyField(_S&& source,
                       MDLayout layout,
                       DynamicPrecision prec,
                       CallBack&& cb)
    : _source{std::forward<_S>(source)}
    , _md_layout{std::move(layout)}
    , _scalar_precision{std::move(prec)}
    , _serialization_callback{std::forward<CallBack>(cb)}
    {}

 private:
    MDLayout _layout() const override {
        return _md_layout;
    }

    DynamicPrecision _precision() const override {
        return _scalar_precision;
    }

    Serialization _serialized() const override {
        return _serialization_callback(_source);
    }

    S _source;
    MDLayout _md_layout;
    DynamicPrecision _scalar_precision;
    SerializationCallBack _serialization_callback;
};

template<typename S, typename CB>
LazyField(S&&, MDLayout, DynamicPrecision, CB&&) -> LazyField<
    std::conditional_t<
        std::is_lvalue_reference_v<S>,
        S,
        std::remove_cvref_t<S>
    >
>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_LAZY_FIELD_HPP_
