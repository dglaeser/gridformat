// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <numbers>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_2.h>

#include <gridformat/grid/adapters/cgal.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

// CGAL triangulations do not have a standard way of associating indices
// with grid vertices. One approach is to use the vertex type with an info
// object, which we can use to attach indices to grid vertices
struct VertexInfo { std::size_t id; };

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Vertex = CGAL::Triangulation_vertex_base_with_info_2<VertexInfo, Kernel>;
using Triangulation = CGAL::Triangulation_2<Kernel, CGAL::Triangulation_data_structure_2<Vertex>>;


// Since there is no defined way of retrieving vertex indices in CGAL triangulations,
// the respective trait is not predefined. After having defined the info object, we can now implement it.
namespace GridFormat::Traits {

template<>
struct PointId<Triangulation, typename Triangulation::Vertex> {
    static std::size_t get(const Triangulation&, const typename Triangulation::Vertex& v) {
        return v.info().id;
    }
};

}  // namespace GridFormat::Traits


void set_vertex_indices(Triangulation& triangulation) {
    std::ranges::for_each(triangulation.finite_vertex_handles(), [i=int{0}] (auto handle) mutable {
        handle->info().id = i++;
    });
}

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
    set_vertex_indices(triangulation);

    const auto filename = GridFormat::VTPWriter{triangulation}
                            .with_data_format(GridFormat::VTK::DataFormat::appended)
                            .with_encoding(GridFormat::Encoding::base64)
                            .with_compression(GridFormat::none)
                            .write("cgal_triangulation");
    std::cout << "Wrote '" << filename << "'" << std::endl;
    return 0;
}
