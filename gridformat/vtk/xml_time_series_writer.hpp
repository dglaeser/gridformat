// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTKXMLTimeSeriesWriter
 */
#ifndef GRIDFORMAT_VTK_XML_TIME_SERIES_WRITER_HPP_
#define GRIDFORMAT_VTK_XML_TIME_SERIES_WRITER_HPP_

#include <iomanip>
#include <sstream>
#include <utility>
#include <string>
#include <type_traits>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for time series of a VTK-XML file format.
 *        Populates the "TimeValue" metadata field supported by VTK.
 */
template<typename VTKWriter>
class VTKXMLTimeSeriesWriter : public TimeSeriesGridWriter<typename VTKWriter::Grid> {
    using ParentType = TimeSeriesGridWriter<typename VTKWriter::Grid>;

 public:
    explicit VTKXMLTimeSeriesWriter(VTKWriter&& writer, std::string base_filename)
    : ParentType(writer.grid(), writer.writer_options())
    , _vtk_writer{std::move(writer)}
    , _base_filename{std::move(base_filename)}
    {}

 private:
    std::string _write(double _time) override {
        this->copy_fields(_vtk_writer);
        _vtk_writer.set_meta_data("TimeValue", _time);
        const auto filename = _vtk_writer.write(_get_filename(this->_step_count));
        _vtk_writer.clear();
        return filename;
    }

    std::string _get_filename(const std::integral auto index) const {
        return _base_filename + "-" + _get_file_number_string(index);
    }

    std::string _get_file_number_string(const std::integral auto index) const {
        std::ostringstream file_number;
        file_number << std::setw(5) << std::setfill('0') << index;
        return file_number.str();
    }

    VTKWriter _vtk_writer;
    std::string _base_filename;
    std::string _pvd_filename;
};

template<typename VTKWriter>
VTKXMLTimeSeriesWriter(VTKWriter&&) -> VTKXMLTimeSeriesWriter<std::remove_cvref_t<VTKWriter>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_XML_TIME_SERIES_WRITER_HPP_
