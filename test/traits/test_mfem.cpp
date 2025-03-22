// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/mfem.hpp>

#include "mfem.hpp"

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"

template<typename MFEMPoint>
double eval_test_function(const MFEMPoint& p, int dim) {
    if (dim == 1)
        return GridFormat::Test::test_function<double>(std::array{p[0]});
    else if (dim == 2)
        return GridFormat::Test::test_function<double>(std::array{p[0], p[1]});
    else
        return GridFormat::Test::test_function<double>(std::array{p[0], p[1], p[2]});
}

template<typename Mesh>
void test(Mesh mesh, const std::string& suffix = "") {
    std::string base_filename =
        "mfem_"
        + std::to_string(mesh.Dimension()) + "d_in_"
        + std::to_string(mesh.SpaceDimension()) + "d"
        + (suffix.empty() ? "" : "_" + suffix);

    GridFormat::VTUWriter writer{mesh};
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return eval_test_function(mesh.GetVertex(vertex), mesh.SpaceDimension());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        mfem::Vector v;
        mesh.GetElementCenter(element, v);
        return eval_test_function(v, mesh.SpaceDimension());
    });
    std::cout << "Wrote '" << writer.write(base_filename) << "'" << std::endl;

    if (mesh.Dimension() < 3) {
        GridFormat::VTPWriter poly_writer{mesh};
        GridFormat::Test::add_meta_data(poly_writer);
        poly_writer.set_point_field("pfunc", [&] (const auto& vertex) {
            return eval_test_function(mesh.GetVertex(vertex), mesh.SpaceDimension());
        });
        poly_writer.set_cell_field("cfunc", [&] (const auto& element) {
            mfem::Vector v;
            mesh.GetElementCenter(element, v);
            return eval_test_function(v, mesh.SpaceDimension());
        });
        std::cout << "Wrote '" << poly_writer.write(base_filename + "_as_poly") << "'" << std::endl;
    }

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
}


int main() {
    test(mfem::Mesh::MakeCartesian1D(15, mfem::Element::Type::SEGMENT));

    test(mfem::Mesh::MakeCartesian2D(8, 10, mfem::Element::Type::TRIANGLE));
    test(mfem::Mesh::MakeCartesian2D(8, 10, mfem::Element::Type::QUADRILATERAL));

    test(mfem::Mesh::MakeCartesian3D(5, 6, 7, mfem::Element::Type::TETRAHEDRON));
    test(mfem::Mesh::MakeCartesian3D(5, 6, 7, mfem::Element::Type::HEXAHEDRON));

    return 0;
}
