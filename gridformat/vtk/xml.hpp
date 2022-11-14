// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_XML_HPP_
#define GRIDFORMAT_VTK_XML_HPP_

#include <bit>
#include <string>
#include <ranges>
#include <utility>
#include <type_traits>

#include <gridformat/common/precision.hpp>
#include <gridformat/common/writer.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/streamable_field.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/encoding/ascii.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/attributes.hpp>
#include <gridformat/vtk/data_array.hpp>
#include <gridformat/vtk/appendix.hpp>

namespace GridFormat::VTK {

template<typename Encoding = GridFormat::Encoding::Base64,
         typename Compression = None,
         typename DataFormat = Automatic>
struct XMLOptions {
    Encoding encoder = Encoding{};
    Compression compression = Compression{};
    DataFormat format = DataFormat{};
};

template<typename CoordPrecision = Automatic,
         typename HeaderPrecision = Precision<std::size_t>>
struct PrecisionOptions {
    CoordPrecision coordinate_precision = {};
    HeaderPrecision header_precision = {};
};

#ifndef DOXYGEN
namespace Detail {

template<typename Opts> struct Format;
template<typename Opts> struct Encoding;
template<typename Opts> struct Compression;
template<typename Opts> struct CoordinatePrecision;
template<typename Opts> struct HeaderPrecision;

template<typename E, typename C, typename F>
struct Encoding<XMLOptions<E, C, F>> : public std::type_identity<E> {};
template<typename E, typename C, typename F>
struct Compression<XMLOptions<E, C, F>> : public std::type_identity<C> {};

template<typename E, typename C, typename F>
struct Format<XMLOptions<E, C, F>> : public std::type_identity<F> {};
template<typename E, typename C>
struct Format<XMLOptions<E, C, Automatic>>
: public std::type_identity<
    std::conditional_t<
        is_any_of<E, GridFormat::Encoding::Ascii, GridFormat::Encoding::AsciiWithOptions>,
        DataFormat::Inlined,
        DataFormat::Appended
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
        std::is_same_v<typename Format<XMLOpts>::type, DataFormat::Appended>)
    && !(std::is_same_v<typename Encoding<XMLOpts>::type, GridFormat::Encoding::RawBinary> &&
        std::is_same_v<typename Format<XMLOpts>::type, DataFormat::Inlined>);

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
template<Concepts::Grid Grid,
         typename XMLOpts = XMLOptions<>,
         typename PrecOpts = PrecisionOptions<>>
class XMLWriterBase : public WriterBase {
    static_assert(
        Detail::is_valid_data_format<XMLOpts>,
        "Incompatible choice of encoding (ascii/base64/binary) and data format (inlined/appended)"
    );
    static_assert(
        std::is_integral_v<Detail::HeaderType<PrecOpts>>,
        "Only integral types can be used for headers"
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
        typename Detail::Format<XMLOpts>::type, DataFormat::Inlined
    >;

 public:
    explicit XMLWriterBase(const Grid& grid,
                           std::string extension,
                           XMLOpts xml_opts = {},
                           PrecOpts prec_opts = {})
    : _grid{grid}
    , _extension{std::move(extension)}
    , _xml_opts{std::move(xml_opts)}
    , _prec_opts{std::move(prec_opts)} {
        if constexpr (use_compression && use_ascii)
            Logging::as_warning("Cannot compress ascii-encoded output, ignoring chosen compression");
    }

    using WriterBase::write;
    void write(const std::string& filename) const {
        WriterBase::write(filename + _extension);
    }

    using WriterBase::set_point_field;
    using WriterBase::set_cell_field;

    //! Vectors need to be made 3d for VTK
    template<Concepts::Vectors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_point_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_point_field(name, _make_vector_range(std::forward<V>(v)), prec);
    }

    //! Tensors need to be made 3d for VTK
    template<Concepts::Tensors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_point_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_point_field(name, _make_tensor_range(std::forward<V>(v)), prec);
    }

    //! Vectors need to be made 3d for VTK
    template<Concepts::Vectors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_cell_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_cell_field(name, _make_vector_range(std::forward<V>(v)), prec);
    }

    //! Tensors need to be made 3d for VTK
    template<Concepts::Tensors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_cell_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_cell_field(name, _make_tensor_range(std::forward<V>(v)), prec);
    }

 protected:
    using CoordinateType = Detail::CoordinateType<PrecOpts, Grid>;
    using HeaderType = Detail::HeaderType<PrecOpts>;

    const Grid& _grid;
    std::string _extension;
    XMLOpts _xml_opts;
    PrecOpts _prec_opts;

    auto _format() const {
        if constexpr (std::is_same_v<decltype(_xml_opts.format), Automatic>)
            return typename Detail::Format<XMLOpts>::type{};
        else
            return _xml_opts.format;
    }

    struct WriteContext {
        std::string vtk_grid_type;
        XMLElement xml_representation;
        Appendix appendix;
    };

    WriteContext _get_write_context(std::string vtk_grid_type) const {
        XMLElement xml("VTKFile");
        xml.set_attribute("type", vtk_grid_type);
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
        da.set_attribute("format", data_format_name(_xml_opts.encoder, _format()));
        if (field.dimension() == 1)
            da.set_attribute("NumberOfComponents", "1");
        else
            da.set_attribute("NumberOfComponents", field.number_of_entries(1));
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

 private:
    template<Concepts::Vectors V>
    auto _make_vector_range(V&& v) {
        return std::forward<V>(v) | std::views::transform([] <std::ranges::range R> (R&& r) {
            return make_extended<vtk_space_dim>(std::forward<R>(r));
        });
    }

    template<Concepts::Tensors T>
    auto _make_tensor_range(T&& t) {
        return std::forward<T>(t) | std::views::transform([] <std::ranges::range Tensor> (Tensor outer) {
            using Vector = std::ranges::range_value_t<Tensor>;
            using Scalar = std::ranges::range_value_t<Vector>;
            static_assert(Concepts::StaticallySized<Vector> && "Tensor expansion expects statically sized tensor rows");

            Vector last_row;
            std::ranges::fill(last_row, Scalar{0.0});
            auto extended_tensor = make_extended<vtk_space_dim>(std::move(outer), std::move(last_row));
            return std::ranges::owning_view{std::move(extended_tensor)}
                | std::views::transform([] <typename V> (V&& vector) {
                    return make_extended<vtk_space_dim>(std::forward<V>(vector), Scalar{0.0});
            });
        });
    }
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_XML_HPP_