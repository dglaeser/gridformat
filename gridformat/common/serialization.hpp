// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::Serialization
 */
#ifndef GRIDFORMAT_COMMON_SERIALIZATION_HPP_
#define GRIDFORMAT_COMMON_SERIALIZATION_HPP_

#include <vector>
#include <cstddef>
#include <cassert>
#include <span>

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

    std::size_t size() const { return _data.size(); }
    void resize(std::size_t size, Byte value = Byte{0}) { _data.resize(size, value); }

    std::span<std::byte> as_span() { return {_data}; }
    std::span<const std::byte> as_span() const { return {_data}; }

    template<typename T>
    std::span<T> as_span_of() {
        assert(_data.size()%sizeof(T) == 0);
        return std::span{reinterpret_cast<T*>(_data.data()), _data.size()/sizeof(T)};
    }

    template<typename T>
    std::span<std::add_const_t<T>> as_span_of() const {
        assert(_data.size()%sizeof(T) == 0);
        return std::span{reinterpret_cast<std::add_const_t<T>*>(_data.data()), _data.size()/sizeof(T)};
    }

    operator std::span<const std::byte>() const { return {_data}; }
    operator std::span<std::byte>() { return {_data}; }

 private:
    std::vector<std::byte> _data;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SERIALIZATION_HPP_
