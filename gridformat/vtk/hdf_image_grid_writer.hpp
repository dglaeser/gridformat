// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Writer for the VTK HDF file format for image grids.
 */
#ifndef GRIDFORMAT_VTK_HDF_IMAGE_GRID_WRITER_HPP_
#define GRIDFORMAT_VTK_HDF_IMAGE_GRID_WRITER_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <ranges>
#include <iterator>
#include <algorithm>
#include <utility>
#include <tuple>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/matrix.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/type_traits.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/concepts.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/writer.hpp>

#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/parallel.hpp>
#include <gridformat/vtk/hdf_common.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace VTKHDFImageDetail {

    template<std::size_t dim>
    struct FieldDataStorage;

    template<>
    struct FieldDataStorage<1> {
        template<typename T>
        using type = std::vector<T>;

        template<typename T, Concepts::StaticallySizedMDRange<1> Extents>
            requires(static_size<Extents> == 1)
        static auto resize(type<T>& buffer, const Extents& extents) {
            buffer.resize(Ranges::at(0, extents));
        }
    };

    template<>
    struct FieldDataStorage<2> {
        template<typename T>
        using type = std::vector<std::vector<T>>;

        template<typename T, Concepts::StaticallySizedMDRange<1> Extents>
            requires(static_size<Extents> == 2)
        static auto resize(type<T>& buffer, const Extents& extents) {
            buffer.resize(Ranges::at(1, extents));
            std::ranges::for_each(buffer, [&] (std::vector<T>& sub_range) {
                sub_range.resize(Ranges::at(0, extents));
            });
        }
    };

    template<>
    struct FieldDataStorage<3> {
        template<typename T>
        using type = std::vector<std::vector<std::vector<T>>>;

        template<typename T, Concepts::StaticallySizedMDRange<1> Extents>
            requires(static_size<Extents> == 3)
        static auto resize(type<T>& buffer, const Extents& extents) {
            buffer.resize(Ranges::at(2, extents));
            std::ranges::for_each(buffer, [&] (std::ranges::range auto& sub_range) {
                sub_range.resize(Ranges::at(1, extents));
                std::ranges::for_each(sub_range, [&] (std::vector<T>& last_range) {
                    last_range.resize(Ranges::at(0, extents));
                });
            });
        }
    };

}  // namespace VTKHDFImageDetail
#endif  // DOXYGEN


/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<bool is_transient, Concepts::ImageGrid Grid, Concepts::Communicator Communicator = NullCommunicator>
class VTKHDFImageGridWriterImpl : public GridDetail::WriterBase<is_transient, Grid>::type {
    static constexpr int root_rank = 0;
    static constexpr std::size_t dim = dimension<Grid>;
    static constexpr std::size_t vtk_space_dim = 3;
    static constexpr std::array<std::size_t, 2> version{1, 0};

    using CT = CoordinateType<Grid>;
    template<typename T> using Vector = std::array<T, vtk_space_dim>;
    template<typename T> using Tensor = std::array<T, vtk_space_dim*vtk_space_dim>;

    using DataSetSlice = VTKHDF::DataSetSlice;
    using DataSetPath = VTKHDF::DataSetPath;
    using IOContext = VTKHDF::IOContext;
    using HDF5File = VTKHDF::HDF5File<Communicator>;

    struct ImageSpecs {
        const std::array<CT, dim> origin;
        const std::array<CT, dim> spacing;
        const std::array<std::size_t, dim> extents;

        template<typename O, typename S, typename E>
        ImageSpecs(O&& origin, S&& spacing, E&& extents)
        : origin{Ranges::to_array<dim, CT>(std::forward<O>(origin))}
        , spacing{Ranges::to_array<dim, CT>(std::forward<S>(spacing))}
        , extents{Ranges::to_array<dim, std::size_t>(std::forward<E>(extents))}
        {}
    };

    static constexpr WriterOptions writer_opts{
        .use_structured_grid_ordering = true,
        .append_null_terminator_to_strings = true
    };

 public:
    explicit VTKHDFImageGridWriterImpl(const Grid& grid)
        requires(std::is_same_v<Communicator, NullCommunicator>)
    : GridWriter<Grid>(grid, ".hdf", writer_opts)
    {}

    explicit VTKHDFImageGridWriterImpl(const Grid& grid, const Communicator& comm)
        requires(std::is_copy_constructible_v<Communicator>)
    : GridWriter<Grid>(grid, ".hdf", writer_opts)
    , _comm{comm}
    {}

    explicit VTKHDFImageGridWriterImpl(const Grid& grid,
                                       std::string filename_without_extension,
                                       VTK::HDFTransientOptions opts = {})
        requires(is_transient && std::is_same_v<Communicator, NullCommunicator>)
    : TimeSeriesGridWriter<Grid>(grid, writer_opts)
    , _comm{}
    , _timeseries_filename{std::move(filename_without_extension) + ".hdf"}
    , _transient_opts{std::move(opts)}
    {}

    explicit VTKHDFImageGridWriterImpl(const Grid& grid,
                                       const Communicator& comm,
                                       std::string filename_without_extension,
                                       VTK::HDFTransientOptions opts = {})
        requires(is_transient && std::is_copy_constructible_v<Communicator>)
    : TimeSeriesGridWriter<Grid>(grid, writer_opts)
    , _comm{comm}
    , _timeseries_filename{std::move(filename_without_extension) + ".hdf"}
    , _transient_opts{std::move(opts)}
    {}

 private:
    void _write(std::ostream&) const requires(!is_transient) {
        throw InvalidState("VTKHDFImageGridWriter does not support export into stream");
    }

    std::string _write([[maybe_unused]] double t) requires(is_transient) {
        if constexpr (is_transient)
            if (this->_step_count == 0)
                HDF5File::clear(_timeseries_filename, _comm);

        HDF5File file{_timeseries_filename, _comm, HDF5File::Mode::append};
        _write_to(file);
        file.set_attribute("NSteps", this->_step_count+1, "VTKHDF/Steps");
        file.write(std::array{t}, {"VTKHDF/Steps", "Values"});
        return _timeseries_filename;
    }

    void _write(const std::string& filename_with_ext) const requires(!is_transient)  {
        HDF5File file{filename_with_ext, _comm, HDF5File::Mode::overwrite};
        _write_to(file);
    }

    void _write_to(HDF5File& file) const {
        const ImageSpecs my_specs{origin(this->grid()), spacing(this->grid()), extents(this->grid())};
        const auto [overall_specs, my_offset] = _get_image_specs(my_specs);
        const auto cell_slice_base = _make_slice(
            overall_specs.extents,
            my_specs.extents,
            my_offset
        );
        const auto point_slice_base = _make_slice(
            Ranges::incremented(overall_specs.extents, 1),
            Ranges::incremented(my_specs.extents, 1),
            my_offset
        );

        file.set_attribute("Version", std::array<std::size_t, 2>{(is_transient ? 2 : 1), 0}, "VTKHDF");
        file.set_attribute("Origin", Ranges::to_array<vtk_space_dim>(overall_specs.origin), "VTKHDF");
        file.set_attribute("Spacing", Ranges::to_array<vtk_space_dim>(overall_specs.spacing), "VTKHDF");
        file.set_attribute("WholeExtent", VTK::CommonDetail::get_extents(overall_specs.extents), "VTKHDF");
        file.set_attribute("Direction", _get_direction(), "VTKHDF");
        file.set_attribute("Type", "ImageData", "VTKHDF");

        // TODO: There seem to be issues with the vtkImageDataReader when parsing field data?
        // std::ranges::for_each(this->_meta_data_field_names(), [&] (const std::string& name) {
        //     auto field_ptr = this->_get_meta_data_field_ptr(name);
        //     _visit_meta_data_field_values(*field_ptr, [&] (auto&& values) {
        //         file.write(std::move(values), {"VTKHDF/FieldData", name});
        //     });
        // });

        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            auto field_ptr = VTK::make_vtk_field(this->_get_point_field_ptr(name));
            _visit_field_values<true>(*field_ptr, [&] (auto&& values) {
                _write_piece_values(file, std::move(values), {"VTKHDF/PointData", name}, point_slice_base);
            });
        });

        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            auto field_ptr = VTK::make_vtk_field(this->_get_cell_field_ptr(name));
            _visit_field_values<false>(*field_ptr, [&] (auto&& values) {
                _write_piece_values(file, std::move(values), {"VTKHDF/CellData", name}, cell_slice_base);
            });
        });
    }

    auto _get_image_specs(const ImageSpecs& piece_specs) const {
        using OffsetType = std::ranges::range_value_t<decltype(piece_specs.extents)>;
        if (Parallel::size(_comm) > 1) {
            PVTK::StructuredParallelGridHelper helper{_comm};
            const auto all_origins = Parallel::gather(_comm, piece_specs.origin, root_rank);
            const auto all_extents = Parallel::gather(_comm, piece_specs.extents, root_rank);
            const auto is_negative_axis = VTK::CommonDetail::structured_grid_axis_orientation(piece_specs.spacing);
            const auto [exts_begin, exts_end, whole_extent, origin] = helper.compute_extents_and_origin(
                all_origins,
                all_extents,
                is_negative_axis,
                basis(this->grid())
            );

            const auto my_whole_extent = Parallel::broadcast(_comm, whole_extent, root_rank);
            const auto my_whole_origin = Parallel::broadcast(_comm, origin, root_rank);
            const auto my_extent_offset = Parallel::scatter(_comm, Ranges::flat(exts_begin), root_rank);
            return std::make_tuple(
                ImageSpecs{my_whole_origin, piece_specs.spacing, my_whole_extent},
                my_extent_offset
            );
        } else {
            return std::make_tuple(piece_specs, std::vector<OffsetType>(dim, 0));
        }
    }

    template<typename TotalExtents, typename Extents, typename Offsets>
    DataSetSlice _make_slice(const TotalExtents& total_extents,
                             const Extents& extents,
                             const Offsets& offsets) const {
        DataSetSlice result;
        std::ranges::copy(total_extents, std::back_inserter(result.size));
        std::ranges::copy(extents, std::back_inserter(result.count));
        std::ranges::copy(offsets, std::back_inserter(result.offset));
        // slices are accessed with the last coordinate first (i.e. values[z][y][x])
        std::ranges::reverse(result.size);
        std::ranges::reverse(result.count);
        std::ranges::reverse(result.offset);
        return result;
    }

    auto _get_direction() const {
        using T = MDRangeScalar<decltype(basis(this->grid()))>;
        std::array<T, vtk_space_dim*vtk_space_dim> coefficients;
        std::ranges::fill(coefficients, T{0});
        std::ranges::for_each(
            Matrix{basis(this->grid())}.transposed(),
            [it = coefficients.begin()] (const std::ranges::range auto& row) mutable {
                std::ranges::copy(row, it);
                std::advance(it, vtk_space_dim);
            }
        );
        return coefficients;
    }

    template<bool is_point_field, typename Visitor>
    void _visit_field_values(const Field& field, const Visitor& visitor) const {
        field.precision().visit([&] <typename T> (const Precision<T>&) {
            using Helper = VTKHDFImageDetail::FieldDataStorage<dim>;

            const auto layout = field.layout();
            const auto size = is_point_field ? Ranges::incremented(extents(this->grid()), 1)
                                             : extents(this->grid());
            if (layout.dimension() == 1) {
                typename Helper::template type<T> data;
                Helper::resize(data, size);
                field.export_to(data | Views::flat);
                visitor(std::move(data));
            } else if (layout.dimension() == 2) {
                typename Helper::template type<Vector<T>> data;
                Helper::resize(data, size);
                field.export_to(data | Views::flat);
                visitor(std::move(data));
            } else if (layout.dimension() == 3) {
                typename Helper::template type<Tensor<T>> data;
                Helper::resize(data, size);
                field.export_to(data | Views::flat);
                visitor(std::move(data));
            } else {
                throw NotImplemented("Support for fields with dimension > 3 or < 1");
            }
        });
    }

    template<typename Visitor>
    void _visit_meta_data_field_values(const Field& field, const Visitor& visitor) const {
        field.precision().visit([&] <typename T> (const Precision<T>&) {
            const auto layout = field.layout();
            if (layout.dimension() == 1) {
                std::vector<T> data(field.layout().extent(0));
                field.export_to(data);
                visitor(std::move(data));
            } else if (layout.dimension() == 2) {
                std::vector<Vector<T>> data(field.layout().extent(0));
                field.export_to(data);
                visitor(std::move(data));
            } else if (layout.dimension() == 3) {
                std::vector<Tensor<T>> data(field.layout().extent(0));
                field.export_to(data);
                visitor(std::move(data));
            } else {
                throw NotImplemented("Support for fields with dimension > 3 or < 1");
            }
        });
    }

    template<std::ranges::range Values>
    void _write_piece_values(HDF5File& file,
                             Values&& values,
                             const DataSetPath& path,
                             const DataSetSlice& slice) const {
        std::vector<std::size_t> size{slice.size.begin(), slice.size.end()};
        std::vector<std::size_t> count{slice.count.begin(), slice.count.end()};
        std::vector<std::size_t> offset{slice.offset.begin(), slice.offset.end()};

        if constexpr (mdrange_dimension<Values> > 1) {
            using FieldType = MDRangeValueTypeAt<dim-1, Values>;
            const auto layout = get_md_layout<FieldType>();
            for (std::size_t i = 0; i < layout.dimension(); ++i) {
                size.push_back(layout.extent(i));
                count.push_back(layout.number_of_entries());
                offset.push_back(0);
            }
        }

        if constexpr (is_transient) {
            size.insert(size.begin(), 1);
            offset.insert(offset.begin(), 0);
            count.insert(count.begin(), 1);
            file.write(std::array{std::move(values)}, path, {
                .size = size,
                .offset = offset,
                .count = count
            });
        } else {
            file.write(values, path, {
                .size = size,
                .offset = offset,
                .count = count
            });
        }
    }

    Communicator _comm;
    std::string _timeseries_filename = "";
    VTK::HDFTransientOptions _transient_opts;
};

template<Concepts::ImageGrid G, Concepts::Communicator C = NullCommunicator>
class VTKHDFImageGridWriter : public VTKHDFImageGridWriterImpl<false, G, C> {
    using ParentType = VTKHDFImageGridWriterImpl<false, G, C>;
 public:
    using ParentType::ParentType;
};

template<Concepts::ImageGrid G, Concepts::Communicator C = NullCommunicator>
class VTKHDFImageGridTimeSeriesWriter : public VTKHDFImageGridWriterImpl<true, G, C> {
    using ParentType = VTKHDFImageGridWriterImpl<true, G, C>;
 public:
    using ParentType::ParentType;
};

template<Concepts::ImageGrid G>
VTKHDFImageGridWriter(const G&) -> VTKHDFImageGridWriter<G, NullCommunicator>;
template<Concepts::ImageGrid G, Concepts::Communicator C>
VTKHDFImageGridWriter(const G&, const C&) -> VTKHDFImageGridWriter<G, C>;

template<Concepts::ImageGrid G>
VTKHDFImageGridTimeSeriesWriter(const G&, std::string, VTK::HDFTransientOptions = {}) -> VTKHDFImageGridTimeSeriesWriter<G, NullCommunicator>;
template<Concepts::ImageGrid G, Concepts::Communicator C>
VTKHDFImageGridTimeSeriesWriter(const G&, const C&, std::string, VTK::HDFTransientOptions = {}) -> VTKHDFImageGridTimeSeriesWriter<G, C>;


}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_IMAGE_GRID_WRITER_HPP_
