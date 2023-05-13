// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTKTimeSeriesWriter
 */
#ifndef GRIDFORMAT_VTK_TIME_SERIES_WRITER_HPP_
#define GRIDFORMAT_VTK_TIME_SERIES_WRITER_HPP_

#include <iomanip>
#include <sstream>
#include <utility>
#include <string>
#include <type_traits>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<typename VTKWriter>
class VTKTimeSeriesWriter : public TimeSeriesGridWriter<typename VTKWriter::Grid> {
    using ParentType = TimeSeriesGridWriter<typename VTKWriter::Grid>;

 public:
    explicit VTKTimeSeriesWriter(VTKWriter&& writer, std::string base_filename)
    : ParentType(writer.grid(), writer.uses_structured_ordering())
    , _vtk_writer{std::move(writer)}
    , _base_filename{std::move(base_filename)}
    {}

    template<typename T>
    void set_meta_data(const std::string& name, T&& value) {
        ParentType::set_meta_data(name, std::forward<T>(value));
    }

    void set_meta_data(const std::string& name, std::string text) {
        text.push_back('\0');
        ParentType::set_meta_data(name, std::move(text));
    }

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
VTKTimeSeriesWriter(VTKWriter&&) -> VTKTimeSeriesWriter<std::decay_t<VTKWriter>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_TIME_SERIES_WRITER_HPP_
