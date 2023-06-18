# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

gridformat_add_test_if(GRIDFORMAT_HAVE_LZMA test_lzma_compression test_lzma_compression.cpp)
gridformat_add_test_if(GRIDFORMAT_HAVE_ZLIB test_zlib_compression test_zlib_compression.cpp)
gridformat_add_test_if(GRIDFORMAT_HAVE_LZ4 test_lz4_compression test_lz4_compression.cpp)
