// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <vector>
#include <cstddef>
#include <filesystem>

#include <gridformat/common/hdf5.hpp>
#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../testing.hpp"

using GridFormat::Testing::operator""_test;
using GridFormat::Testing::expect;
using GridFormat::Testing::throws;
using GridFormat::Testing::eq;

namespace {

constexpr auto literal_text = "some_literal_text";
const std::string string_text{"some_string_text"};

template<typename Writer>
void set_meta_data(Writer& writer) {
    writer.set_meta_data("literal", literal_text);
    writer.set_meta_data("string", string_text);
    writer.set_meta_data("numbers", std::vector<int>{1, 2, 3, 4});
}

HighFive::DataSet open_field_data(HighFive::File& file, const std::string& name) {
    return file.getDataSet("/VTKHDF/FieldData/" + name);
}

//! Check that the given dataset is a (1,)-shaped dataset of variable-length strings
void expect_string_dataset(HighFive::File& file, const std::string& name, const std::string& expected) {
    auto dataset = open_field_data(file, name);
    const auto type = dataset.getDataType();
    expect(type.getClass() == HighFive::DataTypeClass::String);
    expect(type.isVariableStr());

    const auto dims = dataset.getDimensions();
    expect(eq(dims.size(), std::size_t{1}));
    expect(eq(dims.at(0), std::size_t{1}));

    std::vector<std::string> values;
    dataset.read(values);
    expect(eq(values.size(), std::size_t{1}));
    expect(eq(values.at(0), expected));
}

//! Numeric meta data must be unaffected by the string format
void expect_unchanged_numbers(HighFive::File& file) {
    auto dataset = open_field_data(file, "numbers");
    expect(dataset.getDataType().getClass() == HighFive::DataTypeClass::Integer);
    expect(eq(dataset.getDataType().getSize(), std::size_t{4}));
    expect(eq(dataset.getDimensions().size(), std::size_t{1}));
    expect(eq(dataset.getDimensions().at(0), std::size_t{4}));
    expect(std::ranges::equal(dataset.read<std::vector<int>>(), std::vector<int>{1, 2, 3, 4}));
}

}  // namespace

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();

    "vtk_hdf_string_meta_data_as_string_array"_test = [&] () {
        GridFormat::VTKHDFWriter writer{grid};
        set_meta_data(writer);
        const auto filename = writer.write("vtk_hdf_string_meta_data_string_array");

        HighFive::File file{filename, HighFive::File::ReadOnly};
        expect_string_dataset(file, "literal", literal_text);
        expect_string_dataset(file, "string", string_text);
        expect_unchanged_numbers(file);
    };

    "vtk_hdf_string_meta_data_read_back"_test = [&] () {
        GridFormat::VTKHDFReader reader;
        reader.open("vtk_hdf_string_meta_data_string_array.hdf");
        expect(eq(reader.meta_data_field("literal")->template export_to<std::string>(), std::string{literal_text}));
        expect(eq(reader.meta_data_field("string")->template export_to<std::string>(), string_text));
        expect(std::ranges::equal(
            reader.meta_data_field("numbers")->template export_to<std::vector<int>>(),
            std::vector<int>{1, 2, 3, 4}
        ));
    };

    // the value has to differ per step, otherwise reading the wrong entry goes unnoticed
    const auto text_at = [] (std::size_t step) { return "step_" + std::to_string(step); };
    const auto write_transient = [&] (const std::string& base, bool static_meta_data) {
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid, base, GridFormat::VTK::HDFTransientOptions{
                .static_grid = true,
                .static_meta_data = static_meta_data
            }
        };
        std::size_t step = 0;
        for (double t : {0.0, 0.5, 1.0}) {
            writer.set_meta_data("string", text_at(step++));
            writer.write(t);
        }
    };

    "vtk_hdf_transient_string_meta_data"_test = [&] () {
        const std::string base = "vtk_hdf_string_meta_data_transient";
        write_transient(base, false);

        // one variable-length string per step, selected via Steps/FieldDataOffsets
        HighFive::File file{base + ".hdf", HighFive::File::ReadOnly};
        auto dataset = open_field_data(file, "string");
        expect(dataset.getDataType().isVariableStr());
        expect(eq(dataset.getDimensions().size(), std::size_t{1}));
        expect(eq(dataset.getDimensions().at(0), std::size_t{3}));
        expect(std::ranges::equal(
            dataset.read<std::vector<std::string>>(),
            std::vector<std::string>{text_at(0), text_at(1), text_at(2)}
        ));
        expect(std::ranges::equal(
            file.getDataSet("/VTKHDF/Steps/FieldDataOffsets/string").read<std::vector<std::size_t>>(),
            std::vector<std::size_t>{0, 1, 2}
        ));

        GridFormat::VTKHDFReader reader;
        reader.open(base + ".hdf");
        expect(eq(reader.number_of_steps(), std::size_t{3}));
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            reader.set_step(step);
            expect(eq(reader.meta_data_field("string")->template export_to<std::string>(), text_at(step)));
        }
    };

    "vtk_hdf_static_transient_string_meta_data"_test = [&] () {
        const std::string base = "vtk_hdf_string_meta_data_transient_static";
        write_transient(base, true);

        // written once, with all steps pointing at the first entry
        HighFive::File file{base + ".hdf", HighFive::File::ReadOnly};
        auto dataset = open_field_data(file, "string");
        expect(dataset.getDataType().isVariableStr());
        expect(eq(dataset.getDimensions().at(0), std::size_t{1}));
        expect(std::ranges::equal(
            file.getDataSet("/VTKHDF/Steps/FieldDataOffsets/string").read<std::vector<std::size_t>>(),
            std::vector<std::size_t>{0, 0, 0}
        ));

        GridFormat::VTKHDFReader reader;
        reader.open(base + ".hdf");
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            reader.set_step(step);
            expect(eq(reader.meta_data_field("string")->template export_to<std::string>(), text_at(0)));
        }
    };

    "vtk_hdf_multi_string_field_data_is_rejected"_test = [&] () {
        // VTK writes a vtkStringArray with N values as a dataset with N entries. We cannot
        // represent that as a field, so reading it must fail instead of silently truncating.
        const std::string filename = "vtk_hdf_string_meta_data_multi.hdf";
        std::filesystem::copy_file(
            "vtk_hdf_string_meta_data_string_array.hdf",
            filename,
            std::filesystem::copy_options::overwrite_existing
        );
        {
            HighFive::File file{filename, HighFive::File::ReadWrite};
            const std::vector<std::string> names{"alpha", "beta", "gamma"};
            file.getGroup("/VTKHDF/FieldData")
                .createDataSet("names", HighFive::DataSpace{names.size()}, GridFormat::HDF5::VariableLengthString{})
                .write(names);
        }

        GridFormat::VTKHDFReader reader;
        reader.open(filename);
        expect(throws([&] () { reader.meta_data_field("names"); }));
        // the single-string fields in the same file remain readable
        expect(eq(reader.meta_data_field("string")->template export_to<std::string>(), string_text));
    };

    "hdf5_append_rejects_mismatching_type_class"_test = [&] () {
        using File = GridFormat::HDF5::File<>;
        const std::string filename = "vtk_hdf_string_meta_data_type_mismatch.hdf";
        File::clear(filename, GridFormat::NullCommunicator{});
        {   // create both in append mode, such that they are extendible
            File file{filename, File::append};
            file.write(std::vector<int>{1, 2, 3}, "/numeric");
            file.write_strings({"a"}, "/strings");
        }
        {
            File file{filename, File::append};
            expect(throws([&] () { file.write_strings({"b"}, "/numeric"); }));
            expect(throws([&] () { file.write(std::vector<int>{4}, "/strings"); }));
            // appending numbers of another type stays allowed, hdf5 converts them
            file.write(std::vector<double>{4.0}, "/numeric");
            file.write_strings({"b"}, "/strings");
        }
        {   // the rejected writes must have left the datasets untouched
            File file{filename, File::read_only};
            expect(eq(file.get_dimensions("/numeric").value().at(0), std::size_t{4}));
            expect(eq(file.get_dimensions("/strings").value().at(0), std::size_t{2}));
            expect(std::ranges::equal(
                file.read_dataset_to<std::vector<int>>("/numeric"), std::vector<int>{1, 2, 3, 4}
            ));
        }
    };

    "hdf5_reports_string_support"_test = [&] () {
        // without a parallel communicator, strings can always be written. The parallel case cannot
        // be covered here, see test_vtk_hdf_parallel_unstructured_grid_writer.
        GridFormat::HDF5::File file{"vtk_hdf_string_meta_data_support.hdf", GridFormat::HDF5::File<>::overwrite};
        expect(file.supports_variable_length_strings());
    };

    "vtk_hdf_invalid_array_names"_test = [&] () {
        const auto write_with_meta_data_name = [&] (const std::string& name) {
            GridFormat::VTKHDFWriter writer{grid};
            writer.set_meta_data(name, string_text);
            writer.write("vtk_hdf_string_meta_data_invalid_name");
        };
        expect(throws([&] () { write_with_meta_data_name("with/slash"); }));
        expect(throws([&] () { write_with_meta_data_name("with.dot"); }));

        const auto write_with_cell_field_name = [&] (const std::string& name) {
            GridFormat::VTKHDFWriter writer{grid};
            writer.set_cell_field(name, [] (const auto&) { return 1.0; });
            writer.write("vtk_hdf_string_meta_data_invalid_name");
        };
        expect(throws([&] () { write_with_cell_field_name("with/slash"); }));
        expect(throws([&] () { write_with_cell_field_name("with.dot"); }));
    };

    return 0;
}
