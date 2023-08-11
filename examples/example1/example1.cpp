// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <array>
#include <ranges>
#include <numbers>
#include <iostream>
#include <cmath>

#include <gridformat/gridformat.hpp>

// Data structure to store data on voxels. This implementation ignores
// physical space dimensions, that is, it has no information on how big
// (in physical space) a voxel is. In this example, we want to register
// this data structure as `ImageGrid` to GridFormat, such that we can
// export the data into suitable file formats.
class VoxelData {
 public:
    struct Voxel {
        std::size_t x;
        std::size_t y;
        std::size_t z;
    };

    VoxelData(std::array<std::size_t, 3> dimensions)
    : _dimensions{std::move(dimensions)}
    , _data(_dimensions[0]*_dimensions[1]*_dimensions[2], 0.0)
    {}

    friend auto voxels(const VoxelData& vd) {
        // `GridFormat` comes with the `MDIndexRange` that one can use to iterate
        // over all multi-dimensional indices within given dimensions (MDLayout):
        using GridFormat::MDIndex;
        using GridFormat::MDLayout;
        return GridFormat::MDIndexRange{MDLayout{vd._dimensions}} | std::views::transform([] (const MDIndex& i) {
            return Voxel{i.get(0), i.get(1), i.get(2)};
        });
    }

    std::size_t size(int direction) const { return _dimensions.at(direction); }
    std::size_t size() const { return _data.size(); }

    void set(const double value, const Voxel& v) { _data.at(_index(v)) = value; }
    double get(const Voxel& v) const { return _data.at(_index(v)); }

    auto center(const Voxel& v) const {
        return std::array{
            static_cast<double>(v.x) + 0.5,
            static_cast<double>(v.y) + 0.5,
            static_cast<double>(v.z) + 0.5
        };
    }

 private:
    std::size_t _index(const Voxel& v) const {
        return v.z*_dimensions[0]*_dimensions[1]
            + v.y*_dimensions[0]
            + v.x;
    }

    std::array<std::size_t, 3> _dimensions;
    std::vector<double> _data;
};


// Specialization of the traits required to fulfill the `ImageGrid`
// concept in GridFormat for our VoxelData class.
namespace GridFormat::Traits {

template<>
struct Cells<VoxelData> {
    static std::ranges::range auto get(const VoxelData& voxel_data) {
        // Let's use voxels as our "grid cells"
        return voxels(voxel_data);
    }
};

template<>
struct Points<VoxelData> {
    static std::ranges::empty_view<int> get(const VoxelData& voxel_data) {
        // For image grids, a point range is only needed if we want to write out
        // field data defined on points. Our VoxelData does not really carry a notion
        // of points, so let us just throw an exception if someone tries to call this.
        // Note however, that we use an empty range of integers as return type, so that
        // GridFormat can deduce a "point type" for our grid (i.e. in this case `int`)
        throw std::runtime_error("VoxelData does not implement points");
    }
};

template<>
struct Extents<VoxelData> {
    static auto get(const VoxelData& voxel_data) {
        return std::array{voxel_data.size(0), voxel_data.size(1), voxel_data.size(2)};
    }
};

template<>
struct Origin<VoxelData> {
    static auto get(const VoxelData&) {
        // Our VoxelData class defines voxels only in terms of their indices and
        // does not carry a notion of physical space. Our indices start at (0, 0, 0).
        return std::array{0.0, 0.0, 0.0};
    }
};

template<>
struct Spacing<VoxelData> {
    static auto get(const VoxelData&) {
        // Since VoxelData defines voxels in terms of indices, we simply have a unit spacing
        return std::array{1.0, 1.0, 1.0};
    }
};

template<>
struct Location<VoxelData, typename VoxelData::Voxel> {
    static auto get(const VoxelData& voxel_data, const typename VoxelData::Voxel& voxel) {
        return std::array{voxel.x, voxel.y, voxel.z};
    }
};

// Note that we must provide a specialization of the Location trait for the "points" of VoxelData,
// which we defined in the Points trait to be of type int. Again, we simply throw if this is tried
// to be called. Note that we must specify a return type that fulfills the requirements of Location,
// that is, it must be a statically sized range with size equal to the grid dimensions.
template<>
struct Location<VoxelData, int> {
    static std::array<std::size_t, 3> get(const VoxelData&, int) {
        throw std::runtime_error("VoxelData does not implement Location for points");
    }
};

}  // namespace GridFormat::Traits


int main() {
    // Let us check against the concept to see if we implemented the traits correctly.
    // When implementing the grid traits for a data structure, it is helpful to make
    // such static_asserts pass before actually using the GridFormat API.
    static_assert(GridFormat::Concepts::ImageGrid<VoxelData>);

    VoxelData voxel_data{{100, 80, 120}};

    // A function that we use to define some cell data output
    const auto indicator_function = [&] (const auto& voxel) -> int {
        const auto coords = voxel_data.center(voxel);
        const auto frequency_x = 2.0*std::numbers::pi/voxel_data.size(0);
        const auto frequency_y = 2.0*std::numbers::pi/voxel_data.size(1);
        const auto frequency_z = 4.0*std::numbers::pi/voxel_data.size(2);
        return std::sin(frequency_x*coords[0])
            + std::cos(frequency_y*coords[1])
            + std::sin(frequency_z*coords[2] + 0.5*std::numbers::pi) > 0.25;
    };

    // we will write a bunch of files. This is a convenience function
    // to add a cell field to a writer and write the file.
    const auto add_data_and_write = [&] (auto& writer, const std::string& filename) {
        // Most file formats allow to attach metadata. You can attach metadata via
        // the `set_meta_data` function, which takes a name and the data to be written.
        // This can be arrays of any sort, including strings.
        writer.set_meta_data("SomeMetadata", "I am metadata");
        writer.set_cell_field("indicator", [&] (const auto& voxel) {
            return indicator_function(voxel);
        });
        const auto written_filename = writer.write(filename);
        std::cout << "Wrote '" << written_filename << "'" << std::endl;
        return written_filename;
    };

    // we also illustrate how data can be read back in with this convenience function.
    const auto echo_meta_data = [&] (auto&& reader, const std::string& filename) {
        reader.open(filename);
        for (const auto& [name, field_ptr] : meta_data_fields(reader))
            std::cout << "Echoing the meta data '" << name << "': \""
                      << field_ptr->template export_to<std::string>() << "\""
                      << std::endl;
    };

    // First, let Gridformat select a suitable default file format for us
    // and use a generic reader that can read any of the supported file formats.
    {
        GridFormat::Writer writer{
            GridFormat::default_for(voxel_data),
            voxel_data
        };
        const auto filename = add_data_and_write(writer, "voxel_data_default_format");
        echo_meta_data(GridFormat::Reader{}, filename);
    }

    // Let's explicitly ask for the .vti image grid format. The reader we construct here
    // is specifically for .vti files and would fail to open other file formats.
    {
        GridFormat::Writer writer{
            GridFormat::vti,
            voxel_data
        };
        const auto filename = add_data_and_write(writer, "voxel_data_explicit_format");
        echo_meta_data(GridFormat::Reader{GridFormat::vti}, filename);
    }

    // Let's explicitly ask for .vti format with raw encoding. Note that on the reader side
    // we don't need to define any format options. It will read whatever it finds in the .vti file.
    {
        GridFormat::Writer writer{
            GridFormat::vti({.encoder = GridFormat::Encoding::raw}),
            voxel_data
        };
        const auto filename = add_data_and_write(writer, "voxel_data_explicit_encoding");
        echo_meta_data(GridFormat::Reader{GridFormat::vti}, filename);
    }

    // Let's explicitly ask for .vti format without compression
    {
        GridFormat::Writer writer{
            GridFormat::vti({.compressor = GridFormat::none}),
            voxel_data
        };
        const auto filename = add_data_and_write(writer, "voxel_data_no_compression");
        echo_meta_data(GridFormat::Reader{GridFormat::vti}, filename);
    }

    return 0;
}
