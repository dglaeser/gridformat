// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <gridformat/common/lvalue_reference.hpp>
#include <gridformat/common/field_transformations.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/concepts.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/writer.hpp>

#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/parallel.hpp>
#include <gridformat/vtk/hdf_common.hpp>

namespace GridFormat {

template<bool is_transient, Concepts::ImageGrid Grid, Concepts::Communicator Communicator = NullCommunicator>
class VTKHDFImageGridWriterImpl : public GridDetail::WriterBase<is_transient, Grid>::type {
    static constexpr int root_rank = 0;
    static constexpr std::size_t dim = dimension<Grid>;
    static constexpr std::size_t vtk_space_dim = 3;
    static constexpr std::array<std::size_t, 2> version{1, 0};

    using CT = CoordinateType<Grid>;
    using IOContext = VTKHDF::IOContext;
    using HDF5File = HDF5::File<Communicator>;

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
    explicit VTKHDFImageGridWriterImpl(LValueReferenceOf<const Grid> grid)
        requires(std::is_same_v<Communicator, NullCommunicator>)
    : GridWriter<Grid>(grid.get(), ".hdf", writer_opts)
    {}

    explicit VTKHDFImageGridWriterImpl(LValueReferenceOf<const Grid> grid, const Communicator& comm)
        requires(std::is_copy_constructible_v<Communicator>)
    : GridWriter<Grid>(grid.get(), ".hdf", writer_opts)
    , _comm{comm}
    {}

    explicit VTKHDFImageGridWriterImpl(LValueReferenceOf<const Grid> grid,
                                       std::string filename_without_extension,
                                       VTK::HDFTransientOptions opts = {
                                            .static_grid = true,
                                            .static_meta_data = false
                                       })
        requires(is_transient && std::is_same_v<Communicator, NullCommunicator>)
    : VTKHDFImageGridWriterImpl(grid.get(), NullCommunicator{}, filename_without_extension, std::move(opts))
    {}

    explicit VTKHDFImageGridWriterImpl(LValueReferenceOf<const Grid> grid,
                                       const Communicator& comm,
                                       std::string filename_without_extension,
                                       VTK::HDFTransientOptions opts = {
                                            .static_grid = true,
                                            .static_meta_data = false
                                       })
        requires(is_transient && std::is_copy_constructible_v<Communicator>)
    : TimeSeriesGridWriter<Grid>(grid.get(), writer_opts)
    , _comm{comm}
    , _timeseries_filename{std::move(filename_without_extension) + ".hdf"}
    , _transient_opts{std::move(opts)} {
        if (!_transient_opts.static_grid)
            throw ValueError("Transient VTK-HDF ImageData files do not support evolving grids");
    }

    const Communicator& communicator() const {
        return _comm;
    }

 private:
    void _write(std::ostream&) const {
        throw InvalidState("VTKHDFImageGridWriter does not support export into stream");
    }

    std::string _write([[maybe_unused]] double t) {
        if constexpr (!is_transient)
            throw InvalidState("This overload only works for transient output");

        if (this->_step_count == 0)
            HDF5File::clear(_timeseries_filename, _comm);

        HDF5File file{_timeseries_filename, _comm, HDF5File::Mode::append};
        _write_to(file);
        file.write_attribute(this->_step_count+1, "/VTKHDF/Steps/NSteps");
        file.write(std::array{t}, "/VTKHDF/Steps/Values");
        return _timeseries_filename;
    }

    void _write(const std::string& filename_with_ext) const {
        if constexpr (is_transient)
            throw InvalidState("This overload only works for non-transient output");
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

        // in order to avoid overlapping hyperslabs for point data in parallel I/O,
        // we only write the last entries of the slab (per direction) when our portion
        // of the image is the last portion of the overall image in that direction.
        const auto my_point_extents = Ranges::to_array<dim>(
            std::views::iota(std::size_t{0}, dim)
            | std::views::transform([&] (std::size_t dir) {
                const auto overall_end = overall_specs.extents[dir];
                const auto my_end = my_specs.extents[dir] + my_offset[dir];
                return my_end < overall_end ? my_specs.extents[dir] : my_specs.extents[dir] + 1;
        }));
        const auto point_slice_base = _make_slice<true>(
            overall_specs.extents,
            my_point_extents,
            my_offset
        );

        file.write_attribute(std::array<std::size_t, 2>{(is_transient ? 2 : 1), 0}, "/VTKHDF/Version");
        file.write_attribute(Ranges::to_array<vtk_space_dim>(overall_specs.origin), "/VTKHDF/Origin");
        file.write_attribute(Ranges::to_array<vtk_space_dim>(overall_specs.spacing), "/VTKHDF/Spacing");
        file.write_attribute(VTK::CommonDetail::get_extents(overall_specs.extents), "/VTKHDF/WholeExtent");
        file.write_attribute(_get_direction(), "/VTKHDF/Direction");
        file.write_attribute("ImageData", "/VTKHDF/Type");

        std::ranges::for_each(this->_meta_data_field_names(), [&] (const std::string& name) {
            if constexpr (is_transient) {
                if (this->_step_count > 0 && _transient_opts.static_meta_data) {
                    file.write(std::array{0}, "/VTKHDF/Steps/FieldDataOffsets/" + name);
                    return;
                } else {
                    file.write(std::array{this->_step_count}, "/VTKHDF/Steps/FieldDataOffsets/" + name);
                }
                auto field_ptr = this->_get_meta_data_field_ptr(name);
                auto sub = make_field_ptr(TransformedField{field_ptr, FieldTransformation::as_sub_field});
                _write_field(file, sub, "/VTKHDF/FieldData/" + name, _slice_from(sub));
            } else {
                auto field_ptr = this->_get_meta_data_field_ptr(name);
                _write_field(file, field_ptr, "/VTKHDF/FieldData/" + name, _slice_from(field_ptr));
            }

        });

        std::vector<std::size_t> non_zero_extents;
        std::ranges::copy(
            my_specs.extents | std::views::filter([] (auto e) { return e != 0; }),
            std::back_inserter(non_zero_extents)
        );

        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            auto field_ptr = _reshape(
                VTK::make_vtk_field(this->_get_point_field_ptr(name)),
                Ranges::incremented(non_zero_extents, 1) | std::views::reverse,
                point_slice_base.count
            );
            _write_field(file, field_ptr, "/VTKHDF/PointData/" + name, point_slice_base);
        });

        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            auto field_ptr = _reshape(
                VTK::make_vtk_field(this->_get_cell_field_ptr(name)),
                non_zero_extents | std::views::reverse,
                cell_slice_base.count
            );
            _write_field(file, field_ptr, "/VTKHDF/CellData/" + name, cell_slice_base);
        });
    }

    template<std::ranges::range E, std::ranges::range S>
    FieldPtr _reshape(FieldPtr f, const E& row_major_extents, S&& _slice_end) const {
        auto flat = _flatten(f);
        auto structured = _make_structured(flat, row_major_extents);
        const auto structured_layout = structured->layout();

        std::vector<std::size_t> slice_end;
        std::ranges::copy(_slice_end, std::back_inserter(slice_end));
        for (std::size_t i = slice_end.size(); i < structured_layout.dimension(); ++i)
            slice_end.push_back(structured_layout.extent(i));
        return transform(structured, FieldTransformation::take_slice({
            .from = std::vector<std::size_t>(slice_end.size(), 0),
            .to = slice_end
        }));
    }

    FieldPtr _flatten(FieldPtr f) const {
        const auto layout = f->layout();
        if (layout.dimension() <= 2)
            return f;
        // vtk requires tensors to be made flat
        auto nl = MDLayout{{layout.extent(0), layout.number_of_entries(1)}};
        return transform(f, FieldTransformation::reshape_to(std::move(nl)));
    }

    template<std::ranges::range E>
    FieldPtr _make_structured(FieldPtr f, const E& row_major_extents) const {
        const auto layout = f->layout();
        std::vector<std::size_t> target_layout(Ranges::size(row_major_extents));
        std::ranges::copy(row_major_extents, target_layout.begin());
        if (layout.dimension() > 1)
            target_layout.push_back(layout.extent(1));
        return transform(f, FieldTransformation::reshape_to(MDLayout{std::move(target_layout)}));
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

    template<bool increment = false, typename TotalExtents, typename Extents, typename Offsets>
    HDF5::Slice _make_slice(const TotalExtents& total_extents,
                            const Extents& extents,
                            const Offsets& offsets) const {
        HDF5::Slice result;
        result.total_size.emplace();

        // use only those dimensions that are not zero
        std::vector<bool> is_nonzero;
        std::ranges::copy(
            total_extents | std::views::transform([] (const auto& v) { return v != 0; }),
            std::back_inserter(is_nonzero)
        );
        std::ranges::for_each(total_extents, [&, i=0] (const auto& value) mutable {
            if (is_nonzero[i++])
                result.total_size.value().push_back(value + (increment ? 1 : 0));
        });
        std::ranges::for_each(extents, [&, i=0] (const auto& value) mutable {
            if (is_nonzero[i++])
                result.count.push_back(value);
        });
        std::ranges::for_each(offsets, [&, i=0] (const auto& value) mutable {
            if (is_nonzero[i++])
                result.offset.push_back(value);
        });

        // slices in VTK are accessed with the last coordinate first (i.e. values[z][y][x])
        std::ranges::reverse(result.total_size.value());
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

    void _write_field(HDF5File& file,
                      FieldPtr field,
                      const std::string& path,
                      const HDF5::Slice& slice) const {
        const std::size_t dimension_offset = is_transient ? 1 : 0;
        std::vector<std::size_t> size(slice.total_size.value().size() + dimension_offset);
        std::vector<std::size_t> count(slice.count.size() + dimension_offset);
        std::vector<std::size_t> offset(slice.offset.size() + dimension_offset);
        std::ranges::copy(slice.total_size.value(), std::ranges::begin(size | std::views::drop(dimension_offset)));
        std::ranges::copy(slice.count, std::ranges::begin(count | std::views::drop(dimension_offset)));
        std::ranges::copy(slice.offset, std::ranges::begin(offset | std::views::drop(dimension_offset)));

        const auto layout = field->layout();
        const bool is_vector_field = layout.dimension() > size.size() - dimension_offset;
        if (is_vector_field)
            std::ranges::for_each(
                std::views::iota(size.size() - dimension_offset, layout.dimension()),
                [&] (const std::size_t codim) {
                    size.push_back(layout.extent(codim));
                    count.push_back(layout.extent(codim));
                    offset.push_back(0);
                }
            );

        if constexpr (is_transient) {
            size.at(0) = 1;
            count.at(0) = 1;
            offset.at(0) = 0;
            FieldPtr sub_field = transform(field, FieldTransformation::as_sub_field);
            file.write(*sub_field, path, HDF5::Slice{
                .offset = std::move(offset),
                .count = std::move(count),
                .total_size = std::move(size)
            });
        } else {
            file.write(*field, path, HDF5::Slice{
                .offset = std::move(offset),
                .count = std::move(count),
                .total_size = std::move(size)
            });
        }
    }

    HDF5::Slice _slice_from(FieldPtr field) const {
        const auto layout = field->layout();
        std::vector<std::size_t> dims(layout.dimension());
        std::vector<std::size_t> offset(layout.dimension(), 0);
        layout.export_to(dims);
        return {
            .offset = offset,
            .count = dims,
            .total_size = dims
        };
    }

    Communicator _comm;
    std::string _timeseries_filename = "";
    VTK::HDFTransientOptions _transient_opts;
};

/*!
 * \ingroup VTK
 * \brief Writer for the VTK HDF file format for image grids.
 */
template<Concepts::ImageGrid G, Concepts::Communicator C = NullCommunicator>
class VTKHDFImageGridWriter : public VTKHDFImageGridWriterImpl<false, G, C> {
    using ParentType = VTKHDFImageGridWriterImpl<false, G, C>;
 public:
    using ParentType::ParentType;
};

/*!
 * \ingroup VTK
 * \brief Writer for the transient VTK HDF file format for image grids.
 */
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


namespace Traits {

template<typename... Args>
struct WritesConnectivity<VTKHDFImageGridWriter<Args...>> : public std::false_type {};

template<typename... Args>
struct WritesConnectivity<VTKHDFImageGridTimeSeriesWriter<Args...>> : public std::false_type {};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_IMAGE_GRID_WRITER_HPP_
