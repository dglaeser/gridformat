// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup API
 * \brief Includes all available compressors.
 */
#ifndef GRIDFORMAT_COMPRESSION_HPP_
#define GRIDFORMAT_COMPRESSION_HPP_

#include <gridformat/common/type_traits.hpp>

#include <gridformat/compression/lzma.hpp>
#include <gridformat/compression/zlib.hpp>
#include <gridformat/compression/lz4.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Compression::Detail {

#if GRIDFORMAT_HAVE_LZ4
using _LZ4 = LZ4;
#else
using _LZ4 = None;
#endif

#if GRIDFORMAT_HAVE_LZMA
using _LZMA = LZMA;
#else
using _LZMA = None;
#endif

#if GRIDFORMAT_HAVE_ZLIB
using _ZLIB = ZLIB;
#else
using _ZLIB = None;
#endif

}  // namespace Compression::Detail
#endif  // DOXYGEN

//! A variant holding any of the supported compressors
using Compressor = UniqueVariant<
    Compression::Detail::_LZ4,
    Compression::Detail::_LZMA,
    Compression::Detail::_ZLIB
>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMPRESSION_HPP_
