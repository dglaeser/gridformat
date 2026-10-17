// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>
#include <vector>
#include <array>
#include <cstddef>

#include <mpi.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>
#include <gridformat/vtk/pvti_writer.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>
#include <gridformat/vtk/pvts_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../testing.hpp"

// create a directory at the path of the file to be written, such that opening it fails
void block_file(const std::string& filename) {
    std::filesystem::remove_all(filename);
    std::filesystem::create_directory(filename);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;

    const auto comm = MPI_COMM_WORLD;
    const int rank = GridFormat::Parallel::rank(comm);
    const GridFormat::Test::StructuredGrid<2> grid{
        {{1.0, 1.0}},
        {{2, 2}},
        {{static_cast<double>(rank), 0.0}}
    };

    // failures on a single rank must make all ranks throw instead of leaving the others waiting
    const auto test_writer = [&] (const std::string& ext, const std::string& piece_ext, const auto& make_writer) {
        const std::string piece_failure_base = "test_parallel_write_errors_piece" + ext;
        if (rank == 1)
            block_file(piece_failure_base + "-1" + piece_ext);
        expect(throws<GridFormat::IOError>([&] () { make_writer().write(piece_failure_base); }));

        const std::string root_failure_base = "test_parallel_write_errors_root" + ext;
        if (rank == 0)
            block_file(root_failure_base + ext);
        expect(throws<GridFormat::IOError>([&] () { make_writer().write(root_failure_base); }));
    };

    "pvtu_writer_throws_on_all_ranks"_test = [&] () {
        test_writer(".pvtu", ".vtu", [&] () { return GridFormat::PVTUWriter{grid, comm}; });
    };

    "pvtp_writer_throws_on_all_ranks"_test = [&] () {
        test_writer(".pvtp", ".vtp", [&] () { return GridFormat::PVTPWriter{grid, comm}; });
    };

    "pvti_writer_throws_on_all_ranks"_test = [&] () {
        test_writer(".pvti", ".vti", [&] () { return GridFormat::PVTIWriter{grid, comm}; });
    };

    "pvtr_writer_throws_on_all_ranks"_test = [&] () {
        test_writer(".pvtr", ".vtr", [&] () { return GridFormat::PVTRWriter{grid, comm}; });
    };

    "pvts_writer_throws_on_all_ranks"_test = [&] () {
        test_writer(".pvts", ".vts", [&] () { return GridFormat::PVTSWriter{grid, comm}; });
    };

    "parallel_pvd_writer_throws_on_all_ranks"_test = [&] () {
        if (rank == 0)
            block_file("test_parallel_write_errors_series.pvd");
        GridFormat::PVDWriter writer{GridFormat::PVTUWriter{grid, comm}, "test_parallel_write_errors_series"};
        expect(throws<GridFormat::IOError>([&] () { writer.write(0.0); }));
    };

    "structured_grid_helper_throws_on_all_ranks"_test = [&] () {
        // the extents are computed on the root only, which throws because of the missing extents
        const GridFormat::PVTK::StructuredParallelGridHelper helper{comm};
        const auto compute = [&] () {
            const std::vector<double> all_origins{0.0, 0.0, 1.0, 0.0};
            helper.compute_extents_and_origin(all_origins, std::vector<std::size_t>{}, std::array{false, false});
        };
        if (rank == 0)
            expect(throws<GridFormat::SizeError>(compute));
        else
            expect(throws<GridFormat::IOError>(compute));
    };

    MPI_Finalize();
    return 0;
}
