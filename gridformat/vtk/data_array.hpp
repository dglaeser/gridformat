// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_DATA_ARRAY_HPP_
#define GRIDFORMAT_VTK_DATA_ARRAY_HPP_

#include <utility>
#include <ostream>
#include <concepts>
#include <vector>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/encoding/ascii.hpp>

namespace GridFormat::VTK {

template<typename Encoder,
         typename Compressor,
         typename HeaderType>
class DataArray {
    static constexpr bool do_compression = !std::is_same_v<Compressor, None>;

 public:
    DataArray(const Field& field,
              Encoder encoder,
              Compressor compressor,
              [[maybe_unused]] const Precision<HeaderType>& = {})
    : _field(field)
    , _encoder{std::move(encoder)}
    , _compressor{std::move(compressor)}
    {}

    friend std::ostream& operator<<(std::ostream& s, const DataArray& da) {
        da.stream(s);
        return s;
    }

    void stream(std::ostream& s) const {
        if constexpr (std::is_same_v<Encoder, GridFormat::Encoding::Ascii>)
            _export_ascii(s, Encoding::ascii.with({
                .delimiter = " ",
                .line_prefix = std::string(10, ' '),
                .entries_per_line = 10
            }));
        else if constexpr (std::is_same_v<Encoder, GridFormat::Encoding::AsciiWithOptions>)
            _export_ascii(s, _encoder);
        else if constexpr (do_compression)
            _export_compressed_binary(s);
        else
            _export_binary(s);
    }

 private:
    template<typename _Enc>
    void _export_ascii(std::ostream& s, _Enc encoder) const {
        s << StreamableField{_field, encoder};
    }

    void _export_binary(std::ostream& s) const {
        auto encoded = _encoder(s);
        std::array<const HeaderType, 1> number_of_bytes{static_cast<HeaderType>(_field.size_in_bytes())};
        encoded.write(std::span{number_of_bytes});
        s << StreamableField{_field, _encoder};
    }

    void _export_compressed_binary(std::ostream& s) const requires(Concepts::Compressor<Compressor>) {
        _field.precision().visit([&] <typename T> (const Precision<T>&) {
            auto encoded = _encoder(s);
            Serialization serialization = _field.serialized();
            const auto blocks = _compressor.template compress<HeaderType>(serialization);

            std::vector<HeaderType> header;
            header.reserve(blocks.compressed_block_sizes.size() + 3);
            header.push_back(blocks.number_of_blocks);
            header.push_back(blocks.block_size);
            header.push_back(blocks.residual_block_size);
            std::ranges::copy(blocks.compressed_block_sizes, std::back_inserter(header));
            encoded.write(std::span{header});
            encoded.write(serialization.as_span());
        });
    }

    const Field& _field;
    Encoder _encoder;
    Compressor _compressor;
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_DATA_ARRAY_HPP_