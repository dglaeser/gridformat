// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <numbers>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_2.h>

#include <gridformat/gridformat.hpp>
#include <gridformat/traits/cgal.hpp>


using Triangulation = CGAL::Triangulation_2<CGAL::Exact_predicates_inexact_constructions_kernel>;

void add_points(Triangulation& triangulation) {
    const auto add_points_at_radius = [&] (double r) {
        const int num_samples = 10;
        for (int i = 0; i < num_samples; ++i) {
            const auto angle = 2.0*std::numbers::pi*i/num_samples;
            triangulation.insert({r*std::cos(angle), r*std::sin(angle)});
        }
    };
    add_points_at_radius(0.5);
    add_points_at_radius(1.0);
}

int main() {
    Triangulation triangulation;
    add_points(triangulation);

    // We've seen in the previous examples how to select options on a file format
    // The VTK-XML formats provide another convenient way to select those options:
    const auto format = GridFormat::vtp.with_encoding(GridFormat::Encoding::base64)
                                       .with_data_format(GridFormat::VTK::DataFormat::appended)
                                       .with_compression(GridFormat::none);
    GridFormat::Writer writer{format, triangulation};

    // The predefined cgal traits yield CGAL handles for cells/points. That is,
    // the point type is Triangulation::Vertex_handle, while the cell type is
    // Triangulation::Face_handle (in 2D) or Triangulation::Cell_handle (in 3D)
    // These handles behave like pointers, and we need to dereference them.
    writer.set_point_field("pfield", [] (const auto& vertex_handle) {
        const auto vertex_x_pos = vertex_handle->point().x();
        return std::sin(vertex_x_pos*std::numbers::pi*3.0);
    });

    writer.set_cell_field("cfield", [] (const auto& cell_handle) {
        const auto first_cell_vertex_x_pos = cell_handle->vertex(0)->point().x();
        return std::sin(first_cell_vertex_x_pos*std::numbers::pi*3.0);
    });

    const auto filename = writer.write("cgal_triangulation");
    std::cout << "Wrote '" << filename << "'" << std::endl;
    return 0;
}
