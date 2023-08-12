// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Readers for the VTK HDF file formats.
 */
#ifndef GRIDFORMAT_VTK_HDF_READER_HPP_
#define GRIDFORMAT_VTK_HDF_READER_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <memory>
#include <ranges>
#include <iterator>
#include <type_traits>

#include <gridformat/parallel/communication.hpp>

#include <gridformat/grid/reader.hpp>
#include <gridformat/vtk/hdf_image_grid_reader.hpp>
#include <gridformat/vtk/hdf_unstructured_grid_reader.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Convenience reader for the vtk-hdf file format that supports both the
 *        image & unstructured grid file formats.
 */
template<typename Communicator = GridFormat::NullCommunicator>
class VTKHDFReader : public GridReader {
 public:
    explicit VTKHDFReader() requires (std::is_default_constructible_v<Communicator>) = default;
    explicit VTKHDFReader(const Communicator& comm) : _comm{comm} {}

 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        std::string image_err;
        std::string unstructured_err;
        try {
            VTKHDFImageGridReader reader;
            reader.open(filename);
            _reader = std::make_unique<VTKHDFImageGridReader>(std::move(reader));
        } catch (const Exception& image_e) {
            image_err = image_e.what();
            try {
                using C = Communicator;
                VTKHDFUnstructuredGridReader<C> reader{_comm};
                reader.open(filename);
                _reader = std::make_unique<VTKHDFUnstructuredGridReader<C>>(std::move(reader));
            } catch (const Exception& unstructured_e) {
                unstructured_err = unstructured_e.what();
            }
        }

        if (!_reader)
            throw IOError(
                "Could not open '" + filename + "' as vtk-hdf file.\n" +
                "Error when trying to read as 'ImageData': " + image_err + "\n" +
                "Error when trying to read as 'UnstructuredGrid': " + unstructured_err
            );

        _copy_fields(fields);
    }

    std::string _name() const override {
        if (_reader)
            return _reader->name();
        return "VTKHDFReader";
    }

    void _close() override {
        if (_reader)
            _reader->close();
        _reader.reset();
    }

    void _visit_cells(const CellVisitor& v) const override {
        _access().visit_cells(v);
    }

    FieldPtr _points() const override {
        return _access().points();
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _access().cell_field(name);
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _access().point_field(name);
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _access().meta_data_field(name);
    }

    std::size_t _number_of_cells() const override {
        return _access().number_of_cells();
    }

    std::size_t _number_of_points() const override {
        return _access().number_of_points();
    }

    std::size_t _number_of_pieces() const override {
        return _access().number_of_pieces();
    }

    typename GridReader::PieceLocation _location() const override {
        return _access().location();
    }

    std::vector<double> _ordinates(unsigned int i) const override {
        return _access().ordinates(i);
    }

    std::array<double, 3> _spacing() const override {
        return _access().spacing();
    }

    std::array<double, 3> _origin() const override {
        return _access().origin();
    }

    std::array<double, 3> _basis_vector(unsigned int i) const override {
        return _access().basis_vector(i);
    }

    bool _is_sequence() const override {
        return _access().is_sequence();
    }

    std::size_t _number_of_steps() const override {
        return _access().number_of_steps();
    }

    double _time_at_step(std::size_t step) const override {
        return _access().time_at_step(step);
    }

    void _set_step(std::size_t step, typename GridReader::FieldNames& names) override {
        _access().set_step(step);
        names.clear();
        _copy_fields(names);
    }

    void _copy_fields(typename GridReader::FieldNames& names) {
        std::ranges::copy(cell_field_names(_access()), std::back_inserter(names.cell_fields));
        std::ranges::copy(point_field_names(_access()), std::back_inserter(names.point_fields));
        std::ranges::copy(meta_data_field_names(_access()), std::back_inserter(names.meta_data_fields));
    }

    const GridReader& _access() const {
        if (!_reader)
            throw InvalidState("No active file opened");
        return *_reader;
    }

    GridReader& _access() {
        if (!_reader)
            throw InvalidState("No active file opened");
        return *_reader;
    }

    Communicator _comm;
    std::unique_ptr<GridReader> _reader;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_READER_HPP_
