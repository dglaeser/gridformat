// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Vector with preallocated memory.
 */
#ifndef GRIDFORMAT_COMMON_RESERVED_VECTOR_HPP_
#define GRIDFORMAT_COMMON_RESERVED_VECTOR_HPP_

#include <bit>
#include <array>
#include <vector>
#include <utility>
#include <memory_resource>

namespace GridFormat {

template<typename T, std::size_t N>
class ReservedVector {
 public:
    using value_type = T;

    ReservedVector(ReservedVector&&) = delete;
    ReservedVector(const ReservedVector& other) { *this = other; }

    ReservedVector(std::size_t n, const T& r) : ReservedVector() {
        _elements.resize(n, r);
    }

    ReservedVector()
    : _buffer{}
    , _resource{_buffer.data(), _buffer.size()}
    , _elements{&_resource}
    {}

    ReservedVector& operator=(const ReservedVector& other) {
        _elements.clear();
        _elements.resize(other.size());
        std::ranges::copy(other._elements, _elements.begin());
        return *this;
    }


    std::size_t size() const { return _elements.size(); }
    void reserve(std::size_t n) { _elements.reserve(n); }
    void resize(std::size_t n) { _elements.resize(n); }
    void resize(std::size_t n, const T& value) { _elements.resize(n, value); }

    void push_back(const T& element) { _elements.push_back(element); }
    void push_back(T&& element) { _elements.push_back(std::move(element)); }

    decltype(auto) begin() { return _elements.begin(); }
    decltype(auto) begin() const { return _elements.begin(); }

    decltype(auto) end() { return _elements.end(); }
    decltype(auto) end() const { return _elements.end(); }

    T& operator[](std::size_t i) { return _elements[i]; }
    const T& operator[](std::size_t i) const { return _elements[i]; }

 private:
    std::array<std::byte, N*sizeof(std::size_t)> _buffer;
    std::pmr::monotonic_buffer_resource _resource;
    std::pmr::vector<T> _elements;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_RESERVED_VECTOR_HPP_
