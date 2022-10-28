// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_DATA_BUFFER_HPP_
#define GRIDFORMAT_COMMON_DATA_BUFFER_HPP_

#include <cstddef>
#include <vector>
#include <ranges>
#include <concepts>
#include <algorithm>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
template<typename T>
class Buffer {
 public:
    using BufferedType = T;

    Buffer(T const* data, std::size_t size)
    : _data(data)
    , _size(size)
    {}

    T const* data() const { return _data; }
    std::size_t size() const { return _size; }

    auto begin() const { return _data; }
    auto end() const { return _data + _size; }

 private:
    T const* _data;
    std::size_t _size;
};

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
template<typename T>
class OwningBuffer {
 public:
    using BufferedType = T;

    explicit OwningBuffer(std::size_t size)
    : _data(size)
    {}

    template<std::ranges::range R> requires(
        std::convertible_to<T, std::ranges::range_value_t<R>>)
    void fill(R&& input_range) {
        std::ranges::copy_n(
            std::ranges::begin(input_range),
            size(),
            _data.begin()
        );
    }

    T const* data() const { return _data.data(); }
    std::size_t size() const { return _data.size(); }
    void resize(std::size_t size) { _data.resize(size); }

    auto begin() const { return _data.begin(); }
    auto end() const { return _data.end(); }

 private:
    std::vector<T> _data;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_DATA_BUFFER_HPP_