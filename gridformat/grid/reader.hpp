// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \brief Base class for grid data readers.
 */
#ifndef GRIDFORMAT_GRID_READER_HPP_
#define GRIDFORMAT_GRID_READER_HPP_

#include <array>
#include <vector>
#include <utility>
#include <functional>
#include <type_traits>
#include <string_view>
#include <sstream>
#include <string>
#include <span>

#include <gridformat/common/field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/grid/cell_type.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Grid
//! \{

//! Concept for grid factories, to allow for convenient creation of grids from grid files
template<typename T, std::size_t space_dim = 3>
concept GridFactory = requires(T& t) {
    typename T::ctype;
    { t.insert_point(std::declval<const std::array<typename T::ctype, space_dim>&>()) };
    { t.insert_cell(GridFormat::CellType{}, std::declval<const std::vector<std::size_t>&>()) };
};

//! \}  group Grid

}  // namespace Concepts

namespace GridFormat {

//! \addtogroup Grid
//! \{

//! Abstract base class for all readers, defines the common interface.
class GridReader {
 public:
    using Vector = std::array<double, 3>;
    using CellVisitor = std::function<void(CellType, const std::vector<std::size_t>&)>;

    //! Describes the location of a piece within a distributed structured grid
    struct PieceLocation {
        std::array<std::size_t, 3> lower_left;
        std::array<std::size_t, 3> upper_right;
    };

    GridReader() = default;
    explicit GridReader(const std::string& filename) { open(filename); }
    virtual ~GridReader() = default;

    //! Return the name of this reader
    std::string name() const {
        return _name();
    }

    //! Open the given grid file
    void open(const std::string& filename) {
        _filename = filename;
        _field_names.clear();
        _open(filename, _field_names);
    }

    //! Close the grid file
    void close() {
        _close();
        _field_names.clear();
        _filename.clear();
    }

    //! Return the name of the opened grid file (empty string until open() is called)
    const std::string& filename() const {
        return _filename;
    }

    //! Return the number of cells in the grid read from the file
    std::size_t number_of_cells() const {
        return _number_of_cells();
    }

    //! Return the number of points in the grid read from the file
    std::size_t number_of_points() const {
        return _number_of_points();
    }

    //! Return the number of pieces contained in the read file (when constructing readers
    //! in parallel, this reader instance contains the data of only one or some of these pieces)
    std::size_t number_of_pieces() const {
        return _number_of_pieces();
    }

    //! Return the extents of the grid (only available for structured grid formats)
    std::array<std::size_t, 3> extents() const {
        const auto loc = location();
        return {
            loc.upper_right.at(0) - loc.lower_left.at(0),
            loc.upper_right.at(1) - loc.lower_left.at(1),
            loc.upper_right.at(2) - loc.lower_left.at(2)
        };
    }

    //! Return the location of this piece in a structured grid (only available for structured grid formats)
    PieceLocation location() const {
        return _location();
    }

    //! Return the ordinates of the grid (only available for rectilinear grid formats)
    std::vector<double> ordinates(unsigned int direction) const {
        if (direction >= 3)
            throw ValueError("direction must be < 3");
        return _ordinates(direction);
    }

    //! Return the spacing of the grid (only available for image grid formats)
    Vector spacing() const {
        return _spacing();
    }

    //! Return the origin of the grid (only available for image grid formats)
    Vector origin() const {
        return _origin();
    }

    //! Return the basis vector of the grid in the given direction (only available for image grid formats)
    Vector basis_vector(unsigned int direction) const {
        if (direction >= 3)
            throw ValueError("direction must be < 3");
        return _basis_vector(direction);
    }

    //! Return true if the read file is a sequence
    bool is_sequence() const {
        return _is_sequence();
    }

    //! Return the number of available steps (only available for sequence formats)
    std::size_t number_of_steps() const {
        return _number_of_steps();
    }

    //! Return the time at the current step (only available for sequence formats)
    double time_at_step(std::size_t step_idx) const {
        return _time_at_step(step_idx);
    }

    //! Set the step from which to read data (only available for sequence formats)
    void set_step(std::size_t step_idx) {
        _set_step(step_idx, _field_names);
    }

    //! Export the grid read from the file into the given grid factory
    template<std::size_t space_dim = 3, Concepts::GridFactory<space_dim> Factory>
    void export_grid(Factory& factory) const {
        if (number_of_points() > 0) {
            const auto point_field = points();
            const auto point_layout = point_field->layout();
            const auto read_space_dim = point_layout.extent(1);
            const auto copied_space_dim = std::min(space_dim, read_space_dim);
            if (point_layout.extent(0) != number_of_points()) {
                std::ostringstream s; s << point_layout;
                throw SizeError(
                    "Point layout " + s.str() + " does not match number of points: " + std::to_string(number_of_points())
                );
            }

            point_field->visit_field_values([&] <typename T> (std::span<const T> coords) {
                auto p = Ranges::filled_array<space_dim>(typename Factory::ctype{0.0});
                for (std::size_t i = 0; i < point_layout.extent(0); ++i) {
                    for (std::size_t dir = 0; dir < copied_space_dim; ++dir)
                        p[dir] = static_cast<typename Factory::ctype>(coords[i*read_space_dim + dir]);
                    factory.insert_point(std::as_const(p));
                }
            });
        }
        visit_cells([&] (GridFormat::CellType ct, const std::vector<std::size_t>& corners) {
            factory.insert_cell(std::move(ct), corners);
        });
    }

    //! Visit all cells in the grid read from the file
    void visit_cells(const CellVisitor& visitor) const {
        _visit_cells(visitor);
    }

    //! Return the points of the grid as field
    FieldPtr points() const {
        return _points();
    }

    //! Return the cell field with the given name
    FieldPtr cell_field(std::string_view name) const {
        return _cell_field(name);
    }

    //! Return the point field with the given name
    FieldPtr point_field(std::string_view name) const {
        return _point_field(name);
    }

    //! Return the meta data field with the given name
    FieldPtr meta_data_field(std::string_view name) const {
        return _meta_data_field(name);
    }

    //! Return a range over the names of all read cell fields
    friend std::ranges::range auto cell_field_names(const GridReader& reader) {
        return reader._field_names.cell_fields;
    }

    //! Return a range over the names of all read point fields
    friend std::ranges::range auto point_field_names(const GridReader& reader) {
        return reader._field_names.point_fields;
    }

    //! Return a range over the names of all read metadata fields
    friend std::ranges::range auto meta_data_field_names(const GridReader& reader) {
        return reader._field_names.meta_data_fields;
    }

    //! Return a range over name-field pairs for all read cell fields
    friend std::ranges::range auto cell_fields(const GridReader& reader) {
        return reader._field_names.cell_fields | std::views::transform([&] (const std::string& n) {
            return std::make_pair(std::move(n), reader.cell_field(n));
        });
    }

    //! Return a range over name-field pairs for all read point fields
    friend std::ranges::range auto point_fields(const GridReader& reader) {
        return reader._field_names.point_fields | std::views::transform([&] (const std::string& n) {
            return std::make_pair(std::move(n), reader.point_field(n));
        });
    }

    //! Return a range over name-field pairs for all read metadata fields
    friend std::ranges::range auto meta_data_fields(const GridReader& reader) {
        return reader._field_names.meta_data_fields | std::views::transform([&] (std::string n) {
            return std::make_pair(std::move(n), reader.meta_data_field(n));
        });
    }

    void set_ignore_warnings(bool value) {
        _ignore_warnings = value;
    }

 protected:
    struct FieldNames {
        std::vector<std::string> cell_fields;
        std::vector<std::string> point_fields;
        std::vector<std::string> meta_data_fields;

        void clear() {
            cell_fields.clear();
            point_fields.clear();
            meta_data_fields.clear();
        }
    };

    void _log_warning(std::string_view warning) const {
        if (!_ignore_warnings)
            log_warning(
                std::string{warning}
                + (warning.ends_with("\n") ? "" : "\n")
                + "To deactivate this warning, call set_ignore_warnings(true);"
            );
    }

 private:
    std::string _filename = "";
    FieldNames _field_names;
    bool _ignore_warnings = false;

    virtual std::string _name() const = 0;
    virtual void _open(const std::string&, FieldNames&) = 0;
    virtual void _close() = 0;

    virtual std::size_t _number_of_cells() const = 0;
    virtual std::size_t _number_of_points() const = 0;
    virtual std::size_t _number_of_pieces() const = 0;

    virtual FieldPtr _cell_field(std::string_view) const = 0;
    virtual FieldPtr _point_field(std::string_view) const = 0;
    virtual FieldPtr _meta_data_field(std::string_view) const = 0;

    virtual void _visit_cells(const CellVisitor&) const {
        throw NotImplemented("'" + _name() + "' does not implement cell visiting");
    }

    virtual FieldPtr _points() const {
        throw NotImplemented("'" + _name() + "' does not implement points()");
    }

    virtual PieceLocation _location() const {
        throw NotImplemented("Extents/Location are only available with structured grid formats");
    }

    virtual std::vector<double> _ordinates(unsigned int) const {
        throw NotImplemented("Ordinates are only available with rectilinear grid formats.");
    }

    virtual Vector _spacing() const {
        throw NotImplemented("Spacing is only available with image grid formats.");
    }

    virtual Vector _origin() const {
        throw NotImplemented("Origin is only available with image grid formats.");
    }

    virtual Vector _basis_vector(unsigned int i) const {
        std::array<double, 3> result{0., 0., 0.};
        result[i] = 1.0;
        return result;
    }

    virtual bool _is_sequence() const = 0;

    virtual std::size_t _number_of_steps() const {
        throw NotImplemented("The format read by '" + _name() + "' is not a sequence");
    }

    virtual double _time_at_step(std::size_t) const {
        throw NotImplemented("The format read by '" + _name() + "' is not a sequence");
    }

    virtual void _set_step(std::size_t, FieldNames&) {
        throw NotImplemented("The format read by '" + _name() + "' is not a sequence");
    }
};

//! \} group Grid

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_READER_HPP_
