// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Base class for VTK XML-type file format writers.
 */
#ifndef GRIDFORMAT_VTK_XML_HPP_
#define GRIDFORMAT_VTK_XML_HPP_

#include <bit>
#include <string>
#include <ranges>
#include <utility>
#include <type_traits>
#include <optional>

#include <gridformat/common/detail/crtp.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/variant.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/field.hpp>

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
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/attributes.hpp>
#include <gridformat/vtk/data_array.hpp>
#include <gridformat/vtk/appendix.hpp>

namespace GridFormat::VTK {

//! \addtogroup VTK
//! \{

#ifndef DOXYGEN
namespace XMLDetail {
    using LZMACompressor = std::conditional_t<Compression::Detail::_have_lzma, Compression::LZMA, None>;
    using ZLIBCompressor = std::conditional_t<Compression::Detail::_have_zlib, Compression::ZLIB, None>;
    using LZ4Compressor = std::conditional_t<Compression::Detail::_have_lz4, Compression::LZ4, None>;
    using Compressor = UniqueVariant<LZ4Compressor, ZLIBCompressor, LZMACompressor>;
}  // namespace XMLDetail
#endif  // DOXYGEN

namespace XML {

using HeaderPrecision = std::variant<UInt32, UInt64>;
using Compressor = ExtendedVariant<XMLDetail::Compressor, None>;
using Encoder = std::variant<Encoding::Ascii, Encoding::Base64, Encoding::RawBinary>;
using DataFormat = std::variant<VTK::DataFormat::Inlined, VTK::DataFormat::Appended>;
using DefaultCompressor = std::variant_alternative_t<0, XMLDetail::Compressor>;

}  // namespace XML

//! Options for VTK-XML files
struct XMLOptions {
    using EncoderOption = ExtendedVariant<XML::Encoder, Automatic>;
    using CompressorOption = ExtendedVariant<XML::Compressor, Automatic>;
    using DataFormatOption = ExtendedVariant<XML::DataFormat, Automatic>;
    using CoordinatePrecisionOption = std::variant<DynamicPrecision, Automatic>;
    EncoderOption encoder = automatic;
    CompressorOption compressor = automatic;
    DataFormatOption data_format = automatic;
    CoordinatePrecisionOption coordinate_precision = automatic;
    XML::HeaderPrecision header_precision = _from_size_t();

 private:
    static XML::HeaderPrecision _from_size_t() {
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
        DynamicPrecision coordinate_precision;
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
                        DynamicPrecision{Precision<GridCoordinateType>{}} :
                        Variant::unwrap(Variant::without<Automatic>(opts.coordinate_precision)),
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
 * \brief TODO: Doc me
 */
template<Concepts::Grid G, typename Impl>
class XMLWriterBase
: public GridWriter<G>
, public Detail::CRTPBase<Impl> {
    using ParentType = GridWriter<G>;
    using GridCoordinateType = CoordinateType<G>;

 public:
    //! Export underlying grid type
    using Grid = G;

    explicit XMLWriterBase(const Grid& grid,
                           std::string extension,
                           XMLOptions xml_opts = {})
    : ParentType(grid, std::move(extension))
    , _xml_settings{XMLDetail::XMLSettings::from<GridCoordinateType>(xml_opts)} {
    }

    Impl with_data_format(const XML::DataFormat& format) const {
        auto opts = _xml_opts();
        Variant::unwrap_to(opts.data_format, format);
        return this->_impl().with(std::move(opts));
    };

    Impl with_compression(const XML::Compressor& compressor) const {
        auto opts = _xml_opts();
        Variant::unwrap_to(opts.compressor, compressor);
        return this->_impl().with(std::move(opts));
    };

    Impl with_encoding(const XML::Encoder& encoder) const {
        auto opts = _xml_opts();
        Variant::unwrap_to(opts.encoder, encoder);
        return this->_impl().with(std::move(opts));
    };

    Impl with_coordinate_precision(const DynamicPrecision& prec) const {
        auto opts = _xml_opts();
        opts.coordinate_precision = prec;
        return this->_impl().with(std::move(opts));
    }

    Impl with_header_precision(const XML::HeaderPrecision& prec) const {
        auto opts = _xml_opts();
        opts.header_precision = prec;
        return this->_impl().with(std::move(opts));
    }

    template<typename T>
    void set_meta_data(const std::string& name, T&& value) {
        ParentType::set_meta_data(name, std::forward<T>(value));
    }

    void set_meta_data(const std::string& name, std::string text) {
        text.push_back('\0');
        ParentType::set_meta_data(name, std::move(text));
    }

 protected:
    XMLDetail::XMLSettings _xml_settings;

    XMLOptions _xml_opts() const {
        return std::visit([&] (const auto& encoder) {
            return std::visit([&] (const auto& compressor) {
                return std::visit([&] (const auto& data_format) {
                    return std::visit([&] (const auto& header_prec) {
                        return XMLOptions{
                            .encoder = encoder,
                            .compressor = compressor,
                            .data_format = data_format,
                            .coordinate_precision = _xml_settings.coordinate_precision,
                            .header_precision = header_prec
                        };
                    }, _xml_settings.header_precision);
                }, _xml_settings.data_format);
            }, _xml_settings.compressor);
        }, _xml_settings.encoder);
    }

    struct WriteContext {
        std::string vtk_grid_type;
        XMLElement xml_representation;
        Appendix appendix;
    };

    WriteContext _get_write_context(std::string vtk_grid_type) const {
        XMLElement xml("VTKFile");
        xml.set_attribute("type", vtk_grid_type);
        xml.set_attribute("version", "2.0");
        std::visit([&] <typename T> (const Precision<T>& prec) {
            xml.set_attribute("header_type", attribute_name(prec));
        }, _xml_settings.header_precision);
        xml.set_attribute("byte_order", attribute_name(std::endian::native));
        std::visit([&] <typename C> (const C& c) {
            if constexpr (!is_none<C>)
                xml.set_attribute("compressor", attribute_name(c));
        }, _xml_settings.compressor);

        Appendix appendix;
        XMLElement& grid_section = xml.add_child(vtk_grid_type);
        XMLElement& field_data = grid_section.add_child("FieldData");
        _add_field_data(field_data, appendix);

        return {
            .vtk_grid_type = std::move(vtk_grid_type),
            .xml_representation = std::move(xml),
            .appendix = std::move(appendix)
        };
    }

    void _add_field_data(XMLElement& field_data, Appendix& appendix) const {
        std::visit([&] (const auto& compressor) {
            std::visit([&] (const auto& encoder) {
                std::visit([&] (const auto& data_format) {
                    std::visit([&] <typename H> (const Precision<H>& header_precision) {
                        std::ranges::for_each(this->_meta_data_field_names(), [&] (const std::string& name) {
                            const auto& field = this->_get_meta_data_field(name);
                            const auto layout = field.layout();
                            auto& arr = field_data.add_child("DataArray");
                            arr.set_attribute("Name", name);
                            arr.set_attribute("format", data_format_name(encoder, data_format));
                            field.precision().visit([&] <typename T> (const Precision<T>& p) {
                                if constexpr (std::is_same_v<T, char>) {
                                    arr.set_attribute("type", "String");
                                    arr.set_attribute("NumberOfTuples", 1);
                                }
                                else {
                                    arr.set_attribute("NumberOfTuples", layout.extent(0));
                                    arr.set_attribute("type", attribute_name(DynamicPrecision{p}));
                                    arr.set_attribute(
                                        "NumberOfComponents",
                                        layout.dimension() > 1
                                            ? layout.sub_layout(1).number_of_entries()
                                            : 1
                                    );
                                }
                                _set_data_array_content(data_format, arr, appendix, DataArray{
                                    field, encoder, compressor, header_precision
                                });
                            });
                        });
                    }, this->_xml_settings.header_precision);
                }, this->_xml_settings.data_format);
            }, this->_xml_settings.encoder);
        }, this->_xml_settings.compressor);
    }

    template<typename ValueType>
    void _set_attribute(WriteContext& context,
                        std::string_view xml_group,
                        const std::string& attr_name,
                        const ValueType& attr_value) const {
        _access_element(context, _get_element_names(xml_group)).set_attribute(attr_name, attr_value);
    }

    void _set_data_array(WriteContext& context,
                         std::string_view xml_group,
                         std::string data_array_name,
                         const Field& field) const {
        const auto layout = field.layout();
        XMLElement& da = _access_element(context, _get_element_names(xml_group)).add_child("DataArray");
        da.set_attribute("Name", std::move(data_array_name));
        da.set_attribute("type", attribute_name(field.precision()));
        da.set_attribute("NumberOfComponents", (layout.dimension() == 1 ? 1 : layout.number_of_entries(1)));
        std::visit([&] (const auto& compressor) {
            std::visit([&] (const auto& encoder) {
                std::visit([&] <typename DataFormat> (const DataFormat& data_format) {
                    std::visit([&] <typename T> (const Precision<T>& header_precision) {
                        da.set_attribute("format", data_format_name(encoder, data_format));
                        _set_data_array_content(data_format, da, context.appendix, DataArray{
                            field, encoder, compressor, header_precision
                        });
                    }, _xml_settings.header_precision);
                }, this->_xml_settings.data_format);
            }, this->_xml_settings.encoder);
        }, this->_xml_settings.compressor);
    }

    template<typename DataFormat, typename Appendix, typename Content> requires(!std::is_lvalue_reference_v<Content>)
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

    XMLElement& _access_element(WriteContext& context, const std::vector<std::string>& sub_elem_names) const {
        XMLElement* element = &context.xml_representation.get_child(context.vtk_grid_type);
        for (const auto& element_name : sub_elem_names)
            element = element->has_child(element_name) ? &element->get_child(element_name)
                                                       : &element->add_child(element_name);
        return *element;
    }

    std::vector<std::string> _get_element_names(std::string_view elements) const {
        std::vector<std::string> result;
        std::ranges::copy(
            std::views::split(elements, std::string_view{"."})
                | std::views::transform([] (const auto& word) {
                    return std::string{word.begin(), word.end()};
                }),
            std::back_inserter(result)
        );
        return result;
    }

    void _write_xml(WriteContext&& context, std::ostream& s) const {
        Indentation indentation{{.width = 2}};
        std::visit([&] (const auto& encoder) {
            std::visit([&] <typename DataFormat> (const DataFormat&) {
                if constexpr (std::is_same_v<DataFormat, VTK::DataFormat::Inlined>)
                    write_xml_with_version_header(context.xml_representation, s, indentation);
                else
                    XML::Detail::write_with_appendix(std::move(context), s, encoder, indentation);
            }, this->_xml_settings.data_format);
        }, this->_xml_settings.encoder);
    }
};

//! \} group VTK

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_XML_HPP_
