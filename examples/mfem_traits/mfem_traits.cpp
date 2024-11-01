// SPDX-FileCopyrightText: 2023 Timo Koch <timokoch@uio.no>
// SPDX-License-Identifier: MIT

#include "mfem.hpp"

#include <gridformat/gridformat.hpp>
#include <gridformat/traits/mfem.hpp>

int main(int argc, char *argv[])
{
    // make sure mfem::Mesh satisfies the UnstructuredGrid concept
    static_assert(GridFormat::Concepts::UnstructuredGrid<mfem::Mesh>);

    // read mesh
    const char* mesh_file = "turtle.msh";
    mfem::Mesh mesh(mesh_file, 1, 1);
    mesh.UniformRefinement();

    // Follow mfem example to solve -Δu = 1 with homogenous Dirichlet BC
    mfem::H1_FECollection fe_collection(1, mesh.Dimension());
    mfem::FiniteElementSpace fe_space(&mesh, &fe_collection);
    std::cout << "Number of unknowns: " << fe_space.GetTrueVSize() << std::endl;

    mfem::Array<int> boundary_dofs;
    fe_space.GetBoundaryTrueDofs(boundary_dofs);
    mfem::GridFunction x(&fe_space);
    x = 0.0;

    mfem::ConstantCoefficient one(1.0);
    mfem::LinearForm b(&fe_space);
    b.AddDomainIntegrator(new mfem::DomainLFIntegrator(one));
    b.Assemble();
    mfem::BilinearForm a(&fe_space);
    a.AddDomainIntegrator(new mfem::DiffusionIntegrator);
    a.Assemble();

    mfem::SparseMatrix A;
    mfem::Vector B, X;
    a.FormLinearSystem(boundary_dofs, x, b, A, X, B);

    mfem::GSSmoother M(A);
    mfem::PCG(A, M, B, X, 1, 200, 1e-12, 0.0);
    a.RecoverFEMSolution(X, b, x);

    // create writer
    GridFormat::Writer writer{ GridFormat::vtu, mesh };

    // attach FEM solution as output
    mfem::Vector x_at_nodes;
    x.GetNodalValues(x_at_nodes);
    writer.set_point_field("u", [&] (const auto& point) { return x_at_nodes[point]; });

    // also output some analytic function
    assert(mesh.SpaceDimension() == 2);
    const auto f = [](const auto& x) { return x[0]*x[1]; };

    writer.set_point_field("xy", [&] (const auto& point) { return f(mesh.GetVertex(point)); });
    writer.set_cell_field("xy", [&] (const auto& cell) {
        mfem::Vector v;
        mesh.GetElementCenter(cell, v);
        return f(v);
    });

    // write data to file
    writer.write("mfem");

    return 0;
}
