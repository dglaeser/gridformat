// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTPWriter
 */
#ifndef GRIDFORMAT_VTK_PVTP_WRITER_HPP_
#define GRIDFORMAT_VTK_PVTP_WRITER_HPP_

#include <ostream>
#include <string>
#include <fstream>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/helpers.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/parallel.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for parallel .pvtu files
 */
template<Concepts::UnstructuredGrid Grid,
         Concepts::Communicator Communicator>
class PVTPWriter : public VTK::XMLWriterBase<Grid, PVTPWriter<Grid, Communicator>> {
    using ParentType = VTK::XMLWriterBase<Grid, PVTPWriter<Grid, Communicator>>;

 public:
    explicit PVTPWriter(const Grid& grid,
                        Communicator comm,
                        VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".pvtp", xml_opts)
    , _comm(comm)
    {}

 private:
    Communicator _comm;

    PVTPWriter _with(VTK::XMLOptions xml_opts) const override {
        return PVTPWriter{this->grid(), _comm, std::move(xml_opts)};
    }

    void _write(std::ostream&) const override {
        throw InvalidState(
            "PVTPWriter does not support direct export into stream. "
            "Use overload with filename instead!"
        );
    }

    virtual void _write(const std::string& filename_with_ext) const {
        _write_piece(filename_with_ext);
        Parallel::barrier(_comm);  // ensure all pieces finished successfully
        if (Parallel::rank(_comm) == 0)
            _write_pvtu_file(filename_with_ext);
        Parallel::barrier(_comm);  // ensure .pvtu file is written before returning
    }

    void _write_piece(const std::string& par_filename) const {
        VTPWriter writer{this->grid(), this->_xml_opts};
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

    void _write_pvtu_file(const std::string& filename_with_ext) const {
        std::ofstream file_stream(filename_with_ext, std::ios::out);

        XMLElement pvtk_xml("VTKFile");
        pvtk_xml.set_attribute("type", "PPolyData");

        XMLElement& grid = pvtk_xml.add_child("PPolyData");
        XMLElement& ppoint_data = grid.add_child("PPointData");
        XMLElement& pcell_data = grid.add_child("PCellData");
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
            }, this->_xml_settings.data_format);
        }, this->_xml_settings.encoder);

        XMLElement& point_array = grid.add_child("PPoints").add_child("PDataArray");
        point_array.set_attribute("NumberOfComponents", "3");
        std::visit([&] <typename T> (const Precision<T>& prec) {
            point_array.set_attribute("type", VTK::attribute_name(prec));
        }, this->_xml_settings.coordinate_precision);

        std::ranges::for_each(Parallel::ranks(_comm), [&] (int rank) {
            grid.add_child("Piece").set_attribute(
                "Source",
                PVTK::piece_basefilename(filename_with_ext, rank) + ".vtp"
            );
        });

        write_xml_with_version_header(pvtk_xml, file_stream, Indentation{{.width = 2}});
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTP_WRITER_HPP_
