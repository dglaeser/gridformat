// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <vector>
#include <ranges>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/pvtu_reader.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    if (GridFormat::Parallel::size(MPI_COMM_WORLD) != 4)
        throw std::runtime_error("This test requires to be run with four processes");

    const auto root_rank = 0;
    const auto world_comm = MPI_COMM_WORLD;
    const auto world_rank = GridFormat::Parallel::rank(world_comm);
    const auto my_write_color = world_rank < 3 ? int{0} : int{1};
    const auto my_read_color = world_rank < 2 ? int{0} : int{1};
    MPI_Comm write_comm;
    MPI_Comm_split(world_comm, my_write_color, world_rank, &write_comm);
    MPI_Comm read_comm;
    MPI_Comm_split(world_comm, my_read_color, world_rank, &read_comm);

    std::string test_filename = "";
    std::size_t num_cells_per_rank = 0;
    std::size_t num_points_per_rank = 0;
    std::vector<std::string> pfield_names;
    std::vector<std::string> cfield_names;
    std::vector<std::string> mfield_names;

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    // Write file with 3 processes
    if (my_write_color == 0) {
        std::cout << "Do write on rank " << world_rank << std::endl;
        const auto grid = GridFormat::Test::make_unstructured_2d(world_rank);
        num_cells_per_rank = GridFormat::number_of_cells(grid);
        num_points_per_rank = GridFormat::number_of_points(grid);
        GridFormat::PVTUWriter writer{grid, write_comm};
        GridFormat::PVTUReader reader{write_comm};
        test_filename = GridFormat::Test::test_reader<2, 2>(
            writer,
            reader,
            "reader_pvtu_test_file_2d_in_2d",
            {},
            (world_rank == 0)
        );

        std::ranges::copy(point_field_names(reader), std::back_inserter(pfield_names));
        std::ranges::copy(cell_field_names(reader), std::back_inserter(cfield_names));
        std::ranges::copy(meta_data_field_names(reader), std::back_inserter(mfield_names));

        "pvtu_reader_name"_test = [&] () {
            expect(reader.name() == "PVTUReader");
        };

        "parallel_pvtu_read_number_of_pieces"_test = [&] () {
            expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(GridFormat::Parallel::size(write_comm))));
        };
    }

    // broadcast info on the written files/fields
    const auto broadcast_string = [&] (const std::string& in) {
        std::string result;
        std::ranges::copy(
            GridFormat::Parallel::broadcast(world_comm, in, root_rank),
            std::back_inserter(result)
        );
        return result;
    };
    test_filename = broadcast_string(test_filename);
    num_cells_per_rank = GridFormat::Parallel::broadcast(world_comm, num_cells_per_rank, root_rank);
    num_points_per_rank = GridFormat::Parallel::broadcast(world_comm, num_points_per_rank, root_rank);
    pfield_names.resize(GridFormat::Parallel::broadcast(world_comm, cfield_names.size(), root_rank));
    cfield_names.resize(GridFormat::Parallel::broadcast(world_comm, pfield_names.size(), root_rank));
    mfield_names.resize(GridFormat::Parallel::broadcast(world_comm, mfield_names.size(), root_rank));
    for (auto& field_name : cfield_names) field_name = broadcast_string(field_name);
    for (auto& field_name : pfield_names) field_name = broadcast_string(field_name);
    for (auto& field_name : mfield_names) field_name = broadcast_string(field_name);
    std::cout << "Filename on rank " << world_rank << ": " << test_filename << std::endl;
    std::cout << "Number of cells (per rank) on rank " << world_rank << ": " << num_cells_per_rank << std::endl;
    std::cout << "Number of points (per rank) on rank " << world_rank << ": " << num_points_per_rank << std::endl;
    std::cout << "Meta data fields on rank " << world_rank << ": " << GridFormat::as_string(mfield_names) << std::endl;
    std::cout << "Point fields on rank " << world_rank << ": " << GridFormat::as_string(pfield_names) << std::endl;
    std::cout << "Cell fields on rank " << world_rank << ": " << GridFormat::as_string(cfield_names) << std::endl;


    // test that sequential I/O yields the expected results
    if (world_rank == 0) {
        std::cout << "Reading '" << test_filename << "' sequentially on rank " << world_rank << std::endl;
        GridFormat::PVTUReader reader{};
        reader.open(test_filename);

        "sequential_pvtu_read_number_of_entities"_test = [&] () {
            expect(eq(reader.number_of_cells(), num_cells_per_rank*3));
            expect(eq(reader.number_of_points(), num_points_per_rank*3));
        };

        "sequential_pvtu_read_field_names"_test = [&] () {
            expect(std::ranges::equal(point_field_names(reader), pfield_names));
            expect(std::ranges::equal(cell_field_names(reader), cfield_names));
            expect(std::ranges::equal(meta_data_field_names(reader), mfield_names));
        };

        const auto sequential_grid = [&] () {
            GridFormat::Test::UnstructuredGridFactory<2, 3> factory;
            reader.export_grid(factory);
            return std::move(factory).grid();
        } ();

        "sequential_pvtu_number_of_exported_entities"_test = [&] () {
            expect(eq(GridFormat::number_of_cells(sequential_grid), num_cells_per_rank*3));
            expect(eq(GridFormat::number_of_points(sequential_grid), num_points_per_rank*3));
        };

        "sequential_pvtu_read_point_fields"_test = [&] () {
            expect(GridFormat::Ranges::size(point_fields(reader)) > 0);
            for (const auto& [name, field_ptr] : point_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(
                    name, field_ptr, sequential_grid, GridFormat::points(sequential_grid)
                ));
        };

        "sequential_pvtu_read_cell_fields"_test = [&] () {
            expect(GridFormat::Ranges::size(cell_fields(reader)) > 0);
            for (const auto& [name, field_ptr] : cell_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(
                    name, field_ptr, sequential_grid, GridFormat::cells(sequential_grid)
                ));
        };
    }

    // test that when reading with more processes than pieces in the file, the last piece is empty
    GridFormat::Parallel::barrier(world_comm);
    {
        std::cout << "Reading '" << test_filename << "' on all 4 processes; rank = " << world_rank << std::endl;
        GridFormat::PVTUReader reader{world_comm};
        reader.open(test_filename);
        const auto proc_grid = [&] () {
            GridFormat::Test::UnstructuredGridFactory<2, 2> factory;
            reader.export_grid(factory);
            return std::move(factory).grid();
        } ();

        if (world_rank < 3) {
            "parallel_pvtu_more_procs_num_cells_on_non_empty_piece"_test = [&] () {
                expect(eq(reader.number_of_cells(), num_cells_per_rank));
                expect(eq(reader.number_of_points(), num_points_per_rank));
            };

            "parallel_pvtu_more_procs_point_fields_on_non_empty_piece"_test = [&] () {
                expect(GridFormat::Ranges::size(point_fields(reader)) > 0);
                for (const auto& [name, field_ptr] : point_fields(reader))
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, proc_grid, GridFormat::points(proc_grid)
                    ));
            };

            "parallel_pvtu_more_procs_cell_fields_on_non_empty_piece"_test = [&] () {
                expect(GridFormat::Ranges::size(cell_fields(reader)) > 0);
                for (const auto& [name, field_ptr] : cell_fields(reader))
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, proc_grid, GridFormat::cells(proc_grid)
                    ));
            };
        } else {
            "parallel_pvtu_more_procs_num_cells_on_empty_piece"_test = [&] () {
                expect(eq(reader.number_of_cells(), std::size_t{0}));
                expect(eq(reader.number_of_points(), std::size_t{0}));
                expect(eq(GridFormat::number_of_cells(proc_grid), std::size_t{0}));
                expect(eq(GridFormat::number_of_points(proc_grid), std::size_t{0}));
            };
        }
    }

    // test that exceeding pieces can be merged when reading with less ranks
    GridFormat::Parallel::barrier(world_comm);
    if (my_read_color == 0) {
        std::cout << "Reading '" << test_filename << "' on only 2 processes (with merging); rank = " << world_rank << std::endl;
        GridFormat::PVTUReader reader{read_comm, true};
        reader.open(test_filename);
        const auto proc_grid = [&] () {
            GridFormat::Test::UnstructuredGridFactory<2, 2> factory;
            reader.export_grid(factory);
            return std::move(factory).grid();
        } ();

        if (world_rank == 0) {
            "parallel_pvtu_less_procs_num_cells_on_normal_piece"_test = [&] () {
                expect(eq(reader.number_of_cells(), num_cells_per_rank));
                expect(eq(reader.number_of_points(), num_points_per_rank));
                expect(eq(GridFormat::number_of_cells(proc_grid), num_cells_per_rank));
                expect(eq(GridFormat::number_of_points(proc_grid), num_points_per_rank));
            };

            "parallel_pvtu_less_procs_point_fields_on_normal_piece"_test = [&] () {
                expect(GridFormat::Ranges::size(point_fields(reader)) > 0);
                for (const auto& [name, field_ptr] : point_fields(reader))
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, proc_grid, GridFormat::points(proc_grid)
                    ));
            };

            "parallel_pvtu_less_procs_cell_fields_on_normal_piece"_test = [&] () {
                expect(GridFormat::Ranges::size(cell_fields(reader)) > 0);
                for (const auto& [name, field_ptr] : cell_fields(reader))
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, proc_grid, GridFormat::cells(proc_grid)
                    ));
            };
        } else {
            "parallel_pvtu_less_procs_num_cells_on_merged_piece"_test = [&] () {
                expect(eq(reader.number_of_cells(), num_cells_per_rank*2));
                expect(eq(reader.number_of_points(), num_points_per_rank*2));
                expect(eq(GridFormat::number_of_cells(proc_grid), num_cells_per_rank*2));
                expect(eq(GridFormat::number_of_points(proc_grid), num_points_per_rank*2));
            };

            "parallel_pvtu_less_procs_point_fields_on_merged_piece"_test = [&] () {
                expect(GridFormat::Ranges::size(point_fields(reader)) > 0);
                for (const auto& [name, field_ptr] : point_fields(reader)) {
                    expect(eq(field_ptr->layout().extent(0), num_points_per_rank*2));
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, proc_grid, GridFormat::points(proc_grid)
                    ));
                }
            };

            "parallel_pvtu_less_procs_cell_fields_on_merged_piece"_test = [&] () {
                expect(GridFormat::Ranges::size(cell_fields(reader)) > 0);
                for (const auto& [name, field_ptr] : cell_fields(reader)) {
                    expect(eq(field_ptr->layout().extent(0), num_cells_per_rank*2));
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, proc_grid, GridFormat::cells(proc_grid)
                    ));
                }
            };
        }
    }

    MPI_Finalize();
    return 0;
}
