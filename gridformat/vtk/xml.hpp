// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Funcionality for writing VTK XML-type file formats
 */
#ifndef GRIDFORMAT_VTK_XML_HPP_
#define GRIDFORMAT_VTK_XML_HPP_

#include <bit>
#include <string>
#include <ranges>
#include <utility>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/callable_overload_set.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/encoding/base64.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/attributes.hpp>
#include <gridformat/vtk/data_array.hpp>
#include <gridformat/vtk/appendix.hpp>

#include <gridformat/encoding.hpp>
#include <gridformat/compression.hpp>


#if GRIDFORMAT_HAVE_LZ4
#include <gridformat/compression/lz4.hpp>
namespace GridFormat::VTK::Defaults { using Compressor = Compression::LZ4; }
#elif GRIDFORMAT_HAVE_LZMA
#include <gridformat/compression/lzma.hpp>
namespace GridFormat::VTK::Defaults { using Compressor = Compression::LZMA; }
#elif GRIDFORMAT_HAVE_ZLIB
#include <gridformat/compression/zlib.hpp>
namespace GridFormat::VTK::Defaults { using Compressor = Compression::ZLIB; }
#else
#include <gridformat/common/type_traits.hpp>
namespace GridFormat::VTK::Defaults { using Compressor = None; }
#endif


namespace GridFormat::VTK {

//! \addtogroup VTK
//! \{

//! Options for VTK-XML files
struct XMLOptions {
    using EncoderOption = ExtendedVariant<GridFormat::Encoder, Automatic>;
    using CompressorOption = ExtendedVariant<GridFormat::Compressor, Automatic, None>;
    using DataFormatOption = ExtendedVariant<VTKDataFormat, Automatic>;
    EncoderOption encoder = automatic;
    CompressorOption compressor = automatic;
    DataFormatOption data_format = automatic;
};

//! Options for setting the header and coordinate type
struct PrecisionOptions {
    using CoordinatePrecision = std::variant<DynamicPrecision, Automatic>;
    using HeaderPrecision = std::variant<UInt32, UInt64>;
    CoordinatePrecision coordinate_precision = automatic;
    HeaderPrecision header_precision = _from_size_t();

 private:
    static HeaderPrecision _from_size_t() {
        if constexpr(sizeof(std::size_t) == 8) return uint64;
        else return uint32;
    }
};


#ifndef DOXYGEN
namespace XMLDetail {

struct XMLSettings {
    using Compressor = ExtendedVariant<GridFormat::Compressor, None>;
    Encoder encoder;
    Compressor compressor;
    VTKDataFormat data_format;

    static XMLSettings from(const XMLOptions& opts) {
        const auto _enc = _make_encoder(opts.encoder);
        const auto _format = _make_data_format(_enc, opts.data_format);
        const auto _comp = _make_compressor(_enc, opts.compressor);
        return {
            .encoder = _enc,
            .compressor = _comp,
            .data_format = _format
        };
    }

 private:
    template<typename E>
    static Encoder _make_encoder(const E& enc) {
        if (_is_automatic(enc))
            return Encoding::base64;
        return _make_from<Encoder>(enc);
    }

    template<typename E, typename F>
    static VTKDataFormat _make_data_format(const E& enc, const F& data_format) {
        if (_is_automatic(data_format))
            return _is_ascii(enc) ? VTKDataFormat{VTK::DataFormat::inlined}
                                  : VTKDataFormat{VTK::DataFormat::appended};
        return _make_from<VTKDataFormat>(data_format);
    }

    template<typename E, typename C>
    static Compressor _make_compressor(const E& enc, const C& compressor) {
        if (_is_automatic(compressor))
            return _is_ascii(enc) ? Compressor{none}
                                  : Compressor{Defaults::Compressor{}};
        if (_is_ascii(enc) && !_is_none(compressor)) {
            log_warning("Ascii output cannot be compressed. Ignoring chosen compressor...");
            return none;
        }
        return _make_from<Compressor>(compressor);
    }

    template<typename ResultVariant, typename InputVariant>
    static ResultVariant _make_from(const InputVariant& v) {
        ResultVariant result;
        std::visit(Overload{
            [&] (const Automatic&) {},
            [&] (const auto& value) { result = value; }
        }, v);
        return result;
    }

    static bool _is_automatic(const auto& v) { return std::holds_alternative<Automatic>(v); }
    static bool _is_none(const auto& v) { return std::holds_alternative<None>(v); }
    static bool _is_raw(const auto& e) { return std::holds_alternative<Encoding::RawBinary>(e); }
    static bool _is_ascii(const auto& e) { return std::holds_alternative<Encoding::Ascii>(e); }
    static bool _is_inlined(const auto& f) { return std::holds_alternative<VTK::DataFormat::Inlined>(f); }
    static bool _is_appended(const auto& f) { return std::holds_alternative<VTK::DataFormat::Appended>(f); }
};

}  // namespace XMLDetail
#endif  // DOXYGEN


/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::Grid G>
class XMLWriterBase : public GridWriter<G> {
    using ParentType = GridWriter<G>;
    using HeaderPrecision = typename PrecisionOptions::HeaderPrecision;

 public:
    //! Export underlying grid type
    using Grid = G;

    explicit XMLWriterBase(const Grid& grid,
                           std::string extension,
                           XMLOptions xml_opts = {},
                           PrecisionOptions prec_opts = {})
    : ParentType(grid, std::move(extension))
    , _xml_settings{XMLDetail::XMLSettings::from(xml_opts)}
    , _header_precision{prec_opts.header_precision} {
        std::visit(Overload{
            [&] (const Automatic&) { _coord_precision = Precision<CoordinateType<Grid>>{}; },
            [&] (const auto& prec) { _coord_precision = prec; }
        }, prec_opts.coordinate_precision);
    }

 protected:
    XMLDetail::XMLSettings _xml_settings;
    HeaderPrecision _header_precision;
    DynamicPrecision _coord_precision;

    XMLOptions _xml_opts() const {
        return std::visit([&] (const auto& encoder) {
            return std::visit([&] (const auto& compressor) {
                return std::visit([&] (const auto data_format) {
                    return XMLOptions{
                        .encoder = encoder,
                        .compressor = compressor,
                        .data_format = data_format
                    };
                }, _xml_settings.data_format);
            }, _xml_settings.compressor);
        }, _xml_settings.encoder);
    }

    PrecisionOptions _precision_opts() const {
        PrecisionOptions result;
        std::visit([&] (const auto& header_prec) {
            _coord_precision.visit([&] (const auto& coord_prec) {
                result.coordinate_precision = coord_prec;
                result.header_precision = header_prec;
            });
        }, _header_precision);
        return result;
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
        }, _header_precision);
        xml.set_attribute("byte_order", attribute_name(std::endian::native));
        xml.add_child(vtk_grid_type);
        std::visit([&] <typename C> (const C& c) {
            if constexpr (!is_none<C>)
                xml.set_attribute("compressor", attribute_name(c));
        }, _xml_settings.compressor);
        return {
            .vtk_grid_type = std::move(vtk_grid_type),
            .xml_representation = std::move(xml),
            .appendix = {}
        };
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
                    std::visit([&] <typename T> (const Precision<T>&) {
                        da.set_attribute("format", data_format_name(encoder, data_format));
                        auto data_array = DataArray{field, encoder, compressor, Precision<T>{}};
                        if constexpr (std::is_same_v<DataFormat, VTK::DataFormat::Inlined>)
                            da.set_content(std::move(data_array));
                        else if constexpr (std::is_same_v<DataFormat, VTK::DataFormat::Appended>)
                            context.appendix.add(std::move(data_array));
                        else
                            throw ValueError("Unsupported data format");
                    }, _header_precision);
                }, this->_xml_settings.data_format);
            }, this->_xml_settings.encoder);
        }, this->_xml_settings.compressor);
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
