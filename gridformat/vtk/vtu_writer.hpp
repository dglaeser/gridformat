// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_VTU_WRITER_HPP_
#define GRIDFORMAT_VTK_VTU_WRITER_HPP_

#include <string>
#include <fstream>
#include <utility>

#include <gridformat/common/writer.hpp>
#include <gridformat/common/fields.hpp>
#include <gridformat/common/extended_range.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/common.hpp>

namespace GridFormat {

template<typename Field>
class StreamedField {
 public:
    explicit StreamedField(const Field& field)
    : _field(field)
    {}

    friend std::ostream& operator<<(std::ostream& s, const StreamedField& streamed) {
        streamed._field.stream(s);
        return s;
    }

 private:
    const Field& _field;
};

template<typename Field>
void add_data_array(const Field& field,
                    const std::string_view name,
                    XMLElement& element) {
    auto& data_array = element.add_child("DataArray");
    data_array.set_attribute("Name", name);
    data_array.set_attribute("type", VTK::data_format(field.precision()));
    data_array.set_attribute("NumberOfComponents", field.number_of_components());
    data_array.set_content(StreamedField{field});
}

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::UnstructuredGrid Grid>
class VTUWriter : public Writer {
    static constexpr int indentation_width = 2;
    static constexpr int data_array_indentation = indentation_width*5;
 public:
    explicit VTUWriter(const Grid& grid)
    : Writer({.delimiter = " ", .line_prefix = std::string(data_array_indentation, ' ')})
    , _grid(grid)
    {}

    void write(const std::string& filename) const {
        std::ofstream file{filename, std::ios::out};
        write(file);
    }

    void write(std::ostream& s) const {
        auto points = GridFormat::Grid::points(_grid);
        auto cells = GridFormat::Grid::cells(_grid);
        const auto num_points = GridFormat::Grid::num_points(_grid);
        const auto num_cells = GridFormat::Grid::num_cells(_grid);

        std::size_t i = 0;
        std::vector<std::size_t> point_ids(num_points);
        for (const auto& point : points)
            point_ids[GridFormat::Grid::id(_grid, point)] = i++;

        XMLElement vtu_file{"VTKFile"};
        vtu_file.set_attribute("type", "UnstructuredGrid");

        auto& piece = vtu_file.add_child("UnstructuredGrid").add_child("Piece");
        piece.set_attribute("NumberOfPoints", num_points);
        piece.set_attribute("NumberOfCells", num_cells);

        auto& point_data_element = piece.add_child("PointData");
        auto& cell_data_element = piece.add_child("CellData");
        std::ranges::for_each(this->_point_data_names(), [&] (const std::string& name) {
            add_data_array(this->_get_point_data(name), name, point_data_element);
        });
        std::ranges::for_each(this->_cell_data_names(), [&] (const std::string& name) {
            add_data_array(this->_get_cell_data(name), name, cell_data_element);
        });

        auto& points_element = piece.add_child("Points");
        auto& cells_element = piece.add_child("Cells");

        VectorField points_field{
            std::views::all(points) | std::views::transform([&] <std::ranges::range P> (P&& point) {
                if constexpr (std::is_lvalue_reference_v<P>)
                    return make_extended<3>(std::views::all(point));
                else
                    return make_extended<3>(std::move(point));
            }),
            this->_format_opts
        };
        add_data_array(points_field, "Coordinates", points_element);

        FlatVectorField connectivity_field{
            cells
            | std::views::transform([&] (const auto& cell) {
                return GridFormat::Grid::corners(_grid, cell)
                    | std::views::transform([&] (const auto& p) {
                        return point_ids[GridFormat::Grid::id(_grid, p)];
                });
            }),
            this->_format_opts
        };
        add_data_array(connectivity_field, "connectivity", cells_element);

        std::vector<std::size_t> offsets;
        offsets.reserve(num_cells);
        for (const auto& cell : cells)
            offsets.push_back(
                std::ranges::distance(GridFormat::Grid::corners(_grid, cell))
                + (offsets.empty() ? 0 : offsets.back())
            );

        ScalarField offsets_field{std::views::all(offsets), this->_format_opts};
        add_data_array(offsets_field, "offsets", cells_element);

        ScalarField cell_types_field{
            cells | std::views::transform([&] (const auto& cell) {
                return VTK::cell_type(GridFormat::Grid::type(_grid, cell));
            }),
            this->_format_opts
        };
        add_data_array(cell_types_field, "types", cells_element);

        write_xml_with_version_header(vtu_file, s, Indentation{{.width = indentation_width}});
    }

 private:
    const Grid& _grid;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_