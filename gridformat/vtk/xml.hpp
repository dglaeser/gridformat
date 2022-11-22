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

namespace GridFormat::VTK {

//! \addtogroup VTK
//! \{

template<typename Encoding = GridFormat::Encoding::Base64,
         typename Compression = None,
         typename DataFormat = Automatic>
struct XMLOptions {
    Encoding encoder = Encoding{};
    Compression compression = Compression{};
    DataFormat data_format = DataFormat{};
};

template<typename CoordPrecision = Automatic,
         typename HeaderPrecision = Precision<std::size_t>>
struct PrecisionOptions {
    CoordPrecision coordinate_precision = {};
    HeaderPrecision header_precision = {};
};

#ifndef DOXYGEN
namespace Detail {

template<typename Opts> struct Encoding;
template<typename Opts> struct Compression;
template<typename Opts> struct DataFormat;
template<typename Opts> struct CoordinatePrecision;
template<typename Opts> struct HeaderPrecision;

template<typename E, typename C, typename F>
struct Encoding<XMLOptions<E, C, F>> : public std::type_identity<E> {};
template<typename E, typename C, typename F>
struct Compression<XMLOptions<E, C, F>> : public std::type_identity<C> {};

template<typename E, typename C, typename F>
struct DataFormat<XMLOptions<E, C, F>> : public std::type_identity<F> {};
template<typename E, typename C>
struct DataFormat<XMLOptions<E, C, Automatic>>
: public std::type_identity<
    std::conditional_t<
        is_any_of<E, GridFormat::Encoding::Ascii, GridFormat::Encoding::AsciiWithOptions>,
        VTK::DataFormat::Inlined,
        VTK::DataFormat::Appended
    >
> {};

template<typename T> struct PrecisionType;
template<typename T> struct PrecisionType<Precision<T>> : public std::type_identity<T> {};
template<> struct PrecisionType<Automatic> : public std::type_identity<Automatic> {};

template<typename C, typename H>
struct CoordinatePrecision<PrecisionOptions<C, H>> : public PrecisionType<C> {};
template<typename C, typename H>
struct HeaderPrecision<PrecisionOptions<C, H>> : public PrecisionType<H> {};

template<typename XMLOpts>
inline constexpr bool is_valid_data_format
    = !(std::is_same_v<typename Encoding<XMLOpts>::type, GridFormat::Encoding::Ascii> &&
        std::is_same_v<typename DataFormat<XMLOpts>::type, VTK::DataFormat::Appended>)
    && !(std::is_same_v<typename Encoding<XMLOpts>::type, GridFormat::Encoding::RawBinary> &&
        std::is_same_v<typename DataFormat<XMLOpts>::type, VTK::DataFormat::Inlined>);

template<typename Opts, typename Grid>
using CoordinateType = std::conditional_t<
    std::is_same_v<typename CoordinatePrecision<Opts>::type, Automatic>,
    CoordinateType<Grid>,
    typename CoordinatePrecision<Opts>::type
>;

template<typename Opts>
using HeaderType = std::conditional_t<
    std::is_same_v<typename HeaderPrecision<Opts>::type, Automatic>,
    std::size_t,
    typename HeaderPrecision<Opts>::type
>;

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::Grid G,
         typename XMLOpts = XMLOptions<>,
         typename PrecOpts = PrecisionOptions<>>
class XMLWriterBase : public GridWriter<G> {
    using ParentType = GridWriter<G>;

    static_assert(
        Detail::is_valid_data_format<XMLOpts>,
        "Incompatible choice of encoding (ascii/base64/binary) and data format (inlined/appended)"
    );

    static constexpr std::size_t vtk_space_dim = 3;
    static constexpr bool use_ascii = is_any_of<
        typename Detail::Encoding<XMLOpts>::type,
        Encoding::Ascii,
        Encoding::AsciiWithOptions
    >;
    static constexpr bool use_compression = !std::is_same_v<
        typename Detail::Compression<XMLOpts>::type, None
    >;
    static constexpr bool write_inline = std::is_same_v<
        typename Detail::DataFormat<XMLOpts>::type, DataFormat::Inlined
    >;

 public:
    //! Export underlying grid type
    using Grid = G;

    explicit XMLWriterBase(const Grid& grid,
                           std::string extension,
                           XMLOpts xml_opts = {},
                           PrecOpts prec_opts = {})
    : ParentType(grid, std::move(extension))
    , _xml_opts{std::move(xml_opts)}
    , _prec_opts{std::move(prec_opts)} {
        if constexpr (use_compression && use_ascii)
            log_warning("Cannot compress ascii-encoded output, ignoring chosen compression");
    }

 protected:
    using CoordinateType = Detail::CoordinateType<PrecOpts, Grid>;
    using HeaderType = Detail::HeaderType<PrecOpts>;
    static_assert(
        std::unsigned_integral<HeaderType> && (sizeof(HeaderType) == 4 || sizeof(HeaderType) == 8),
        "VTK supports only UInt32 and UInt64 as header types"
    );

    XMLOpts _xml_opts;
    PrecOpts _prec_opts;

    auto _data_format() const {
        if constexpr (std::is_same_v<decltype(_xml_opts.data_format), Automatic>)
            return typename Detail::DataFormat<XMLOpts>::type{};
        else
            return _xml_opts.data_format;
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
        xml.set_attribute("header_type", attribute_name(Precision<HeaderType>{}));
        xml.set_attribute("byte_order", attribute_name(std::endian::native));
        xml.add_child(vtk_grid_type);
        if constexpr (use_compression)
            xml.set_attribute("compressor", attribute_name(_xml_opts.compression));
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
        XMLElement& da = _access_element(context, _get_element_names(xml_group)).add_child("DataArray");
        da.set_attribute("Name", std::move(data_array_name));
        da.set_attribute("type", attribute_name(field.precision()));
        da.set_attribute("format", data_format_name(_xml_opts.encoder, _data_format()));

        const auto layout = field.layout();
        da.set_attribute("NumberOfComponents", (layout.dimension() == 1 ? 1 : layout.number_of_entries(1)));
        if constexpr (write_inline)
            da.set_content(
                DataArray{field, _xml_opts.encoder, _xml_opts.compression, Precision<HeaderType>{}}
            );
        else
            context.appendix.add(
                DataArray{field, _xml_opts.encoder, _xml_opts.compression, Precision<HeaderType>{}}
            );
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
        if constexpr (write_inline)
            write_xml_with_version_header(context.xml_representation, s, indentation);
        else
            XML::Detail::write_with_appendix(std::move(context), s, _xml_opts.encoder, indentation);
    }
};

//! \} group VTK

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_XML_HPP_
