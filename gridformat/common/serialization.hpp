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
#include <iterator>
#include <span>
#include <bit>

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

    void push_back(std::vector<std::byte>&& bytes) {
        const auto size_before = size();
        _data.reserve(size_before + bytes.size());
        std::ranges::move(std::move(bytes), std::back_inserter(_data));
    }

    void cut_front(std::size_t number_of_bytes) {
        if (number_of_bytes > size())
            throw SizeError("Cannot cut more bytes than stored");
        const auto new_size = _data.size() - number_of_bytes;
        std::span trail{_data.data() + number_of_bytes, new_size};
        std::ranges::move(trail, _data.begin());
        _data.resize(new_size);
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

    std::vector<std::byte>&& data() && { return std::move(_data); }

 private:
    template<typename T>
    void _check_valid_cast() const {
        if (_data.size()%sizeof(T) != 0)
            throw TypeError("Cannot cast to span of given type, size mismatch");
    }

    std::vector<std::byte> _data;
};


//! Options for converting between byte orders
struct ByteOrderConversionOptions {
    std::endian from;
    std::endian to = std::endian::native;
};


//! Convert the byte order of all values in a span
template<Concepts::Scalar T>
void change_byte_order(std::span<T> values, const ByteOrderConversionOptions& opts) {
    if (opts.from == opts.to)
        return;

    std::size_t offset = 0;
    std::array<std::byte, sizeof(T)> buffer;
    auto bytes = std::as_writable_bytes(values);
    while (offset < bytes.size()) {
        std::ranges::copy_n(bytes.data() + offset, sizeof(T), buffer.begin());
        std::ranges::reverse(buffer);
        std::ranges::copy(buffer, bytes.data() + offset);
        offset += sizeof(T);
    }
}


}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SERIALIZATION_HPP_
