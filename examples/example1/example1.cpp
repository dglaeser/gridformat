// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <array>
#include <ranges>
#include <numbers>
#include <cmath>

#include <gridformat/gridformat.hpp>

// Data structure to store data on voxels. In this example, we want
// to register this data structure as `ImageGrid` to GridFormat, such
// that we can export the data into suitable file formats.
class VoxelData {
 public:
    struct Voxel { std::size_t index; };

    VoxelData(std::array<std::size_t, 3> dimensions)
    : _dimensions{std::move(dimensions)}
    , _data(_dimensions[0]*_dimensions[1]*_dimensions[2], 0.0)
    {}

    friend auto voxels(const VoxelData& vd) {
        return std::views::iota(std::size_t{0}, vd.size())
            | std::views::transform([] (std::size_t i) { return Voxel{i}; });
    }

    std::size_t size(int direction) const { return _dimensions.at(direction); }
    std::size_t size() const { return _data.size(); }

    void set(const double value, const Voxel& v) { _data.at(v.index) = value; }
    double get(const Voxel& v) const { return _data.at(v.index); }

    auto center(const Voxel& v) const {
        const std::size_t res = v.index%(_dimensions[0]*_dimensions[1]);
        const std::size_t iz = v.index/(_dimensions[0]*_dimensions[1]);
        const std::size_t iy = res/_dimensions[0];
        const std::size_t ix = res%_dimensions[0];
        // add 0.5 to get the center of the voxel
        return std::array{
            static_cast<double>(ix) + 0.5,
            static_cast<double>(iy) + 0.5,
            static_cast<double>(iz) + 0.5
        };
    }

 private:
    std::size_t _index(const std::array<std::size_t, 3>& voxel) const {
        return voxel[2]*(_dimensions[0]*_dimensions[1]) + voxel[1]*_dimensions[0] + voxel[0];
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
        // For image grids, this point range is only needed if we want to write out
        // field data defined on point. Our VoxelData does not really carry a notion
        // of points, so let us just throw an exception if someone tries to call this.
        // Note that we could easily implement this, for instance, by simply returning a
        // sequence of indices that represent our grid "points" (see comment in the
        // Location trait for information on what we would have to do there):
        //
        // const std::size_t number_of_points = (voxel_data.size(0) + 1)
        //                                     *(voxel_data.size(1) + 1)
        //                                     *(voxel_data.size(2) + 1);
        // return std::views::iota(std::size_t{0}, number_of_points);
        //
        // Instead of the above, we just throw an exception. Note however, that we use
        // an empty range of integers as return type, so that GridFormat can deduce a
        // "point type" for our grid (thus, in this case int)
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
        const auto coords = voxel_data.center(voxel);
        return std::array{
            static_cast<std::size_t>(coords[0]),
            static_cast<std::size_t>(coords[1]),
            static_cast<std::size_t>(coords[2])
        };
    }
};

// Note that we must provide a specialization of the Location trait for the "points" of VoxelData,
// which we defined in the Points trait to be of type int. Again, we simply throw if this is tried
// to be called. Note that we must specify a return type that fulfills the requirements of Location,
// that is, it must be a statically sized range with size equal to the grid dimensions.
//
// If we had return a range of indices for the points as written in the comments of the `Points` trait,
// we would have to identify the location of a point, given its index (similar to the code in voxel_data.center()).
template<>
struct Location<VoxelData, int> {
    static std::array<std::size_t, 3> get(const VoxelData&, int) {
        throw std::runtime_error("VoxelData does not implement points");
    }
};

}  // namespace GridFormat::Traits


int main() {
    // Let us check against the concept to see if we implemented the traits correctly
    static_assert(GridFormat::Concepts::ImageGrid<VoxelData>);

    VoxelData voxel_data{{100, 80, 120}};

    // Function that we use to define some cell data output
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
        writer.set_cell_field("indicator", [&] (const auto& voxel) {
            return indicator_function(voxel);
        });
        const auto written_filename = writer.write(filename);
        std::cout << "Wrote '" << written_filename << "'" << std::endl;
    };

    // First, let gridformat select a suitable default file format for us
    {
        GridFormat::Writer writer{
            GridFormat::default_for(voxel_data),
            voxel_data
        };
        add_data_and_write(writer, "voxel_data_default_format");
    }

    // Let's explicit ask for the .vti image grid format
    {
        GridFormat::Writer writer{
            GridFormat::vti,
            voxel_data
        };
        add_data_and_write(writer, "voxel_data_explicit_format");
    }

    // Let's explicitly ask for .vti format with raw encoding
    {
        GridFormat::Writer writer{
            GridFormat::vti({.encoder = GridFormat::Encoding::raw}),
            voxel_data
        };
        add_data_and_write(writer, "voxel_data_explicit_encoding");
    }

    // Let's explicitly ask for .vti format without compression
    {
        GridFormat::Writer writer{
            GridFormat::vti({.compressor = GridFormat::none}),
            voxel_data
        };
        add_data_and_write(writer, "voxel_data_no_compression");
    }

    return 0;
}
