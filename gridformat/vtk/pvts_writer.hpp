// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTSWriter
 */
#ifndef GRIDFORMAT_VTK_PVTS_WRITER_HPP_
#define GRIDFORMAT_VTK_PVTS_WRITER_HPP_

#include <ostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <array>
#include <tuple>
#include <cmath>

#include <gridformat/common/field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/helpers.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/parallel.hpp>
#include <gridformat/vtk/vts_writer.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for parallel .pvts files
 */
template<Concepts::ImageGrid Grid,
         Concepts::Communicator Communicator>
class PVTSWriter : public VTK::XMLWriterBase<Grid, PVTSWriter<Grid, Communicator>> {
    using ParentType = VTK::XMLWriterBase<Grid, PVTSWriter<Grid, Communicator>>;
    using CT = CoordinateType<Grid>;

    static constexpr std::size_t space_dim = 3;
    static constexpr std::size_t dim = dimension<Grid>;
    static constexpr int root_rank = 0;

 public:
    explicit PVTSWriter(const Grid& grid,
                        Communicator comm,
                        VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".pvts", xml_opts)
    , _comm(comm)
    {}

 private:
    Communicator _comm;

    PVTSWriter _with(VTK::XMLOptions xml_opts) const override {
        return PVTSWriter{this->grid(), _comm, std::move(xml_opts)};
    }

    void _write(std::ostream&) const override {
        throw InvalidState(
            "PVTSWriter does not support direct export into stream. "
            "Use overload with filename instead!"
        );
    }

    virtual void _write(const std::string& filename_with_ext) const {
        const auto& local_extents = extents(this->grid());
        const auto [origin, orientation] = _get_origin_and_orientations();

        const auto all_origins = Parallel::gather(_comm, origin, root_rank);
        const auto all_extents = Parallel::gather(_comm, local_extents, root_rank);
        const auto [exts_begin, exts_end, whole_extent] = _extents_and_origin(all_origins, all_extents, orientation);

        const auto my_whole_extent = Parallel::broadcast(_comm, whole_extent, root_rank);
        const auto my_extent_offset = Parallel::scatter(_comm, Ranges::as_flat_vector(exts_begin), root_rank);

        _write_piece(filename_with_ext, Ranges::to_array<dim>(my_extent_offset), {my_whole_extent});
        Parallel::barrier(_comm);  // ensure all pieces finished successfully
        if (Parallel::rank(_comm) == 0)
            _write_pvts_file(filename_with_ext, my_whole_extent, exts_begin, exts_end);
        Parallel::barrier(_comm);  // ensure .pvts file is written before returning
    }

    auto _get_origin_and_orientations() const {
        std::array<CT, dim> origin;
        std::array<int, dim> orientations;
        for (unsigned dir = 0; dir < dim; ++dir) {
            std::array<CT, 2> ordinates_01;
            std::ranges::copy(std::views::take(ordinates(this->grid(), dir), 2), ordinates_01.begin());
            origin[dir] = ordinates_01[0];
            orientations[dir] = ordinates_01[1] - ordinates_01[0] >= CT{0} ? 1 : -1;
        }
        return std::make_tuple(origin, orientations);
    }


    auto _extents_and_origin(const std::vector<CT>& all_origins,
                             const std::vector<std::size_t>& all_extents,
                             const std::array<int, dim>& orientations) const
    {
        const auto max_coord = std::ranges::max(all_origins, {}, [&] (const CT& x) {
            using std::abs; return abs(x);
        });
        const auto default_epsilon = 1e-6*max_coord;

        const auto num_ranks = Parallel::size(_comm);
        std::vector<std::array<std::size_t, dim>> pieces_begin(num_ranks);
        std::vector<std::array<std::size_t, dim>> pieces_end(num_ranks);
        std::array<std::size_t, dim> whole_extent;

        if (Parallel::rank(_comm) == 0) {
            const auto mapper_helper = _make_mapper_helper(all_origins, orientations, default_epsilon);
            const auto rank_mapper = mapper_helper.make_mapper();

            for (unsigned dir = 0; dir < dim; ++dir) {
                for (int rank = 0; rank < num_ranks; ++rank) {
                    auto ranks_below = rank_mapper.ranks_below(rank_mapper.location(rank), dir);
                    const std::size_t offset = std::accumulate(
                        std::ranges::begin(ranks_below),
                        std::ranges::end(ranks_below),
                        std::size_t{0},
                        [&] (const std::size_t current, int r) {
                            return current + Parallel::access_gathered<dim>(all_extents, _comm, {dir, r});
                        }
                    );
                    pieces_begin[rank][dir] = offset;
                    pieces_end[rank][dir] = offset + Parallel::access_gathered<dim>(all_extents, _comm, {dir, rank});
                }

                whole_extent[dir] = (*std::max_element(
                    pieces_end.begin(), pieces_end.end(),
                    [&] (const auto& a1, const auto& a2) { return a1[dir] < a2[dir]; }
                ))[dir];
            }
        }

        return std::make_tuple(
            std::move(pieces_begin),
            std::move(pieces_end),
            std::move(whole_extent)
        );
    }

    auto _make_mapper_helper(const std::vector<CT>& all_origins,
                             const std::array<int, dim>& orientations,
                             CT default_eps) const {
        PVTK::StructuredGridMapperHelper<CT, dim> helper(Parallel::size(_comm), default_eps);
        std::ranges::for_each(Parallel::ranks(_comm), [&] (int rank) {
            helper.set_origin_for(rank, Parallel::access_gathered<dim>(all_origins, _comm, rank));
        });

        // reverse orientation if spacing is negative
        for (unsigned dir = 0; dir < dim; ++dir)
            if (orientations[dir] < 0)
                helper.reverse(dir);

        return helper;
    }

    void _write_piece(const std::string& par_filename,
                      const std::array<std::size_t, dim>& offset,
                      typename VTSWriter<Grid>::Domain domain) const {
        auto writer = VTSWriter{this->grid(), this->_xml_opts}
                        .as_piece_for(std::move(domain))
                        .with_offset(offset);
        std::ranges::for_each(this->_meta_data_field_names(), [&] (const std::string& name) {
            writer.set_meta_data(name, VTK::make_vtk_field(this->_get_shared_meta_data_field(name)));
        });
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            writer.set_point_field(name, VTK::make_vtk_field(this->_get_shared_point_field(name)));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            writer.set_cell_field(name, this->_get_shared_cell_field(name));
        });
        writer.write(PVTK::piece_basefilename(par_filename, Parallel::rank(_comm)));
    }

    void _write_pvts_file(const std::string& filename_with_ext,
                          const std::array<std::size_t, dim>& extents,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_begin,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_end) const {
        std::ofstream file_stream(filename_with_ext, std::ios::out);

        XMLElement pvtk_xml("VTKFile");
        pvtk_xml.set_attribute("type", "PStructuredGrid");

        XMLElement& grid = pvtk_xml.add_child("PStructuredGrid");
        grid.set_attribute("WholeExtent", VTK::CommonDetail::extents_string(extents));

        XMLElement& ppoint_data = grid.add_child("PPointData");
        XMLElement& pcell_data = grid.add_child("PCellData");
        XMLElement& pcoords = grid.add_child("PPoints");
        std::visit([&] (const auto& encoder) {
            std::visit([&] (const auto& data_format) {
                PVTK::PDataArrayHelper pdata_helper{encoder, data_format, ppoint_data};
                PVTK::PDataArrayHelper cdata_helper{encoder, data_format, pcell_data};
                std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
                    pdata_helper.add(name, this->_get_point_field(name));
                });
                std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
                    cdata_helper.add(name, this->_get_cell_field(name));
                });

                std::visit([&] <typename T> (const Precision<T>& prec) {
                    XMLElement& pdata_array = pcoords.add_child("PDataArray");
                    pdata_array.set_attribute("NumberOfComponents", std::to_string(space_dim));
                    pdata_array.set_attribute("Name", "Coordinates");
                    pdata_array.set_attribute("format", VTK::data_format_name(encoder, data_format));
                    pdata_array.set_attribute("type", VTK::attribute_name(prec));
                }, this->_xml_settings.coordinate_precision);
            }, this->_xml_settings.data_format);
        }, this->_xml_settings.encoder);

        std::ranges::for_each(Parallel::ranks(_comm), [&] (int rank) {
            std::array<std::size_t, dim> extents_begin;
            std::array<std::size_t, dim> extents_end;
            for (unsigned dir = 0; dir < dim; ++dir) {
                extents_begin[dir] = proc_extents_begin[rank][dir];
                extents_end[dir] = proc_extents_end[rank][dir];
            }
            auto& piece = grid.add_child("Piece");
            piece.set_attribute("Extent", VTK::CommonDetail::extents_string(extents_begin, extents_end));
            piece.set_attribute("Source", PVTK::piece_basefilename(filename_with_ext, rank) + ".vts");
        });

        write_xml_with_version_header(pvtk_xml, file_stream, Indentation{{.width = 2}});
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTS_WRITER_HPP_
