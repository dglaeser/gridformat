// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVDWriter
 */
#ifndef GRIDFORMAT_VTK_PVD_WRITER_HPP_
#define GRIDFORMAT_VTK_PVD_WRITER_HPP_

#include <iomanip>
#include <sstream>
#include <fstream>
#include <utility>
#include <string>
#include <ranges>
#include <type_traits>
#include <filesystem>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/grid/writer.hpp>
#include <gridformat/grid.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace PVDDetail {

    template<typename W>
    class WriterStorage {
     public:
        explicit WriterStorage(W&& w) : _w(std::move(w)) {}

     protected:
        W& _writer() { return _w; }
        const W& _writer() const { return _w; }

     private:
        W _w;
    };

}  // namespace PVDDetail
#endif  // DOXYGEN

/*!
 * \ingroup VTK
 * \brief Writer for .pvd time-series file format
 */
template<typename VTKWriter>
class PVDWriter : public PVDDetail::WriterStorage<VTKWriter>,
                  public TimeSeriesGridWriter<typename VTKWriter::Grid> {
    using Storage = PVDDetail::WriterStorage<VTKWriter>;
    using ParentType = TimeSeriesGridWriter<typename VTKWriter::Grid>;

 public:
    explicit PVDWriter(VTKWriter&& writer, std::string base_filename)
    : Storage(std::move(writer))
    , ParentType(Storage::_writer().grid(), Storage::_writer().writer_options())
    , _base_filename{std::move(base_filename)}
    , _pvd_filename{_base_filename + ".pvd"}
    , _xml{"VTKFile"} {
        _xml.set_attribute("type", "Collection");
        _xml.set_attribute("version", "1.0");
        _xml.add_child("Collection");
        this->_writer().clear();
    }

 private:
    std::string _write(double _time) override {
        const auto time_step_index = _xml.get_child("Collection").number_of_children();
        const auto vtk_filename = _write_time_step_file(time_step_index);
        _add_dataset(_time, vtk_filename);

        const auto& communicator = Traits::CommunicatorAccess<VTKWriter>::get(this->_writer());
        if (Parallel::rank(communicator) == 0) {
            std::ofstream pvd_file(_pvd_filename, std::ios::out);
            write_xml_with_version_header(_xml, pvd_file, Indentation{{.width = 2}});
        }
        Parallel::barrier(communicator);  // make sure all process exit here after the pvd file is written

        return _pvd_filename;
    }

    std::string _write_time_step_file(const std::integral auto index) {
        this->copy_fields(this->_writer());
        const auto filename = this->_writer().write(
            _base_filename + "-" + _get_file_number_string(index)
        );
        this->_writer().clear();
        return filename;
    }

    std::string _get_file_number_string(const std::integral auto index) const {
        std::ostringstream file_number;
        file_number << std::setw(5) << std::setfill('0') << index;
        return file_number.str();
    }

    XMLElement& _add_dataset(const Concepts::Scalar auto _time, const std::string& filename) {
        auto& dataset = _xml.get_child("Collection").add_child("DataSet");
        dataset.set_attribute("timestep", _time);
        dataset.set_attribute("group", "");
        dataset.set_attribute("part", "0");
        dataset.set_attribute("name", "");
        dataset.set_attribute("file", std::filesystem::path{filename}.filename());
        return dataset;
    }

    std::string _base_filename;
    std::filesystem::path _pvd_filename;
    XMLElement _xml;
};

template<typename VTKWriter>
PVDWriter(VTKWriter&&) -> PVDWriter<std::remove_cvref_t<VTKWriter>>;

namespace Traits {

template<typename VTKWriter>
struct WritesConnectivity<PVDWriter<VTKWriter>> : public WritesConnectivity<VTKWriter> {};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVD_WRITER_HPP_
