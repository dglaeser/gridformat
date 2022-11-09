// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_XML_WRITER_BASE_HPP_
#define GRIDFORMAT_VTK_XML_WRITER_BASE_HPP_

#include <string>
#include <ranges>
#include <utility>
#include <iterator>

#include <gridformat/common/precision.hpp>
#include <gridformat/common/writer.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/range_formatter.hpp>
#include <gridformat/common/streamable_field.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/options.hpp>
#include <gridformat/vtk/attributes.hpp>
#include <gridformat/vtk/_traits.hpp>

namespace GridFormat::VTK {

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::Grid Grid,
         typename Format = VTK::DataFormat::Inlined,
         typename Encoding = VTK::Encoding::Ascii>
class XMLWriterBase : public WriterBase {
    static constexpr std::size_t vtk_space_dim = 3;

 public:
    explicit XMLWriterBase(const Grid& grid,
                           std::string extension,
                           [[maybe_unused]] const Format& = {},
                           [[maybe_unused]] const Encoding& = {})
    : _grid(grid)
    , _extension(std::move(extension))
    {}

    using WriterBase::write;
    void write(const std::string& filename) const {
        WriterBase::write(filename + _extension);
    }

    template<typename T>
    void set_header_precision(const Precision<T>& prec) {
        _header_precision = prec;
    }

    template<typename T>
    void set_coordinate_precision(const Precision<T>& prec) {
        _coordinate_precision = prec;
    }

    using WriterBase::set_point_field;
    using WriterBase::set_cell_field;

    //! Vectors need to be made 3d for VTK
    template<Concepts::Vectors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_point_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_point_field( name, _make_vector_range(std::forward<V>(v)), prec);
    }

    //! Tensors need to be made 3d for VTK
    template<Concepts::Tensors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_point_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_point_field( name, _make_tensor_range(std::forward<V>(v)), prec);
    }

    //! Vectors need to be made 3d for VTK
    template<Concepts::Vectors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_cell_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_cell_field( name, _make_vector_range(std::forward<V>(v)), prec);
    }

    //! Tensors need to be made 3d for VTK
    template<Concepts::Tensors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_cell_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_cell_field( name, _make_tensor_range(std::forward<V>(v)), prec);
    }

 protected:
    const Grid& _grid;
    std::string _extension;
    PrecisionTraits _header_precision = Precision<std::size_t>{};
    PrecisionTraits _coordinate_precision = Precision<CoordinateType<Grid>>{};
    RangeFormatOptions _range_format_opts = {.delimiter = " ", .line_prefix = std::string(10, ' ')};

    struct WriteContext {
        std::string vtk_grid_type;
        XMLElement xml_representation;
        int appendix;
    };

    WriteContext _get_write_context(std::string vtk_grid_type) const {
        XMLElement xml("VTKFile");
        xml.set_attribute("type", vtk_grid_type);
        xml.add_child(vtk_grid_type);
        return {
            .vtk_grid_type = std::move(vtk_grid_type),
            .xml_representation = std::move(xml),
            .appendix = 1
        };
    }

    template<typename ValueType>
    void _set_attribute(WriteContext& context,
                        std::string_view xml_group,
                        const std::string& attr_name,
                        const ValueType& attr_value) const {
        _access_element(context, _get_element_names(xml_group)).set_attribute(attr_name, attr_value);
    }

    template<typename F> requires(std::derived_from<std::decay_t<F>, Field>)
    void _set_data_array(WriteContext& context,
                         std::string_view xml_group,
                         std::string data_array_name,
                         F&& field) const {
        XMLElement& da = _access_element(context, _get_element_names(xml_group)).add_child("DataArray");
        da.set_attribute("Name", std::move(data_array_name));
        da.set_attribute("type", attribute_name(field.precision()));
        if (field.layout().dimension() == 1)
            da.set_attribute("NumberOfComponents", "1");
        else
            da.set_attribute("NumberOfComponents", field.layout().sub_layout(1).number_of_entries());
        da.set_content(make_streamable(std::forward<F>(field), _range_format_opts));
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

    void _write_xml(const WriteContext& context, std::ostream& s) const {
        write_xml_with_version_header(context.xml_representation, s, Indentation{{.width = 2}});
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

#endif  // GRIDFORMAT_VTK_XML_WRITER_BASE_HPP_