// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <array>
#include <ranges>
#include <numbers>
#include <iostream>
#include <stdexcept>
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

    const std::array<std::size_t, 3> dimensions() const {
        return _dimensions;
    }

    void set_value_at(const Voxel& v, const int value) {
        _data.at(_index(v)) = value;
    }

    int get_value_at(const Voxel& v) const {
        return _data.at(_index(v));
    }

    auto center_of(const Voxel& v) const {
        return std::array{
            static_cast<double>(v.x) + 0.5,
            static_cast<double>(v.y) + 0.5,
            static_cast<double>(v.z) + 0.5
        };
    }

 private:
    std::size_t _index(const Voxel& v) const {
        if (v.x >= _dimensions[0] || v.y >= _dimensions[1] || v.z >= _dimensions[2])
            throw std::runtime_error("Given voxel is out of bounds");
        return v.z*_dimensions[0]*_dimensions[1]
            + v.y*_dimensions[0]
            + v.x;
    }

    std::array<std::size_t, 3> _dimensions;
    std::vector<int> _data;
};


// Specialization of the traits required to fulfill the `ImageGrid`
// concept in GridFormat for our VoxelData class.
namespace GridFormat::Traits {

template<>
struct Cells<VoxelData> {
    static std::ranges::range auto get(const VoxelData& voxel_data) {
        // `GridFormat` comes with the `MDIndexRange` that one can use to iterate
        // over all multi-dimensional indices within given dimensions (MDLayout).
        // Let's use such an index range and transform it with `std::ranges` to
        // yield a range of `VoxelData::Voxel` (which will be deduced as cell type).
        using GridFormat::MDLayout;
        using GridFormat::MDIndex;
        using GridFormat::MDIndexRange;
        return MDIndexRange{MDLayout{voxel_data.dimensions()}} | std::views::transform([] (const MDIndex& i) {
            return VoxelData::Voxel{i.get(0), i.get(1), i.get(2)};
        });
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
        return voxel_data.dimensions();
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
    static_assert(GridFormat::Concepts::ImageGrid<VoxelData>);

    VoxelData voxel_data{{100, 80, 120}};

    // let us set some indicator function as voxel data
    for (std::size_t z = 0; z < voxel_data.dimensions()[2]; ++z)
        for (std::size_t y = 0; y < voxel_data.dimensions()[1]; ++y)
            for (std::size_t x = 0; x < voxel_data.dimensions()[0]; ++x) {
                const VoxelData::Voxel voxel{x, y, z};
                const auto center = voxel_data.center_of(voxel);
                const auto frequency_x = 2.0*std::numbers::pi/voxel_data.dimensions()[0];
                const auto frequency_y = 2.0*std::numbers::pi/voxel_data.dimensions()[1];
                const auto frequency_z = 4.0*std::numbers::pi/voxel_data.dimensions()[2];
                const bool value = std::sin(frequency_x*center[0])
                    + std::cos(frequency_y*center[1])
                    + std::sin(frequency_z*center[2] + 0.5*std::numbers::pi) > 0.25;
                voxel_data.set_value_at(voxel, value);
            }

    // we will write a bunch of files. This is a convenience function
    // to add a cell field to a writer and write the file.
    const auto add_data_and_write = [&] (auto& writer, const std::string& filename) {
        // Most file formats allow to attach metadata. You can attach metadata via
        // the `set_meta_data` function, which takes a name and the data to be written.
        // This can be arrays of any sort, including strings.
        writer.set_meta_data("SomeMetadata", "I am metadata");
        writer.set_cell_field("indicator", [&] (const auto& voxel) {
            return voxel_data.get_value_at(voxel);
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
