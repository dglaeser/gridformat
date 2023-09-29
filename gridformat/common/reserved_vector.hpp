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
#include <iterator>

namespace GridFormat {

template<typename T, std::size_t N>
class ReservedVector {
    using Vector = std::pmr::vector<T>;

 public:
    using value_type = T;

    ReservedVector() = default;

    ReservedVector(const ReservedVector& other) : ReservedVector() {
        _elements.clear();
        _elements.reserve(other.size());
        std::copy(other._elements.begin(), other._elements.end(), std::back_inserter(_elements));
    }

    ReservedVector(std::size_t n, const T& r) : ReservedVector() {
        _elements.resize(n, r);
    }

    ReservedVector(std::initializer_list<T>&& initList) : ReservedVector() {
        _elements.resize(initList.size());
        std::copy(initList.begin(), initList.end(), _elements.begin());
    }

    ReservedVector(ReservedVector&& other) : ReservedVector() {
        _elements.clear();
        _elements.reserve(other.size());
        std::move(other._elements.begin(), other._elements.end(), std::back_inserter(_elements));
    }

    ReservedVector& operator=(const ReservedVector& other) {
        _elements = Vector{typename Vector::allocator_type{&_resource}};
        _elements.reserve(other.size());
        std::copy(other._elements.begin(), other._elements.end(), std::back_inserter(_elements));
        return *this;
    };

    void clear() { _elements.clear(); }
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

    decltype(auto) operator[](std::size_t i) { return _elements[i]; }
    decltype(auto) operator[](std::size_t i) const { return _elements[i]; }

    decltype(auto) at(std::size_t i) { return _elements.at(i); }
    decltype(auto) at(std::size_t i) const { return _elements.at(i); }

 private:
    std::array<std::byte, N*sizeof(T)> _buffer;
    std::pmr::monotonic_buffer_resource _resource{_buffer.data(), _buffer.size()};
    Vector _elements{typename Vector::allocator_type{&_resource}};
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_RESERVED_VECTOR_HPP_
