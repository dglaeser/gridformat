// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_SERIALIZATION_HPP_
#define GRIDFORMAT_COMMON_SERIALIZATION_HPP_

#include <vector>
#include <cstddef>
#include <concepts>
#include <type_traits>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/traits.hpp>

namespace GridFormat {

class Serialization {
 public:
    using Byte = std::byte;

    Serialization() = default;
    explicit Serialization(std::integral auto size)
    : _data(size)
    {}

    std::size_t size() const { return _data.size(); }
    void resize(std::size_t size) { _data.resize(size); }

    std::byte* data() { return _data.data(); }
    const std::byte* data() const { return _data.data(); }

 private:
    std::vector<std::byte> _data;
};

namespace Traits {

template<>
struct Byte<Serialization> : public std::type_identity<typename Serialization::Byte> {};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SERIALIZATION_HPP_