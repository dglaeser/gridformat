// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::Serialization
 */
#ifndef GRIDFORMAT_COMMON_SERIALIZATION_HPP_
#define GRIDFORMAT_COMMON_SERIALIZATION_HPP_

#include <vector>
#include <cstddef>
#include <algorithm>
#include <span>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Represents the serialization (vector of bytes) of an object
 */
class Serialization {
 public:
    using Byte = std::byte;

    Serialization() = default;

    explicit Serialization(std::size_t size)
    : _data{size}
    {}

    template<Concepts::Scalar T>
    static Serialization from_scalar(const T& value) {
        Serialization result{sizeof(value)};
        std::byte* out = result.as_span().data();
        const std::byte* value_bytes = reinterpret_cast<const std::byte*>(&value);
        std::copy_n(value_bytes, sizeof(value), out);
        return result;
    }

    std::span<std::byte> as_span() { return {_data}; }
    std::span<const std::byte> as_span() const { return {_data}; }

    std::size_t size() const {
        return _data.size();
    }

    void resize(std::size_t size, Byte value = Byte{0}) {
        _data.resize(size, value);
    }

    template<Concepts::Scalar T>
    std::span<T> as_span_of(const Precision<T>& = {}) {
        _check_valid_cast<T>();
        return std::span{reinterpret_cast<T*>(_data.data()), _data.size()/sizeof(T)};
    }

    template<Concepts::Scalar T>
    std::span<std::add_const_t<T>> as_span_of(const Precision<T>& = {}) const {
        _check_valid_cast<T>();
        return std::span{reinterpret_cast<std::add_const_t<T>*>(_data.data()), _data.size()/sizeof(T)};
    }

    operator std::span<const std::byte>() const { return {_data}; }
    operator std::span<std::byte>() { return {_data}; }

 private:
    template<typename T>
    void _check_valid_cast() const {
        if (_data.size()%sizeof(T) != 0)
            throw TypeError("Cannot cast to span of given type, size mismatch");
    }

    std::vector<std::byte> _data;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SERIALIZATION_HPP_
