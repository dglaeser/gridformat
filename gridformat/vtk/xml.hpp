// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Helper classes and functions for VTK XML-type file format writers & readers.
 */
#ifndef GRIDFORMAT_VTK_XML_HPP_
#define GRIDFORMAT_VTK_XML_HPP_

#include <bit>
#include <string>
#include <ranges>
#include <utility>
#include <type_traits>
#include <functional>
#include <optional>
#include <iterator>
#include <string_view>
#include <concepts>

#include <gridformat/common/detail/crtp.hpp>
#include <gridformat/common/callable_overload_set.hpp>
#include <gridformat/common/optional_reference.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/variant.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/lazy_field.hpp>
#include <gridformat/common/path.hpp>

#include <gridformat/encoding/base64.hpp>
#include <gridformat/encoding/ascii.hpp>
#include <gridformat/encoding/raw.hpp>

#include <gridformat/compression/lz4.hpp>
#include <gridformat/compression/lzma.hpp>
#include <gridformat/compression/zlib.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/xml/element.hpp>
#include <gridformat/xml/parser.hpp>

#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/attributes.hpp>
#include <gridformat/vtk/data_array.hpp>
#include <gridformat/vtk/appendix.hpp>

namespace GridFormat::VTK {

#ifndef DOXYGEN
namespace XMLDetail {
    using LZMACompressor = std::conditional_t<Compression::Detail::_have_lzma, Compression::LZMA, None>;
    using ZLIBCompressor = std::conditional_t<Compression::Detail::_have_zlib, Compression::ZLIB, None>;
    using LZ4Compressor = std::conditional_t<Compression::Detail::_have_lz4, Compression::LZ4, None>;
    using Compressor = UniqueVariant<LZ4Compressor, ZLIBCompressor, LZMACompressor>;
}  // namespace XMLDetail

namespace XML {

using HeaderPrecision = std::variant<UInt32, UInt64>;
using CoordinatePrecision = std::variant<Float32, Float64>;
using Compressor = ExtendedVariant<XMLDetail::Compressor, None>;
using Encoder = std::variant<Encoding::Ascii, Encoding::Base64, Encoding::RawBinary>;
using DataFormat = std::variant<VTK::DataFormat::Inlined, VTK::DataFormat::Appended>;
using DefaultCompressor = std::variant_alternative_t<0, XMLDetail::Compressor>;

}  // namespace XML
#endif  // DOXYGEN

/*!
 * \ingroup VTK
 * \brief Options for VTK-XML files for setting the desired encoding, data format and compression.
 * \details The data format can be
 *          - GridFormat::VTK::DataFormat::inlined
 *          - GridFormat::VTK::DataFormat::appended
 *
 *          For encoding one can choose between
 *          - GridFormat::Encoding::ascii
 *          - GridFormat::Encoding::base64
 *          - GridFormat::Encoding::raw
 *
 *          Note, however, that ascii encoding only works with inlined data, and raw binary encoding only
 *          works with appended data. Finally, one can choose between three different compressors or
 *          GridFormat::none:
 *          - GridFormat::Compression::zlib
 *          - GridFormat::Compression::lz4
 *          - GridFormat::Compression::lzma
 *
 *          Note that these compressors are only available if the respective libraries were found.
 *          All options can also be set to GridFormat::automatic, in which case a suitable option
 *          is chosen.
 */
struct XMLOptions {
    using EncoderOption = ExtendedVariant<XML::Encoder, Automatic>;
    using CompressorOption = ExtendedVariant<XML::Compressor, Automatic>;
    using DataFormatOption = ExtendedVariant<XML::DataFormat, Automatic>;
    using CoordinatePrecisionOption = ExtendedVariant<XML::CoordinatePrecision, Automatic>;
    EncoderOption encoder = automatic;
    CompressorOption compressor = automatic;
    DataFormatOption data_format = automatic;
    CoordinatePrecisionOption coordinate_precision = automatic;
    XML::HeaderPrecision header_precision = _from_size_t();

 private:
    static constexpr XML::HeaderPrecision _from_size_t() {
        if constexpr(sizeof(std::size_t) == 8) return uint64;
        else return uint32;
    }
};


#ifndef DOXYGEN
namespace XMLDetail {

    struct XMLSettings {
        XML::Encoder encoder;
        XML::Compressor compressor;
        XML::DataFormat data_format;
        XML::CoordinatePrecision coordinate_precision;
        XML::HeaderPrecision header_precision;

        template<typename GridCoordinateType>
        static XMLSettings from(const XMLOptions& opts) {
            const auto _enc = _make_encoder(opts.encoder);
            const auto _format = _make_data_format(_enc, opts.data_format);
            const auto _comp = _make_compressor(_enc, opts.compressor);
            return {
                .encoder = _enc,
                .compressor = _comp,
                .data_format = _format,
                .coordinate_precision =
                    Variant::is<Automatic>(opts.coordinate_precision) ?
                        XML::CoordinatePrecision{Precision<GridCoordinateType>{}} :
                        Variant::without<Automatic>(opts.coordinate_precision),
                .header_precision = opts.header_precision
            };
        }

     private:
        static XML::Encoder _make_encoder(const typename XMLOptions::EncoderOption& enc) {
            if (Variant::is<Automatic>(enc))
                return Encoding::base64;
            return Variant::without<Automatic>(enc);
        }

        static XML::DataFormat _make_data_format(const typename XML::Encoder& enc,
                                                 const typename XMLOptions::DataFormatOption& data_format) {
            if (Variant::is<Automatic>(data_format))
                return Variant::is<Encoding::Ascii>(enc)
                    ? XML::DataFormat{VTK::DataFormat::inlined}
                    : XML::DataFormat{VTK::DataFormat::appended};
            return Variant::without<Automatic>(data_format);
        }

        static XML::Compressor _make_compressor(const typename XML::Encoder& enc,
                                                const typename XMLOptions::CompressorOption& compressor) {
            if (Variant::is<Automatic>(compressor))
                return Variant::is<Encoding::Ascii>(enc)
                    ? XML::Compressor{none}
                    : XML::Compressor{XML::DefaultCompressor{}};
            if (Variant::is<Encoding::Ascii>(enc) && !Variant::is<None>(compressor)) {
                log_warning("Ascii output cannot be compressed. Ignoring chosen compressor...");
                return none;
            }
            return Variant::without<Automatic>(compressor);
        }
    };

}  // namespace XMLDetail
#endif  // DOXYGEN


/*!
 * \ingroup VTK
 * \brief Base class for VTK-XML Writer implementations
 */
template<Concepts::Grid G, typename Impl>
class XMLWriterBase
: public GridWriter<G>
, public GridFormat::Detail::CRTPBase<Impl> {
    using ParentType = GridWriter<G>;
    using GridCoordinateType = CoordinateType<G>;

 public:
    //! Export underlying grid type
    using Grid = G;

    explicit XMLWriterBase(const Grid& grid,
                           std::string extension,
                           bool use_structured_grid_ordering,
                           XMLOptions xml_opts = {})
    : ParentType(grid, std::move(extension), WriterOptions{use_structured_grid_ordering, true})
    , _xml_opts{std::move(xml_opts)}
    , _xml_settings{XMLDetail::XMLSettings::from<GridCoordinateType>(_xml_opts)}
    {}

    Impl with(XMLOptions opts) const {
        auto result = _with(std::move(opts));
        this->copy_fields(result);
        return result;
    }

    Impl with_data_format(const XML::DataFormat& format) const {
        auto opts = _xml_opts;
        Variant::unwrap_to(opts.data_format, format);
        return with(std::move(opts));
    }

    Impl with_compression(const XML::Compressor& compressor) const {
        auto opts = _xml_opts;
        Variant::unwrap_to(opts.compressor, compressor);
        return with(std::move(opts));
    }

    Impl with_encoding(const XML::Encoder& encoder) const {
        auto opts = _xml_opts;
        Variant::unwrap_to(opts.encoder, encoder);
        return with(std::move(opts));
    }

    Impl with_coordinate_precision(const XML::CoordinatePrecision& prec) const {
        auto opts = _xml_opts;
        Variant::unwrap_to(opts.coordinate_precision, prec);
        return with(std::move(opts));
    }

    Impl with_header_precision(const XML::HeaderPrecision& prec) const {
        auto opts = _xml_opts;
        opts.header_precision = prec;
        return with(std::move(opts));
    }

 private:
    virtual Impl _with(XMLOptions opts) const = 0;

 protected:
    XMLOptions _xml_opts;
    XMLDetail::XMLSettings _xml_settings;

    struct WriteContext {
        std::string vtk_grid_type;
        XMLElement xml_representation;
        Appendix appendix;
    };

    WriteContext _get_write_context(std::string vtk_grid_type) const {
        return std::visit([&] (const auto& compressor) {
            return std::visit([&] (const auto& header_precision) {
                XMLElement xml("VTKFile");
                xml.set_attribute("type", vtk_grid_type);
                xml.set_attribute("version", "2.2");
                xml.set_attribute("byte_order", attribute_name(std::endian::native));
                xml.set_attribute("header_type", attribute_name(DynamicPrecision{header_precision}));
                if constexpr (!is_none<decltype(compressor)>)
                    xml.set_attribute("compressor", attribute_name(compressor));

                xml.add_child(vtk_grid_type).add_child("FieldData");
                WriteContext context{
                    .vtk_grid_type = std::move(vtk_grid_type),
                    .xml_representation = std::move(xml),
                    .appendix = {}
                };
                _add_meta_data_fields(context);
                return context;
            }, _xml_settings.header_precision);
        }, _xml_settings.compressor);
    }

    void _add_meta_data_fields(WriteContext& context) const {
        XMLElement& field_data = context.xml_representation
                                    .get_child(context.vtk_grid_type)
                                    .get_child("FieldData");
        std::visit([&] (const auto& encoder) {
            std::visit([&] (const auto& compressor) {
                std::visit([&] (const auto& data_format) {
                    std::visit([&] (const auto& header_precision) {
                        const auto& names = this->_meta_data_field_names();
                        std::ranges::for_each(names, [&] (const std::string& name) {
                            const auto& field = this->_get_meta_data_field(name);
                            const auto layout = field.layout();
                            const auto precision = field.precision();

                            auto& array = field_data.add_child("DataArray");
                            array.set_attribute("Name", name);
                            array.set_attribute("format", data_format_name(encoder, data_format));
                            if (precision.template is<char>() && layout.dimension() == 1) {
                                array.set_attribute("type", "String");
                                array.set_attribute("NumberOfTuples", 1);
                            } else {
                                array.set_attribute("NumberOfTuples", layout.extent(0));
                                array.set_attribute("type", attribute_name(precision));
                                array.set_attribute(
                                    "NumberOfComponents",
                                    layout.dimension() > 1
                                        ? layout.sub_layout(1).number_of_entries()
                                        : 1
                                );
                            }
                            DataArray content{field, encoder, compressor, header_precision};
                            _set_data_array_content(data_format, array, context.appendix, std::move(content));
                        });
                    }, _xml_settings.header_precision);
                }, _xml_settings.data_format);
            }, _xml_settings.compressor);
        }, _xml_settings.encoder);
    }

    template<typename ValueType>
    void _set_attribute(WriteContext& context,
                        std::string_view xml_group,
                        const std::string& attr_name,
                        const ValueType& attr_value) const {
        _access_at(xml_group, context).set_attribute(attr_name, attr_value);
    }

    void _set_data_array(WriteContext& context,
                         std::string_view xml_group,
                         std::string data_array_name,
                         const Field& field) const {
        const auto layout = field.layout();
        XMLElement& da = _access_at(xml_group, context).add_child("DataArray");
        da.set_attribute("Name", std::move(data_array_name));
        da.set_attribute("type", attribute_name(field.precision()));
        da.set_attribute("NumberOfComponents", (layout.dimension() == 1 ? 1 : layout.number_of_entries(1)));
        std::visit([&] (const auto& encoder) {
            std::visit([&] (const auto& compressor) {
                std::visit([&] (const auto& data_format) {
                    std::visit([&] (const auto& header_prec) {
                        da.set_attribute("format", data_format_name(encoder, data_format));
                        DataArray content{field, encoder, compressor, header_prec};
                        _set_data_array_content(data_format, da, context.appendix, std::move(content));
                    }, _xml_settings.header_precision);
                }, _xml_settings.data_format);
            }, _xml_settings.compressor);
        }, _xml_settings.encoder);
    }

    template<typename DataFormat, typename Appendix, typename Content>
        requires(!std::is_lvalue_reference_v<Content>)
    void _set_data_array_content(const DataFormat&,
                                 XMLElement& e,
                                 Appendix& app,
                                 Content&& c) const {
        static constexpr bool is_inlined = std::is_same_v<DataFormat, VTK::DataFormat::Inlined>;
        static constexpr bool is_appended = std::is_same_v<DataFormat, VTK::DataFormat::Appended>;
        static_assert(is_inlined || is_appended, "Unknown data format");

        if constexpr (is_inlined)
            e.set_content(std::move(c));
        else
            app.add(std::move(c));
    }

    XMLElement& _access_at(std::string_view path, WriteContext& context) const {
        return access_or_create_at(path, context.xml_representation.get_child(context.vtk_grid_type));
    }

    void _write_xml(WriteContext&& context, std::ostream& s) const {
        Indentation indentation{{.width = 2}};
        _set_default_active_fields(context.xml_representation.get_child(context.vtk_grid_type));
        std::visit([&] (const auto& encoder) {
            std::visit([&] <typename DataFormat> (const DataFormat&) {
                if constexpr (std::is_same_v<DataFormat, VTK::DataFormat::Inlined>)
                    write_xml_with_version_header(context.xml_representation, s, indentation);
                else
                    XML::Detail::write_with_appendix(std::move(context), s, encoder, indentation);
            }, this->_xml_settings.data_format);
        }, this->_xml_settings.encoder);
    }

    void _set_default_active_fields(XMLElement& xml) const {
        const auto set = [&] (std::string_view group, const auto& attr, const auto& name) -> void {
            auto group_element = access_at(group, xml);
            if (!group_element)
                return;
            group_element.unwrap().set_attribute(attr, name);
        };

        // discard vectors with more than 3 elements for active arrays
        const auto get_vector_filter = [] (unsigned int rank) {
            return [r=rank] (const auto& name_field_ptr_pair) {
                if (r == 1)
                    return name_field_ptr_pair.second->layout().extent(1) <= 3;
                return true;
            };
        };

        for (std::string_view group : {"Piece/PointData", "PPointData"})
            for (unsigned int i = 0; i <= 2; ++i)
                for (const auto& [n, _] : point_fields_of_rank(i, *this)
                                            | std::views::filter(get_vector_filter(i))
                                            | std::views::take(1))
                    set(group, active_array_attribute_for_rank(i), n);

        for (std::string_view group : {"Piece/CellData", "PCellData"})
            for (unsigned int i = 0; i <= 2; ++i)
                for (const auto& [n, _] : cell_fields_of_rank(i, *this)
                                            | std::views::filter(get_vector_filter(i))
                                            | std::views::take(1))
                    set(group, active_array_attribute_for_rank(i), n);
    }
};


#ifndef DOXYGEN
namespace XMLDetail {

    struct DataArrayStreamLocation {
        std::streamsize begin;                       // location where data begins
        std::optional<std::streamsize> offset = {};  // used for appended data
    };

    template<std::integral I, std::integral O>
    void _move_to_appendix_position(std::istream& stream, I appendix_begin, O offset_in_appendix) {
        InputStreamHelper helper{stream};
        helper.seek_position(appendix_begin);
        helper.shift_until_not_any_of(" \n\t");
        if (helper.read_chunk(1) != "_")
            throw IOError("VTK-XML appendix must start with '_'");
        helper.shift_by(offset_in_appendix);
    }

    void _move_to_data(const DataArrayStreamLocation& location, std::istream& s) {
        if (location.offset)
            _move_to_appendix_position(s, location.begin, location.offset.value());
        else {
            InputStreamHelper helper{s};
            helper.seek_position(location.begin);
            helper.shift_whitespace();
        }
    }

    template<typename HeaderType>
    void _decompress_with(const std::string& vtk_compressor,
                         Serialization& data,
                         const Compression::CompressedBlocks<HeaderType>& blocks) {
        if (vtk_compressor == "vtkLZ4DataCompressor") {
#if GRIDFORMAT_HAVE_LZ4
            LZ4Compressor{}.decompress(data, blocks);
#else
            throw InvalidState("Need LZ4 to decompress the data");
#endif
        } else if (vtk_compressor == "vtkLZMADataCompressor") {
#if GRIDFORMAT_HAVE_LZMA
            LZMACompressor{}.decompress(data, blocks);
#else
            throw InvalidState("Need LZMA to decompress the data");
#endif
        } else if (vtk_compressor == "vtkZLibDataCompressor") {
#if GRIDFORMAT_HAVE_ZLIB
            ZLIBCompressor{}.decompress(data, blocks);
#else
            throw InvalidState("Need ZLib to decompress the data");
#endif
        } else {
            throw NotImplemented("Unsupported vtk compressor '" + vtk_compressor + "'");
        }
    }

    template<Concepts::Scalar TargetType, Concepts::Scalar HeaderType = std::size_t>
    class DataArrayReader {
        static constexpr Precision<TargetType> target_precision{};
        static constexpr Precision<HeaderType> header_precision{};

        static constexpr bool is_too_small_integral = std::integral<TargetType> && sizeof(TargetType) < 4;
        static constexpr bool is_signed_integral = std::signed_integral<TargetType>;
        using BufferedType = std::conditional_t<is_signed_integral, short, unsigned short>;

     public:
        using Header = std::vector<HeaderType>;

        DataArrayReader(std::istream& s,
                        std::endian e = std::endian::native,
                        std::string compressor = "")
        : _stream{s}
        , _endian{e}
        , _compressor{compressor}
        {}

        void read_ascii(std::size_t number_of_values, Serialization& out_values) {
            out_values.resize(number_of_values*sizeof(TargetType));
            std::span<TargetType> out_span = out_values.as_span_of(target_precision);

            // istream_view uses operator>>, which seems to do weird stuff for e.g. uint8_t.
            // The smallest supported integral type seems to be short. In case we deal with
            // small integral types, we first read into a vector and then cast into the desired type.
            if constexpr (is_too_small_integral) {
                std::vector<BufferedType> buffer(number_of_values);
                _read_ascii_to(std::span{buffer}, number_of_values);
                std::ranges::copy(buffer | std::views::transform([] <typename T> (const T& value) {
                    return static_cast<TargetType>(value);
                }), out_span.begin());
            } else {
                _read_ascii_to(out_span, number_of_values);
            }
        }

        template<Concepts::Decoder Decoder>
        void read_binary(const Decoder& decoder,
                         OptionalReference<Header> header = {},
                         OptionalReference<Serialization> values = {}) {
            if constexpr (std::unsigned_integral<HeaderType> && sizeof(HeaderType) >= 4) {
                if (_compressor.empty())
                    return _read_encoded(decoder, header, values);
                else
                    return _read_encoded_compressed(decoder, header, values);
            } else {
                throw IOError("Unsupported header type");
            }
        }

     private:
        template<typename T, std::size_t size>
        void _read_ascii_to(std::span<T, size> buffer, std::size_t expected_num_values) const {
            const auto [_, out] = std::ranges::copy(
                std::ranges::istream_view<T>{_stream}
                | std::views::take(expected_num_values),
                buffer.begin()
            );
            const auto number_of_values_read = std::ranges::distance(buffer.begin(), out);
            if (number_of_values_read < 0 || static_cast<std::size_t>(number_of_values_read) < expected_num_values)
                throw SizeError("Could not read the requested number of values from the stream");
        }

        template<typename Decoder>
        void _read_encoded(const Decoder& decoder,
                           OptionalReference<Header> out_header = {},
                           OptionalReference<Serialization> out_values = {}) {
            const auto pos = _stream.tellg();
            Serialization header = decoder.decode_from(_stream, sizeof(HeaderType));

            if (header.size() != sizeof(HeaderType)) {  // no padding - header & values are encoded together
                if (header.size() < sizeof(HeaderType))
                    throw SizeError("Could not read header");

                header.resize(sizeof(HeaderType));
                change_byte_order(header.as_span_of(header_precision), {.from = _endian});

                if (out_header)
                    std::ranges::copy(header.as_span_of(header_precision), std::back_inserter(out_header.unwrap()));

                if (out_values) {
                    _stream.seekg(pos);
                    const auto number_of_bytes = header.as_span_of(header_precision)[0];
                    const auto number_of_bytes_with_header = number_of_bytes + sizeof(HeaderType);
                    Serialization& header_and_values = out_values.unwrap();
                    header_and_values = decoder.decode_from(_stream, number_of_bytes_with_header);
                    header_and_values.cut_front(sizeof(HeaderType));
                    change_byte_order(header_and_values.as_span_of(header_precision), {.from = _endian});
                }
            } else {  // values are encoded separately
                change_byte_order(header.as_span_of(header_precision), {.from = _endian});

                if (out_header)
                    std::ranges::copy(header.as_span_of(header_precision), std::back_inserter(out_header.unwrap()));

                if (out_values) {
                    const auto number_of_bytes = header.as_span_of(header_precision)[0];
                    Serialization& values = out_values.unwrap();
                    values = decoder.decode_from(_stream, number_of_bytes);
                    change_byte_order(values.as_span_of(Precision<TargetType>{}), {.from = _endian});
                }
            }
        }

        template<typename Decoder>
        void _read_encoded_compressed(const Decoder& decoder,
                                      OptionalReference<Header> out_header = {},
                                      OptionalReference<Serialization> out_values = {}) {
            const auto begin_pos = _stream.tellg();
            const auto header_bytes = sizeof(HeaderType)*3;
            Serialization header = decoder.decode_from(_stream, header_bytes);

            // if the decoded header is larger than requested, then there is no padding,
            // which means that we'll have to decode the header together with the values.
            const bool decode_blocks_with_header = header.size() != header_bytes;
            if (decode_blocks_with_header)
                header.resize(header_bytes);

            auto header_data = header.as_span_of(header_precision);
            if (header_data.size() < 3)
                throw SizeError("Could not read data array header");

            change_byte_order(header_data, {.from = _endian});
            const auto number_of_blocks = header_data[0];
            const auto full_block_size = header_data[1];
            const auto residual_block_size = header_data[2];
            const HeaderType number_of_raw_bytes = residual_block_size > 0
                ? full_block_size*(number_of_blocks-1) + residual_block_size
                : full_block_size*number_of_blocks;

            Serialization block_sizes;
            const std::size_t block_sizes_bytes = sizeof(HeaderType)*number_of_blocks;
            if (decode_blocks_with_header) {
                _stream.seekg(begin_pos);
                block_sizes = decoder.decode_from(_stream, header_bytes + block_sizes_bytes);
                block_sizes.cut_front(sizeof(HeaderType)*3);
            } else {
                block_sizes = decoder.decode_from(_stream, block_sizes_bytes);
            }

            std::vector<HeaderType> compressed_block_sizes(number_of_blocks);
            change_byte_order(block_sizes.as_span_of(header_precision), {.from = _endian});
            std::ranges::copy(
                block_sizes.as_span_of(header_precision),
                compressed_block_sizes.begin()
            );

            if (out_header) {
                std::ranges::copy(header_data, std::back_inserter(out_header.unwrap()));
                std::ranges::copy(compressed_block_sizes, std::back_inserter(out_header.unwrap()));
            }

            if (out_values) {
                Serialization& values = out_values.unwrap();
                values = decoder.decode_from(_stream, std::accumulate(
                    compressed_block_sizes.begin(),
                    compressed_block_sizes.end(),
                    HeaderType{0}
                ));

                _decompress_with(_compressor, values, Compression::CompressedBlocks{
                    {number_of_raw_bytes, full_block_size},
                    std::move(compressed_block_sizes)
                });
                change_byte_order(values.as_span_of(target_precision), {.from = _endian});
            }
        }

        std::istream& _stream;
        std::endian _endian;
        std::string _compressor;
    };

}  // namespace XMLDetail
#endif  // DOXYGEN


namespace XML {

/*!
 * \ingroup VTK
 * \brief Return a range over all data array elements in the given xml section.
 */
std::ranges::range auto data_arrays(const XMLElement& e) {
    return children(e) | std::views::filter([] (const XMLElement& child) {
        return child.name() == "DataArray";
    });
}

/*!
 * \ingroup VTK
 * \brief Return a range over the names of all data array elements in the given xml section.
 */
std::ranges::range auto data_array_names(const XMLElement& e) {
    return data_arrays(e) | std::views::transform([] (const XMLElement& data_array) {
        return data_array.get_attribute("Name");
    });
}

/*!
 * \ingroup VTK
 * \brief Return the data array element with the given name within the given xml section.
 */
const XMLElement& get_data_array(std::string_view name, const XMLElement& section) {
    for (const auto& da
            : data_arrays(section)
            | std::views::filter([&] (const auto& e) { return e.get_attribute("Name") == name; }))
        return da;
    throw ValueError(
        "Could not find data array with name '" + std::string{name}
        + "' in section '"  + std::string{section.name()} + "'"
    );
}

}  // namespace XML


#ifndef DOXYGEN
namespace XMLDetail {

    // can be reused by readers to copy read field names into the storage container
    template<typename FieldNameContainer>
    void copy_field_names_from(const XMLElement& vtk_grid, FieldNameContainer& names) {
        if (vtk_grid.has_child("Piece")) {
            const XMLElement& piece = vtk_grid.get_child("Piece");
            if (piece.has_child("PointData"))
                std::ranges::copy(
                    XML::data_array_names(piece.get_child("PointData")),
                    std::back_inserter(names.point_fields)
                );
            if (piece.has_child("CellData"))
                std::ranges::copy(
                    XML::data_array_names(piece.get_child("CellData")),
                    std::back_inserter(names.cell_fields)
                );
        }
        if (vtk_grid.has_child("FieldData"))
            std::ranges::copy(
                XML::data_array_names(vtk_grid.get_child("FieldData")),
                std::back_inserter(names.meta_data_fields)
            );
    }

}  // namespace XMLDetail
#endif  // DOXYGEN


/*!
 * \ingroup VTK
 * \brief Helper class for VTK-XML readers to use.
 */
class XMLReaderHelper {
    using DataArrayStreamLocation = XMLDetail::DataArrayStreamLocation;

 public:
    explicit XMLReaderHelper(const std::string& filename)
    : _filename{filename}
    , _parser{filename, "ROOT", [] (const XMLElement& e) { return e.name() == "AppendedData"; }} {
        if (!_element().has_child("VTKFile"))
            throw IOError("Could not read " + filename + " as vtk-xml file. No root element <VTKFile> found.");
    }

    static XMLReaderHelper make_from(const std::string& filename, std::string_view vtk_type) {
        if (!Path::exists(filename))
            throw IOError("File '" + filename + "' does not exist.");
        if (!Path::is_file(filename))
            throw IOError("Given path '" + filename + "' is not a file.");

        std::optional<XMLReaderHelper> helper;
        try {
            helper.emplace(XMLReaderHelper{filename});
        } catch (const std::exception& e) {
            throw IOError("Could not parse '" + filename + "' as xml file. Error: " + e.what());
        }

        if (!helper->get().has_attribute("type"))
            throw IOError("'type' attribute missing in VTKFile root element.");
        if (helper->get().get_attribute("type") != vtk_type)
            throw IOError(
                "Given vtk-xml file has type '"
                + helper->get().get_attribute("type")
                + "', expected '" + std::string{vtk_type} + "'"
            );

        return std::move(helper).value();
    }

    const XMLElement& get(std::string_view path = "") const {
        OptionalReference opt_ref = access_at(path, _element().get_child("VTKFile"));
        if (!opt_ref)
            throw ValueError("The given path '" + std::string{path} + "' could not be found.");
        return opt_ref.unwrap();
    }

    //! Returns the field representing the points of the grid
    FieldPtr make_points_field(std::string_view section_path, std::size_t num_expected_points) const {
        std::size_t visited = 0;
        FieldPtr result{nullptr};
        for (const auto& data_array : XML::data_arrays(get(section_path))) {
            if (!result)
                result = make_data_array_field(data_array.get_attribute("Name"), section_path, num_expected_points);
            else {
                log_warning("Points section contains more than one data array, using first one as point coordinates");
                break;
            }
            visited++;
        }
        if (visited == 0)
            throw ValueError("Points section does not contain a data array element");
        return result;
    }

    //! Returns a field which draws the actual field values from the file upon request
    FieldPtr make_data_array_field(std::string_view name,
                                   std::string_view section_path,
                                   const std::optional<std::size_t> number_of_tuples = {}) const {
        return make_data_array_field(XML::get_data_array(name, get(section_path)), number_of_tuples);
    }

    //! Returns a field which draws the actual field values from the file upon request
    FieldPtr make_data_array_field(const XMLElement& element,
                                   const std::optional<std::size_t> number_of_tuples = {}) const {
        if (element.name() != "DataArray")
            throw ValueError("Given path is not a DataArray element");
        if (!element.has_attribute("type"))
            throw ValueError("DataArray element does not specify the data type (`type` attribute)");
        if (!element.has_attribute("format"))
            throw ValueError("Data array element does not specify its format (e.g. ascii/binary)");
        if (number_of_tuples.has_value())
            return _make_data_array_field(element, number_of_tuples.value());
        return _make_data_array_field(element, _number_of_tuples(element));
    }

 private:
    const XMLElement& _element() const {
        return _parser.get_xml();
    }

    const XMLElement& _appendix() const {
        if (!_element().get_child("VTKFile").has_child("AppendedData"))
            throw ValueError("Read vtk file has no appendix");
        return _element().get_child("VTKFile").get_child("AppendedData");
    }

    FieldPtr _make_data_array_field(const XMLElement& element,
                                    const std::size_t number_of_tuples) const {
        if (element.name() != "DataArray")
            throw ValueError("Given xml element does not describe a data array");
        if (!element.has_attribute("format"))
            throw ValueError("Data array element does not specify its format (e.g. ascii/binary)");
        if (!element.has_attribute("type"))
            throw ValueError("Data array element does not specify the data type");
        if (element.get_attribute("format") == "appended" && !element.has_attribute("offset"))
            throw ValueError("Data array element specifies to use appended data but does not specify offset");
        return element.get_attribute("format") == "ascii"
            ? _make_ascii_data_array_field(element, number_of_tuples)
            : _make_binary_data_array_field(element, number_of_tuples);
    }

    FieldPtr _make_ascii_data_array_field(const XMLElement& e, const std::size_t num_tuples) const {
        MDLayout expected_layout = _expected_layout(e, num_tuples);
        auto num_values = expected_layout.number_of_entries();
        return from_precision_attribute(e.get_attribute("type")).visit([&] <typename T> (const Precision<T>& prec) {
            return make_field_ptr(LazyField{
                std::string{_filename},
                std::move(expected_layout),
                prec,
                [_nv=num_values, _begin=_parser.get_content_bounds(e).begin_pos] (std::string filename) {
                    std::ifstream file{filename};
                    file.seekg(_begin);
                    Serialization result{_nv*sizeof(T)};
                    XMLDetail::DataArrayReader<T>{file}.read_ascii(_nv, result);
                    return result;
                }
            });
        });
    }

    FieldPtr _make_binary_data_array_field(const XMLElement& e, const std::size_t num_tuples) const {
        auto expected_layout = _expected_layout(e, num_tuples);
        return from_precision_attribute(e.get_attribute("type")).visit([&] <typename T> (const Precision<T>& prec) {
            FieldPtr result;
            _apply_decoder_for(e, [&] (auto&& decoder) {
                result = make_field_ptr(LazyField{
                    std::string{_filename},
                    std::move(expected_layout),
                    prec,
                    [
                        _loc=_stream_location_for(e),
                        _header_prec=from_precision_attribute(get().get_attribute("header_type")),
                        _endian=from_endian_attribute(get().get_attribute("byte_order")),
                        _comp=get().get_attribute_or(std::string{""}, "compressor"),
                        _decoder=std::move(decoder)
                    ] (std::string filename) {
                        std::ifstream file{filename};
                        XMLDetail::_move_to_data(_loc, file);
                        return _header_prec.visit([&] <typename H> (const Precision<H>&) {
                            Serialization result;
                            XMLDetail::DataArrayReader<T, H>{file, _endian, _comp}.read_binary(_decoder, {}, result);
                            return result;
                        });
                    }
                });
            });
            return result;
        });
    }

    MDLayout _expected_layout(const XMLElement& e, const std::size_t num_tuples) const {
        const auto num_comps = e.get_attribute_or(std::size_t{1}, "NumberOfComponents");
        if (num_comps > 1)
            return MDLayout{{num_tuples, num_comps}};
        return MDLayout{{num_tuples}};
    }

    std::size_t _number_of_tuples(const XMLElement& element) const {
        const auto number_of_components = element.get_attribute_or(std::size_t{1}, "NumberOfComponents");
        const auto number_of_values = [&] () {
            if (element.get_attribute("type") != "String") {
                if (element.has_attribute("NumberOfTuples"))
                    return from_string<std::size_t>(element.get_attribute("NumberOfTuples"));
            } else if (element.has_attribute("NumberOfTuples")) {
                const auto num_values = from_string<std::size_t>(element.get_attribute("NumberOfTuples"));
                if (num_values > 1)
                    throw ValueError("Cannot read string data arrays with more than one tuple");
            }
            return _deduce_number_of_values(element);
        } ();

        if (number_of_values%number_of_components != 0)
            throw ValueError(
                "The number of components of data array '" + element.name() + "' "
                + "(" + as_string(number_of_components) + ") "
                + "is incompatible with the number of values it contains "
                + "(" + as_string(number_of_values) + ")"
            );
        return number_of_values/number_of_components;
    }

    std::size_t _deduce_number_of_values(const XMLElement& element) const {
        std::ifstream file{_filename};
        XMLDetail::_move_to_data(_stream_location_for(element), file);

        if (element.get_attribute("format") == "ascii") {
            const auto precision = from_precision_attribute(element.get_attribute("type"));
            return precision.visit([&] <typename T> (const Precision<T>&) {
                using _T = std::conditional_t<
                    std::integral<T> && sizeof(T) < 4,  // see XMLDetail::DataArrayReader::read_ascii
                    int,
                    T
                >;
                return std::ranges::distance(std::ranges::istream_view<_T>{file});
            });
        }

        InputStreamHelper helper{file};
        const auto header = _read_binary_data_array_header(helper, element);
        if (get().has_attribute("compressor") && header.size() < 3)
            throw ValueError("Could not read compression header");
        const std::size_t number_of_bytes = [&] () {
            if (get().has_attribute("compressor")) {
                const auto num_full_blocks = header.at(0);
                const auto full_block_size = header.at(1);
                const auto residual_block_size = header.at(2);
                return residual_block_size > 0
                    ? full_block_size*(num_full_blocks - 1) + residual_block_size
                    : full_block_size*num_full_blocks;
            }
            return header.at(0);
        } ();
        const std::size_t value_type_number_of_bytes = from_precision_attribute(
            element.get_attribute("type")
        ).size_in_bytes();

        if (number_of_bytes%value_type_number_of_bytes != 0)
            throw ValueError(
                "The length of the data array '" + element.name() + "' "
                + "is incompatible with the data type '" + element.get_attribute("type") + "'"
            );

        return number_of_bytes/value_type_number_of_bytes;
    }

    DataArrayStreamLocation _stream_location_for(const XMLElement& element) const {
        if (element.get_attribute("format") == "appended")
            return DataArrayStreamLocation{
                .begin = _parser.get_content_bounds(_appendix()).begin_pos,
                .offset = from_string<std::size_t>(element.get_attribute("offset"))
            };
        return DataArrayStreamLocation{.begin = _parser.get_content_bounds(element).begin_pos};
    }

    std::vector<std::size_t> _read_binary_data_array_header(InputStreamHelper& stream,
                                                            const XMLElement& element) const {
        const std::string compressor = get().get_attribute_or(std::string{""}, "compressor");
        const std::endian endian = from_endian_attribute(get().get_attribute("byte_order"));
        const auto header_prec = from_precision_attribute(get().get_attribute("header_type"));
        const auto values_prec = from_precision_attribute(element.get_attribute("type"));

        return header_prec.visit([&] <typename HT> (const Precision<HT>&) {
            return values_prec.visit([&] <typename VT> (const Precision<VT>&) {
                std::vector<HT> _header;
                XMLDetail::DataArrayReader<VT, HT> reader{stream, endian, compressor};
                _apply_decoder_for(element, [&] (const auto& decoder) {
                    reader.read_binary(decoder, _header);
                });

                if (_header.empty())
                    throw IOError("Could not read header for data array '" + element.get_attribute("Name") + "'");

                std::vector<std::size_t> result;
                std::ranges::for_each(_header, [&] <typename T> (const T& value) {
                    result.push_back(static_cast<std::size_t>(value));
                });
                return result;
            });
        });
    }

    template<typename Action>
    void _apply_decoder_for(const XMLElement& data_array, const Action& action) const {
        if (data_array.get_attribute("format") == "binary")
            return action(Base64Decoder{});
        else if (data_array.get_attribute("format") == "appended")
            return _appendix().get_attribute("encoding") == "base64"
                ? action(Base64Decoder{})
                : action(RawDecoder{});
        throw InvalidState("Unknown data format");
    }

    std::string _filename;
    XMLParser _parser;
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_XML_HPP_
