// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVDReader
 */
#ifndef GRIDFORMAT_VTK_PVD_READER_HPP_
#define GRIDFORMAT_VTK_PVD_READER_HPP_

#include <memory>
#include <optional>
#include <filesystem>
#include <iterator>

#include <gridformat/common/string_conversion.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/reader.hpp>
#include <gridformat/vtk/xml.hpp>

#include <gridformat/vtk/vtu_reader.hpp>
#include <gridformat/vtk/vtp_reader.hpp>
#include <gridformat/vtk/vts_reader.hpp>
#include <gridformat/vtk/vtr_reader.hpp>
#include <gridformat/vtk/vti_reader.hpp>

#include <gridformat/vtk/pvtu_reader.hpp>
#include <gridformat/vtk/pvtp_reader.hpp>
#include <gridformat/vtk/pvts_reader.hpp>
#include <gridformat/vtk/pvtr_reader.hpp>
#include <gridformat/vtk/pvti_reader.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .pvd time series file format
 */
template<Concepts::Communicator C = NullCommunicator>
class PVDReader : public GridReader {
    struct Step {
        std::string filename;
        double time;
    };

 public:
    PVDReader() requires(std::same_as<C, NullCommunicator>) {}
    explicit PVDReader(const C& comm) : _communicator{comm} {}

 private:
    std::string _name() const override {
        return "PVDReader";
    }

    void _open(const std::string& filename, typename GridReader::FieldNames& names) override {
        _reset();
        _read_steps(filename);
        _make_step_reader();
        _read_current_field_names(names);
    }

    void _close() override {
        _reset();
    }

    std::size_t _number_of_cells() const override {
        return _access_reader().number_of_cells();
    }

    std::size_t _number_of_points() const override {
        return _access_reader().number_of_points();
    }

    virtual FieldPtr _cell_field(std::string_view name) const override {
        return _access_reader().cell_field(name);
    }

    virtual FieldPtr _point_field(std::string_view name) const override {
        return _access_reader().point_field(name);
    }

    virtual FieldPtr _meta_data_field(std::string_view name) const override {
        return _access_reader().meta_data_field(name);
    }

    virtual void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        _access_reader().visit_cells(visitor);
    }

    virtual FieldPtr _points() const override {
        return _access_reader().points();
    }

    virtual typename GridReader::PieceLocation _location() const override {
        return _access_reader().location();
    }

    virtual std::vector<double> _ordinates(unsigned int i) const override {
        return _access_reader().ordinates(i);
    }

    virtual typename GridReader::Vector _spacing() const override {
        return _access_reader().spacing();
    }

    virtual typename GridReader::Vector _origin() const override {
        return _access_reader().origin();
    }

    virtual typename GridReader::Vector _basis_vector(unsigned int i) const override  {
        return _access_reader().basis_vector(i);
    }

    virtual bool _is_sequence() const override {
        return true;
    }

    std::size_t _number_of_steps() const override {
        return _steps.size();
    }

    double _time_at_step(std::size_t step_idx) const override {
        return _steps.at(step_idx).time;
    }

    void _set_step(std::size_t step_idx, typename GridReader::FieldNames& names) override {
        names.clear();
        _step_index = step_idx;
        _make_step_reader();
        _read_current_field_names(names);
    }

    void _read_steps(const std::string& filename) {
        auto helper = VTK::XMLReaderHelper::make_from(filename, "Collection");
        for (const auto& data_set : children(helper.get("Collection")) | std::views::filter([] (const XMLElement& e) {
            return e.name() == "DataSet";
        }))
            _steps.emplace_back(Step{
                .filename = _get_piece_path(data_set.get_attribute("file"), filename),
                .time = from_string<double>(data_set.get_attribute("timestep"))
            });
    }

    std::string _get_piece_path(std::filesystem::path vtk_file,
                                std::filesystem::path pvtk_file) const {
        if (vtk_file.is_absolute())
            return vtk_file;
        return pvtk_file.parent_path() / vtk_file;
    }

    void _make_step_reader() {
        std::filesystem::path path{_steps.at(_step_index).filename};
        if (path.extension() == ".vtu")
            _step_reader = std::make_unique<VTUReader>();
        else if (path.extension() == ".vtp")
            _step_reader = std::make_unique<VTPReader>();
        else if (path.extension() == ".vts")
            _step_reader = std::make_unique<VTSReader>();
        else if (path.extension() == ".vtr")
            _step_reader = std::make_unique<VTRReader>();
        else if (path.extension() == ".vti")
            _step_reader = std::make_unique<VTIReader>();
        else if (path.extension() == ".pvtu")
            _step_reader = std::make_unique<PVTUReader>(_communicator);
        else if (path.extension() == ".pvtp")
            _step_reader = std::make_unique<PVTPReader>(_communicator);
        else if (path.extension() == ".pvts")
            _step_reader = std::make_unique<PVTSReader>(_communicator);
        else if (path.extension() == ".pvtr")
            _step_reader = std::make_unique<PVTRReader>(_communicator);
        else if (path.extension() == ".pvti")
            _step_reader = std::make_unique<PVTIReader>(_communicator);
        else
            throw IOError("Could not find reader for format with extension " + std::string{path.extension()});
        _step_reader->open(path);
    }

    void _read_current_field_names(typename GridReader::FieldNames& names) const {
        std::ranges::copy(cell_field_names(_access_reader()), std::back_inserter(names.cell_fields));
        std::ranges::copy(point_field_names(_access_reader()), std::back_inserter(names.point_fields));
        std::ranges::copy(meta_data_field_names(_access_reader()), std::back_inserter(names.meta_data_fields));
    }

    void _reset() {
        _step_reader.reset();
        _steps.clear();
        _step_index = 0;
    }

    const GridReader& _access_reader() const {
        if (!_step_reader)
            throw ValueError("No data available");
        return *_step_reader;
    }

    C _communicator;
    std::unique_ptr<GridReader> _step_reader;
    std::vector<Step> _steps;
    std::size_t _step_index = 0;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVD_READER_HPP_
