// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_STREAM_HPP_
#define GRIDFORMAT_COMMON_STREAM_HPP_

#include <span>
#include <limits>
#include <ostream>
#include <concepts>

#include <gridformat/common/concepts.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
class OutputStream {
 public:
    explicit OutputStream(std::ostream& s)
    : _stream(s)
    {}

    template<Concepts::Streamable<std::ostream> T>
    requires(not Concepts::Scalar<T>)
    OutputStream& operator<<(const T& t) {
        _stream << t;
        return *this;
    }

    template<Concepts::Scalar T>
    OutputStream& operator<<(const T& t) {
        auto orig_precision = _stream.precision();
        _stream.precision(std::numeric_limits<T>::digits10);
        _stream << t;
        _stream.precision(orig_precision);
        return *this;
    }

    template<typename T, std::size_t size>
    void write(std::span<const T, size> data) {
        _write(std::as_bytes(data));
    }

 private:
    template<std::size_t size>
    void _write(std::span<const std::byte, size> data) {
        const char* chars = reinterpret_cast<const char*>(data.data());
        _stream.write(chars, data.size());
    }

    std::ostream& _stream;
};

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
template<typename Stream>
class StreamWrapperBase {
 public:
    explicit StreamWrapperBase(Stream& s)
    : _stream(s)
    {}

 protected:
    template<typename T>
    requires(Concepts::FormattedStream<Stream, T>)
    void _write_formatted(const T& t) {
        _stream << t;
    }

    template<typename T, std::size_t size>
    requires(Concepts::Stream<Stream, T>)
    void _write_raw(std::span<const T, size> data) {
        _stream.write(data);
    }

    Stream& _stream;
};

//! Specialization for std ostreams
template<std::derived_from<std::ostream> Stream>
class StreamWrapperBase<Stream> {
 public:
    explicit StreamWrapperBase(Stream& s)
    : _stream(s)
    {}

 protected:
    template<typename T>
    void _write_formatted(const T& t) {
        _stream << t;
    }

    template<typename T, std::size_t size>
    void _write_raw(std::span<const T, size> data) {
        _stream.write(std::as_bytes(data));
    }

    template<std::size_t size>
    void _write_raw(std::span<const std::byte, size> bytes) {
        const char* chars = reinterpret_cast<const char*>(bytes.data());
        _stream.write(chars, bytes.size());
    }

    OutputStream _stream;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STREAM_HPP_