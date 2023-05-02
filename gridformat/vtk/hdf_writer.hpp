// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTKHDFWriter
 */
#ifndef GRIDFORMAT_VTK_HDF_WRITER_HPP_
#define GRIDFORMAT_VTK_HDF_WRITER_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <concepts>
#include <fstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <highfive/H5Easy.hpp>
#include <highfive/H5File.hpp>
#pragma GCC diagnostic pop

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/writer.hpp>

#include <gridformat/vtk/common.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace VTKHDFDetail {

// Custom string data type using ascii encoding: VTKHDF uses ascii, but HighFive uses UTF-8.
struct AsciiString : public HighFive::DataType {
    explicit AsciiString(std::size_t n) {
        _hid = H5Tcopy(H5T_C_S1);
        if (H5Tset_size(_hid, n) < 0) {
            HighFive::HDF5ErrMapper::ToException<HighFive::DataTypeException>(
                "Unable to define datatype size to " + std::to_string(n)
            );
        }
        // define encoding to ASCII
        H5Tset_cset(_hid, H5T_CSET_ASCII);
        H5Tset_strpad(_hid, H5T_STR_SPACEPAD);
    }

    template<std::size_t N>
    static AsciiString from(const char (&input)[N]) {
        if (input[N-1] == '\0')
            return AsciiString{N-1};
        return AsciiString{N};
    }

    static AsciiString from(const std::string& n) {
        return AsciiString{n.size()};
    }
};

}  // namespace VTKHDFDetail
#endif  // DOXYGEN

template<typename T, int dim>
class StructuredDataArray;

template<typename T>
class StructuredDataArray<T, 2> {
 public:
    template<Concepts::StaticallySizedRange Extents>
        requires (static_size<Extents> == 2 and Concepts::IndexableRange<Extents>)
    explicit StructuredDataArray(const Extents& extents) {
        resize(extents);
    }

    template<Concepts::StaticallySizedRange R = std::array<std::size_t, 2>>
        requires(static_size<R> == 2 and Concepts::IndexableRange<R>)
    T& operator[](const R& index_tuple) {
        return _data[index_tuple[1]][index_tuple[0]];
    }

    template<Concepts::StaticallySizedRange R = std::array<std::size_t, 2>>
        requires(static_size<R> == 2 and Concepts::IndexableRange<R>)
    const T& operator[](const R& index_tuple) const {
        return _data[index_tuple[1]][index_tuple[0]];
    }

    template<Concepts::StaticallySizedRange R = std::array<std::size_t, 2>>
        requires(static_size<R> == 2 and Concepts::IndexableRange<R>)
    void resize(const R& extents) {
        _data.resize(extents[1]);
        std::ranges::for_each(_data, [&] (auto& sub_range) { sub_range.resize(extents[0]); });
    }

    auto& data() { return _data; }
    const auto& data() const { return _data; }

 private:
    std::vector<std::vector<T>> _data;
};

template<typename T>
class StructuredDataArray<T, 3> {
 public:
    template<Concepts::StaticallySizedRange Extents>
        requires (static_size<Extents> == 3 and Concepts::IndexableRange<Extents>)
    explicit StructuredDataArray(const Extents& extents) {
        resize(extents);
    }

    template<Concepts::StaticallySizedRange R = std::array<std::size_t, 2>>
        requires(static_size<R> == 3 and Concepts::IndexableRange<R>)
    T& operator[](const R& index_tuple) {
        return _data[index_tuple[2]][index_tuple[1]][index_tuple[0]];
    }

    template<Concepts::StaticallySizedRange R = std::array<std::size_t, 2>>
        requires(static_size<R> == 3 and Concepts::IndexableRange<R>)
    const T& operator[](const R& index_tuple) const {
        return _data[index_tuple[2]][index_tuple[1]][index_tuple[0]];
    }

    template<Concepts::StaticallySizedRange R = std::array<std::size_t, 2>>
        requires(static_size<R> == 3 and Concepts::IndexableRange<R>)
    void resize(const R& extents) {
        _data.resize(extents[2]);
        std::ranges::for_each(_data, [&] (auto& sub_range) {
            sub_range.resize(extents[1]);
            std::ranges::for_each(sub_range, [&] (auto& sub_sub_range) { sub_sub_range.resize(extents[0]); });
        });
    }

    auto& data() { return _data; }
    const auto& data() const { return _data; }

 private:
    std::vector<std::vector<std::vector<T>>> _data;
};

/*!
 * \ingroup VTK
 * \todo TODO: Doc ma
 */
template<Concepts::Grid Grid>
class VTKHDFWriter;

template<Concepts::Grid Grid>
VTKHDFWriter(const Grid&) -> VTKHDFWriter<Grid>;

// Specialization for image grids
template<Concepts::UnstructuredGrid Grid> requires(not Concepts::ImageGrid<Grid>)
class VTKHDFWriter<Grid> : public GridWriter<Grid> {
    using CT = CoordinateType<Grid>;

 public:
    explicit VTKHDFWriter(const Grid& grid)
    : GridWriter<Grid>(grid, ".hdf")
    {}

 private:
    virtual void _write(const std::string& filename_with_ext) const {
        HighFive::File file{filename_with_ext, HighFive::File::Overwrite};
        auto vtk_group = file.createGroup("VTKHDF");
        vtk_group.createAttribute("Version", std::array<std::size_t, 2>{1, 0});

        auto type_attr = vtk_group.createAttribute(
            "Type",
            HighFive::DataSpace{1},
            VTKHDFDetail::AsciiString::from("UnstructuredGrid")
        );
        type_attr.write("UnstructuredGrid");

        vtk_group.createDataSet("NumberOfPoints", std::vector{number_of_points(this->grid())});
        vtk_group.createDataSet("NumberOfCells", std::vector{number_of_cells(this->grid())});

        const auto point_id_map = make_point_id_map(this->grid());
        const auto connectivity_field = VTK::make_connectivity_field(this->grid(), point_id_map);
        std::vector<long> connectivity(connectivity_field->layout().number_of_entries());
        connectivity_field->export_to(connectivity);
        vtk_group.createDataSet("NumberOfConnectivityIds", std::vector<long>{static_cast<long>(connectivity.size())});
        vtk_group.createDataSet("Connectivity", connectivity);

        const auto offsets_field = VTK::make_offsets_field(this->grid());
        std::vector<long> offsets(offsets_field->layout().number_of_entries());
        offsets_field->export_to(offsets);
        offsets.insert(offsets.begin(), 0);
        vtk_group.createDataSet("Offsets", offsets);

        const auto types_field = VTK::make_cell_types_field(this->grid());
        std::vector<std::uint8_t> types(types_field->layout().number_of_entries());
        types_field->export_to(types);
        vtk_group.createDataSet("Types", types);

        const auto coords_field = VTK::make_coordinates_field<CT>(this->grid());
        std::vector<std::array<CT, 3>> coords(number_of_points(this->grid()));
        coords_field->export_to(coords);
        vtk_group.createDataSet("Points", coords);

        auto pd_group = vtk_group.createGroup("PointData");
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            auto field_ptr = VTK::make_vtk_field(this->_get_shared_point_field(name));
            field_ptr->precision().visit([&] <typename T> (const Precision<T>&) {
                _write_field<T>(pd_group, name, *field_ptr);
            });
        });

        auto cd_group = vtk_group.createGroup("CellData");
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            auto field_ptr = VTK::make_vtk_field(this->_get_shared_cell_field(name));
            field_ptr->precision().visit([&] <typename T> (const Precision<T>&) {
                _write_field<T>(cd_group, name, *field_ptr);
            });
        });
    }

    virtual void _write(std::ostream&) const {
        throw InvalidState("VTKHDFWriter does not support export into stream");
    }

    template<typename T, typename Group>
    void _write_field(Group& group, const std::string& name, const Field& field) const {
        const auto layout = field.layout();
        if (layout.is_scalar()) {
            T data;
            field.export_to(data);
            group.createDataSet(name, data);
        } else if (layout.dimension() == 1) {
            std::vector<T> data(layout.extent(0));
            field.export_to(data);
            group.createDataSet(name, data);
        } else if (layout.dimension() == 2) {
            std::vector<std::vector<T>> data(layout.extent(0));
            std::ranges::for_each(data, [&] (auto& entry) { entry.resize(layout.extent(1)); });
            field.export_to(data);
            group.createDataSet(name, data);
        } else if (layout.dimension() == 3) {
            // std::vector<std::vector<std::vector<T>>> data(layout.extent(0));
            // std::ranges::for_each(data, [&] (auto& entry) {
            //     entry.resize(layout.extent(1));
            //     std::ranges::for_each(entry, [&] (auto& sub_entry) { sub_entry.resize(layout.extent(2)); });
            // });
            // field.export_to(data);
            // group.createDataSet(name, data);
        } else {
            throw NotImplemented("Support for fields with dimension > 3 or < 1");
        }
    }
};

// Specialization for image grids
template<Concepts::ImageGrid Grid>
class VTKHDFWriter<Grid> : public GridWriter<Grid> {
    using CT = CoordinateType<Grid>;
    static constexpr std::size_t dim = dimension<Grid>;

 public:
    explicit VTKHDFWriter(const Grid& grid)
    : GridWriter<Grid>(grid, ".hdf")
    {}

 private:
    virtual void _write(const std::string& filename_with_ext) const {
        std::array<CT, 3> my_origin{0., 0., 0.};
        std::array<CT, 3> my_spacing{0., 0., 0.};
        std::ranges::copy(origin(this->grid()), my_origin.begin());
        std::ranges::copy(spacing(this->grid()), my_spacing.begin());

        std::array<std::array<CT, 3>, 3> my_direction;
        {
            int i = 0;
            std::ranges::for_each(direction(this->grid()), [&] (const auto& dir) {
                std::ranges::fill(my_direction[i], 0.0);
                std::ranges::copy(dir, my_direction[i].begin());
                ++i;
            });
            for (; i < 3; ++i) {
                std::array<CT, 3> dir;
                std::ranges::fill(dir, 0.0);
                dir[i] = 1.0;
                my_direction[i] = dir;
            }
        }

        HighFive::File file{filename_with_ext, HighFive::File::Overwrite};
        auto vtk_group = file.createGroup("VTKHDF");
        vtk_group.createAttribute("Version", std::array<std::size_t, 2>{1, 0});
        vtk_group.createAttribute("Origin", my_origin);
        vtk_group.createAttribute("Spacing", my_spacing);
        vtk_group.createAttribute("WholeExtent", VTK::CommonDetail::get_extents(extents(this->grid())));
        vtk_group.createAttribute("Direction", Ranges::as_flat_vector(my_direction));

        using VTKHDFDetail::AsciiString;
        auto type_attr = vtk_group.createAttribute("Type", HighFive::DataSpace{1}, AsciiString::from("ImageData"));
        type_attr.write("ImageData");

        auto pd_group = vtk_group.createGroup("PointData");
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            const auto& field = this->_get_point_field(name);
            field.precision().visit([&] <typename T> (const Precision<T>&) {
                _write_field<true, T>(pd_group, name, field);
            });
        });

        auto cd_group = vtk_group.createGroup("CellData");
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            const auto& field = this->_get_cell_field(name);
            field.precision().visit([&] <typename T> (const Precision<T>&) {
                _write_field<false, T>(cd_group, name, field);
            });
        });
    }

    virtual void _write(std::ostream&) const {
        throw InvalidState("VTKHDFWriter does not support export into stream");
    }

    template<bool is_point_data, typename T, typename Group>
    void _write_field(Group& group, const std::string& name, const Field& field) const {
        const auto layout = field.layout();
        if (layout.is_scalar()) {
            T data;
            field.export_to(data);
            group.createDataSet(name, data);
        } else if (layout.dimension() == 1) {
            std::vector<T> data(layout.extent(0));
            field.export_to(data);
            if constexpr (is_point_data)
                group.createDataSet(name, _as_point_data(data).data());
            else
                group.createDataSet(name, _as_cell_data(data).data());
        } else if (layout.dimension() == 2) {
            // std::vector<std::vector<T>> data(layout.extent(0));
            // std::ranges::for_each(data, [&] (auto& entry) { entry.resize(layout.extent(1)); });
            // field.export_to(data);
            // if constexpr (is_point_data)
            //     group.createDataSet(name, _as_point_data(data).data());
            // else
            //     group.createDataSet(name, _as_cell_data(data).data());
        } else if (layout.dimension() == 3) {
            // std::vector<std::vector<std::vector<T>>> data(layout.extent(0));
            // std::ranges::for_each(data, [&] (auto& entry) {
            //     entry.resize(layout.extent(1));
            //     std::ranges::for_each(entry, [&] (auto& sub_entry) { sub_entry.resize(layout.extent(2)); });
            // });
            // field.export_to(data);
            // if constexpr (is_point_data)
            //     group.createDataSet(name, _as_point_data(data).data());
            // else
            //     group.createDataSet(name, _as_cell_data(data).data());
        } else {
            throw NotImplemented("Support for fields with dimension > 3 or < 1");
        }
    }

    template<typename T>
    auto _as_point_data(const std::vector<T>& data) const {
        auto npoints = Ranges::to_array<dim>(extents(this->grid()));
        std::ranges::for_each(npoints, [] (auto& n) { n += 1; });
        return _as_field_data(data, npoints, points(this->grid()));
    }

    template<typename T>
    auto _as_cell_data(const std::vector<T>& data) const {
        return _as_field_data(data, extents(this->grid()), cells(this->grid()));
    }

    template<typename T, typename Extents, typename EntityRange>
    auto _as_field_data(const std::vector<T>& data,
                        const Extents& extents,
                        const EntityRange& entities) const {
        if constexpr (std::ranges::range<T>) {
            using ValueType = std::vector<MDRangeValueType<T>>;
            StructuredDataArray<ValueType, dim> out{extents};
            _fill_structured_data(out, data, entities);
            return out;
        } else {
            StructuredDataArray<T, dim> out{extents};
            _fill_structured_data(out, data, entities);
            return out;
        }
    }

    template<typename T, typename O, typename EntityRange>
        requires(Concepts::MDRange<O, 2>)
    void _fill_structured_data(StructuredDataArray<T, dim>& out,
                               const std::vector<O>& data,
                               const EntityRange& entities) const {
        std::ranges::for_each(entities, [&] (const auto& e) {
            const auto index_tuple = Ranges::to_array<dim>(location(this->grid(), e));
            const auto index = flat_index(this->grid(), e);
            std::ranges::for_each(data[index], [&] (const auto& srange) {
                std::ranges::copy(srange, std::back_inserter(out[index_tuple]));
            });
        });
    }
    template<typename T, typename O, typename EntityRange>
        requires(not Concepts::MDRange<O, 2>)
    void _fill_structured_data(StructuredDataArray<T, dim>& out,
                               const std::vector<O>& data,
                               const EntityRange& entities) const {
        std::ranges::for_each(entities, [&] (const auto& e) {
            const auto index_tuple = Ranges::to_array<dim>(location(this->grid(), e));
            out[index_tuple] = data[flat_index(this->grid(), e)];
        });
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_WRITER_HPP_
