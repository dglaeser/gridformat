// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/mfem.hpp>

#include "mfem.hpp"

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"


int main() {
    auto mesh = mfem::Mesh::MakeCartesian2D(15, 10, mfem::Element::Type::QUADRILATERAL);

    GridFormat::VTUWriter writer{mesh};
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(std::array{
            mesh.GetVertex(vertex)[0],
            mesh.GetVertex(vertex)[1]
        });
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        mfem::Vector v;
        mesh.GetElementCenter(element, v);
        return GridFormat::Test::test_function<double>(std::array{v[0], v[1]});
    });
    std::cout << "Wrote '" << writer.write("mfem_2d_in_2d") << "'" << std::endl;

    GridFormat::VTPWriter poly_writer{mesh};
    GridFormat::Test::add_meta_data(poly_writer);
    poly_writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(std::array{
            mesh.GetVertex(vertex)[0],
            mesh.GetVertex(vertex)[1]
        });
    });
    poly_writer.set_cell_field("cfunc", [&] (const auto& element) {
        mfem::Vector v;
        mesh.GetElementCenter(element, v);
        return GridFormat::Test::test_function<double>(std::array{v[0], v[1]});
    });
    std::cout << "Wrote '" << poly_writer.write("mfem_2d_in_2d_as_poly") << "'" << std::endl;

    // run a bunch of unit tests
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "number_of_cells"_test = [&] () {
        expect(eq(
            GridFormat::Ranges::size(GridFormat::cells(mesh)),
            static_cast<std::size_t>(GridFormat::Traits::NumberOfCells<mfem::Mesh>::get(mesh))
        ));
    };
    "number_of_vertices"_test = [&] () {
        expect(eq(
            GridFormat::Ranges::size(GridFormat::points(mesh)),
            static_cast<std::size_t>(GridFormat::Traits::NumberOfPoints<mfem::Mesh>::get(mesh))
        ));
    };
    "number_of_cell_points"_test = [&] () {
        for (const auto& c : GridFormat::cells(mesh))
            expect(eq(
                GridFormat::Ranges::size(GridFormat::points(mesh, c)),
                static_cast<std::size_t>(
                    GridFormat::Traits::NumberOfCellPoints<mfem::Mesh, std::decay_t<decltype(c)>>::get(mesh, c)
                )
            ));
    };

    return 0;
}
