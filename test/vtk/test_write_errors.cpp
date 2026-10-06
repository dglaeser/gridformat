// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>

#include <gridformat/grid/image_grid.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/parallel/traits.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>
#include <gridformat/vtk/pvti_writer.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>
#include <gridformat/vtk/pvts_writer.hpp>

#include "../testing.hpp"

// create a directory at the path of the file to be written, such that opening it fails
void block_file(const std::string& filename) {
    std::filesystem::remove_all(filename);
    std::filesystem::create_directory(filename);
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;

    const GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {2, 2}};
    const GridFormat::NullCommunicator comm;

    "writer_throws_on_non_existing_directory"_test = [&] () {
        expect(throws<GridFormat::IOError>([&] () {
            GridFormat::VTUWriter{grid}.write("non_existing_dir/out");
        }));
    };

    "pvd_writer_throws_on_unwritable_pvd_file"_test = [&] () {
        block_file("test_write_errors_series.pvd");
        GridFormat::PVDWriter writer{GridFormat::VTUWriter{grid}, "test_write_errors_series"};
        expect(throws<GridFormat::IOError>([&] () { writer.write(0.0); }));
    };

    "parallel_writers_throw_on_unwritable_main_file"_test = [&] () {
        block_file("test_write_errors_parallel.pvtu");
        expect(throws<GridFormat::IOError>([&] () {
            GridFormat::PVTUWriter{grid, comm}.write("test_write_errors_parallel");
        }));
        block_file("test_write_errors_parallel.pvtp");
        expect(throws<GridFormat::IOError>([&] () {
            GridFormat::PVTPWriter{grid, comm}.write("test_write_errors_parallel");
        }));
        block_file("test_write_errors_parallel.pvti");
        expect(throws<GridFormat::IOError>([&] () {
            GridFormat::PVTIWriter{grid, comm}.write("test_write_errors_parallel");
        }));
        block_file("test_write_errors_parallel.pvtr");
        expect(throws<GridFormat::IOError>([&] () {
            GridFormat::PVTRWriter{grid, comm}.write("test_write_errors_parallel");
        }));
        block_file("test_write_errors_parallel.pvts");
        expect(throws<GridFormat::IOError>([&] () {
            GridFormat::PVTSWriter{grid, comm}.write("test_write_errors_parallel");
        }));
    };

    return 0;
}
