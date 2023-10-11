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
#include <algorithm>
#include <iterator>

namespace GridFormat {

template<typename T, std::size_t N>
class ReservedVector {
    using Vector = std::pmr::vector<T>;

 public:
    using value_type = T;

    ReservedVector() = default;
    ReservedVector(std::size_t n, const T& r) : ReservedVector() { _elements.resize(n, r); }
    ReservedVector(std::initializer_list<T>&& initList) : ReservedVector() { _copy_back(initList); }

    // We need both template and non-template variant, because if only the template is present, the
    // compiler tries to use the default copy ctor (which is deleted). Same for move ctor & assignments.
    template<std::size_t M>
    ReservedVector(const ReservedVector<T, M>& other) : ReservedVector() { _copy_back(other); }
    ReservedVector(const ReservedVector& other) : ReservedVector() { _copy_back(other); }

    template<std::size_t M>
    ReservedVector(ReservedVector<T, M>&& other) : ReservedVector() { _move_back(std::move(other)); }
    ReservedVector(ReservedVector&& other) : ReservedVector() { _move_back(std::move(other)); }

    ReservedVector& operator=(const ReservedVector& other) {
        _elements = Vector{typename Vector::allocator_type{&_resource}};
        _copy_back(other);
        return *this;
    }

    template<std::size_t M>
    ReservedVector& operator=(const ReservedVector<T, M>& other) {
        _elements = Vector{typename Vector::allocator_type{&_resource}};
        _copy_back(other);
        return *this;
    }

    ReservedVector& operator=(ReservedVector&& other) {
        _elements = Vector{typename Vector::allocator_type{&_resource}};
        _move_back(std::move(other));
        return *this;
    }

    template<std::size_t M>
    ReservedVector& operator=(ReservedVector<T, M>&& other) {
        _elements = Vector{typename Vector::allocator_type{&_resource}};
        _move_back(std::move(other));
        return *this;
    }

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
    template<std::ranges::sized_range R>
    void _copy_back(const R& other) {
        _elements.reserve(std::ranges::size(other));
        std::ranges::copy(other, std::back_inserter(_elements));
    }

    template<std::ranges::sized_range R> requires(!std::is_lvalue_reference_v<R>)
    void _move_back(R&& other) {
        _elements.reserve(std::ranges::size(other));
        std::ranges::move(std::move(other), std::back_inserter(_elements));
    }

    std::array<std::byte, N*sizeof(T)> _buffer;
    std::pmr::monotonic_buffer_resource _resource{_buffer.data(), _buffer.size()};
    Vector _elements{typename Vector::allocator_type{&_resource}};
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_RESERVED_VECTOR_HPP_
