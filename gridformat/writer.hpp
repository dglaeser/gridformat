// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup API
 * \brief A generic writer providing access to the writers for all supported formats.
 */
#ifndef GRIDFORMAT_WRITER_HPP_
#define GRIDFORMAT_WRITER_HPP_

#include <string>
#include <memory>
#include <utility>
#include <functional>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/parallel/concepts.hpp>

namespace GridFormat {

template<typename FileFormat>
struct WriterFactory;


#ifndef DOXYGEN
namespace WriterDetail {

    template<typename FileFormat, typename Grid>
    static constexpr bool has_sequential_factory
        = is_complete<WriterFactory<FileFormat>>
        and Concepts::Grid<Grid>
        and requires(const FileFormat& f, const Grid& grid) {
            { WriterFactory<FileFormat>::make(f, grid) } -> std::derived_from<GridWriter<Grid>>;
        };

    template<typename FileFormat, typename Grid>
    static constexpr bool has_sequential_time_series_factory
        = is_complete<WriterFactory<FileFormat>>
        and Concepts::Grid<Grid>
        and requires(const FileFormat& f, const Grid& grid) {
            { WriterFactory<FileFormat>::make(f, grid, std::string{}) } -> std::derived_from<TimeSeriesGridWriter<Grid>>;
        };

    template<typename FileFormat, typename Grid, typename Comm>
    static constexpr bool has_parallel_factory
        = is_complete<WriterFactory<FileFormat>>
        and Concepts::Grid<Grid>
        and Concepts::Communicator<Comm>
        and requires(const FileFormat& f, const Grid& grid, const Comm& comm) {
            { WriterFactory<FileFormat>::make(f, grid, comm) } -> std::derived_from<GridWriter<Grid>>;
        };

    template<typename FileFormat, typename Grid, typename Comm>
    static constexpr bool has_parallel_time_series_factory
        = is_complete<WriterFactory<FileFormat>>
        and Concepts::Grid<Grid>
        and Concepts::Communicator<Comm>
        and requires(const FileFormat& f, const Grid& grid, const Comm& comm) {
            { WriterFactory<FileFormat>::make(f, grid, comm, std::string{}) } -> std::derived_from<TimeSeriesGridWriter<Grid>>;
        };

    template<typename _Grid, typename Grid>
    concept Compatible = std::same_as<std::remove_cvref_t<_Grid>, Grid> and std::is_lvalue_reference_v<_Grid>;

}  // namespace WriterDetail
#endif  // DOXYGEN


/*!
 * \ingroup API
 * \brief Interface to the writers for all supported file formats.
 *        Depending on the chosen format, this exposes the interface of
 *        grid file or time series writers.
 * \details Typically you would construct this class with one of the predefined
 *          file format instances. For example, with the .vtu file format:
 *          \code{.cpp}
 *            GridFormat::Writer writer{GridFormat::vtu, grid};
 *          \endcode
 * \note The constructor checks that the grid you are passing in is
 *       actually an lvalue-reference. If not, compilation will fail.
 *       This is because all writers take the grid per reference and
 *       their lifetime is bound to the lifetime of the given grid.
 * \tparam Grid The type of grid which should be written out.
 */
template<Concepts::Grid G>
class Writer {
 public:
    using Grid = G;

    /*!
     * \brief Construct a sequential grid file writer.
     * \param f The file format which should be written.
     * \param grid The grid which should be written out.
     */
    template<typename FileFormat, WriterDetail::Compatible<Grid> _Grid>
        requires(WriterDetail::has_sequential_factory<FileFormat, Grid>)
    Writer(const FileFormat& f, _Grid&& grid)
    : Writer(WriterFactory<FileFormat>::make(f, grid))
    {}

    /*!
     * \brief Construct a sequential time series writer.
     * \param f The file format which should be written.
     * \param grid The grid which should be written out.
     * \param base_filename The name of the file (without extension) into which to write.
     */
    template<typename FileFormat, WriterDetail::Compatible<Grid> _Grid>
        requires(WriterDetail::has_sequential_time_series_factory<FileFormat, Grid>)
    Writer(const FileFormat& f, _Grid&& grid, const std::string& base_filename)
    : Writer(WriterFactory<FileFormat>::make(f, grid, base_filename))
    {}

    /*!
     * \brief Construct a parallel grid file writer.
     * \param f The file format which should be written.
     * \param grid The grid which should be written out.
     * \param comm The communicator for parallel communication.
     */
    template<typename FileFormat, WriterDetail::Compatible<Grid> _Grid, Concepts::Communicator Comm>
        requires(WriterDetail::has_parallel_factory<FileFormat, Grid, Comm>)
    Writer(const FileFormat& f, _Grid&& grid, const Comm& comm)
    : Writer(WriterFactory<FileFormat>::make(f, grid, comm))
    {}

    /*!
     * \brief Construct a parallel time series file writer.
     * \param f The file format which should be written.
     * \param grid The grid which should be written out.
     * \param comm The communicator for parallel communication.
     * \param base_filename The name of the file (without extension) into which to write.
     */
    template<typename FileFormat, WriterDetail::Compatible<Grid> _Grid, Concepts::Communicator Comm>
        requires(WriterDetail::has_parallel_time_series_factory<FileFormat, Grid, Comm>)
    Writer(const FileFormat& f, _Grid&& grid, const Comm& comm, const std::string& base_filename)
    : Writer(WriterFactory<FileFormat>::make(f, grid, comm, base_filename))
    {}

    //! Construct a grid file writer from a writer implementation
    template<std::derived_from<GridWriter<Grid>> W>
        requires(!std::is_lvalue_reference_v<W>)
    explicit Writer(W&& writer)
    : _writer(std::make_unique<W>(std::move(writer)))
    {}

    //! Construct a time series file writer from a writer implementation
    template<std::derived_from<TimeSeriesGridWriter<Grid>> W>
        requires(!std::is_lvalue_reference_v<W>)
    explicit Writer(W&& writer)
    : _time_series_writer(std::make_unique<W>(std::move(writer)))
    {}

    /*!
     * \brief Write the grid and data to a file.
     * \param filename The name of file into which to write (without extension).
     * \note Calling this function is only allowed if the writer was created as
     *       a grid file writer. If this instance is a time series writer, calling
     *       this function will throw an exception.
     */
    std::string write(const std::string& filename) const {
        if (!_writer)
            throw InvalidState(
                "Writer was constructed as a time series writer. Only write(Scalar) can be used."
            );
        return _writer->write(filename);
    }

    /*!
     * \brief Write a time step in a time series.
     * \param time_value The time corresponding to this time step.
     * \note Calling this function is only allowed if the writer was created as
     *       a time series file writer. If this instance is a grid file writer,
     *       calling this function will throw an exception.
     */
    template<Concepts::Scalar T>
    std::string write(const T& time_value) const {
        if (!_time_series_writer)
            throw InvalidState(
                "Writer was not constructed as a time series writer. Only write(std::string) can be used."
            );
        return _time_series_writer->write(time_value);
    }

    /*!
     * \brief Set a meta data field to be added to the output.
     * \param name The name of the meta data field.
     * \param field The actual meta data.
     * \note Supported metadata are scalar values, strings, or ranges of scalars.
     */
    template<typename Field>
    void set_meta_data(const std::string& name, Field&& field) {
        _visit_writer([&] (auto& writer) {
            writer.set_meta_data(name, std::forward<Field>(field));
        });
    }

    /*!
     * \brief Set a point data field to be added to the output.
     * \param name The name of the point data field.
     * \param field The actual point data.
     * \note Point data is usually given as lambdas that are invocable
     *       with points of the grid. You can also pass in custom fields
     *       that inherit from the `Field` class. This is discouraged,
     *       however.
     */
    template<typename Field>
    void set_point_field(const std::string& name, Field&& field) {
        _visit_writer([&] (auto& writer) {
            writer.set_point_field(name, std::forward<Field>(field));
        });
    }

    /*!
     * \brief Overload with custom precision with which to write the field.
     * \note Can be used to save space on disk and increase the write speed
     *       if you know that your field can be represented sufficiently well
     *       by a smaller precision.
     */
    template<typename Field, typename T>
    void set_point_field(const std::string& name, Field&& field, const Precision<T>& prec) {
        _visit_writer([&] (auto& writer) {
            writer.set_point_field(name, std::forward<Field>(field), prec);
        });
    }

    /*!
     * \brief Set a cell data field to be added to the output.
     * \param name The name of the cell data field.
     * \param field The actual cell data.
     * \note Cell data is usually given as lambdas that are invocable
     *       with cells of the grid. You can also pass in custom fields
     *       that inherit from the `Field` class. This is discouraged,
     *       however.
     */
    template<typename Field>
    void set_cell_field(const std::string& name, Field&& field) {
        _visit_writer([&] (auto& writer) {
            writer.set_cell_field(name, std::forward<Field>(field));
        });
    }

    /*!
     * \brief Overload with custom precision with which to write the field.
     * \note Can be used to save space on disk and increase the write speed
     *       if you know that your field can be represented sufficiently well
     *       by a smaller precision.
     */
    template<typename Field, typename T>
    void set_cell_field(const std::string& name, Field&& field, const Precision<T>& prec) {
        _visit_writer([&] (auto& writer) {
            writer.set_cell_field(name, std::forward<Field>(field), prec);
        });
    }

    /*!
     * \brief Remove a meta data field from the output.
     * \param name The name of the meta data field.
     */
    FieldPtr remove_meta_data(const std::string& name) {
        return _visit_writer([&] (auto& writer) {
            return writer.remove_meta_data(name);
        });
    }

    /*!
     * \brief Remove a point field from the output.
     * \param name The name of the point field.
     */
    FieldPtr remove_point_field(const std::string& name) {
        return _visit_writer([&] (auto& writer) {
            return writer.remove_point_field(name);
        });
    }

    /*!
     * \brief Remove a cell field from the output.
     * \param name The name of the cell field.
     */
    FieldPtr remove_cell_field(const std::string& name) {
        return _visit_writer([&] (auto& writer) {
            return writer.remove_cell_field(name);
        });
    }

    //! Remove all data inserted to the writer.
    void clear() {
        _visit_writer([&] (auto& writer) {
            writer.clear();
        });
    }

    //! Ignore/consider warnings (default: true)
    void set_ignore_warnings(bool value) {
        _visit_writer([&] (auto& writer) {
            writer.set_ignore_warnings(value);
        });
    }

    /*!
     * \brief Copy all inserted fields into another writer.
     * \param out The writer into which to copy all fields of this writer.
     */
    template<typename Writer>
    void copy_fields(Writer& out) const {
        _visit_writer([&] (const auto& writer) {
            writer.copy_fields(out);
        });
    }

    /*!
     * \brief Return the basic options used by this writer.
     * \note This is used internally and not be required by users.
     */
    const std::optional<WriterOptions>& writer_options() const {
        return _visit_writer([&] (const auto& writer) -> const std::optional<WriterOptions>& {
            return writer.writer_options();
        });
    }

    /*!
     * \brief Return a reference to the underlying grid.
     */
    const Grid& grid() const {
        return _visit_writer([&] (const auto& writer) -> const Grid& {
            return writer.grid();
        });
    }

    /*!
     * \brief Return a range over all point fields that were added to the given writer.
     * \param w The writer whose fields to return
     * \return A range over key-value pairs containing the name and a pointer to actual field.
     * \note You can use range-based for loops with structured bindings, for instance:
     * \code{.cpp}
     *  for (const auto& [name, field_ptr] : point_fields(writer)) { ... }
     * \endcode
     */
    friend decltype(auto) point_fields(const Writer& w) {
        return w._visit_writer([&] (const auto& writer) {
            return point_fields(writer);
        });
    }

    /*!
     * \brief Return a range over all cell fields that were added to the given writer.
     * \param w The writer whose fields to return
     * \return A range over key-value pairs containing the name and a pointer to actual field.
     * \note You can use range-based for loops with structured bindings, for instance:
     * \code{.cpp}
     *  for (const auto& [name, field_ptr] : cell_fields(writer)) { ... }
     * \endcode
     */
    friend decltype(auto) cell_fields(const Writer& w) {
        return w._visit_writer([&] (const auto& writer) {
            return cell_fields(writer);
        });
    }

    /*!
     * \brief Return a range over all meta data fields that were added to the given writer.
     * \param w The writer whose fields to return
     * \return A range over key-value pairs containing the name and a pointer to actual field.
     * \note You can use range-based for loops with structured bindings, for instance:
     * \code{.cpp}
     *  for (const auto& [name, field_ptr] : meta_data_fields(writer)) { ... }
     * \endcode
     */
    friend decltype(auto) meta_data_fields(const Writer& w) {
        return w._visit_writer([&] (const auto& writer) {
            return meta_data_fields(writer);
        });
    }

 private:
    template<typename Visitor>
    decltype(auto) _visit_writer(const Visitor& visitor) {
        if (_writer)
            return std::invoke(visitor, *_writer);
        else {
            if (!_time_series_writer)
                throw InvalidState("No writer set");
            return std::invoke(visitor, *_time_series_writer);
        }
    }

    template<typename Visitor>
    decltype(auto) _visit_writer(const Visitor& visitor) const {
        if (_writer)
            return std::invoke(visitor, *_writer);
        else {
            if (!_time_series_writer)
                throw InvalidState("No writer set");
            return std::invoke(visitor, *_time_series_writer);
        }
    }

    std::unique_ptr<GridWriter<Grid>> _writer{nullptr};
    std::unique_ptr<TimeSeriesGridWriter<Grid>> _time_series_writer{nullptr};
};

template<typename F, typename G>
    requires(Concepts::Grid<std::remove_cvref_t<G>>)
Writer(const F&, G&&) -> Writer<std::remove_cvref_t<G>>;

template<typename F, typename G, Concepts::Communicator C>
    requires(Concepts::Grid<std::remove_cvref_t<G>>)
Writer(const F&, G&&, const C&) -> Writer<std::remove_cvref_t<G>>;

template<typename F, typename G>
    requires(Concepts::Grid<std::remove_cvref_t<G>>)
Writer(const F&, G&&, const std::string&) -> Writer<std::remove_cvref_t<G>>;

template<typename F, typename G, Concepts::Communicator C>
    requires(Concepts::Grid<std::remove_cvref_t<G>>)
Writer(const F&, G&&, const C&, const std::string&) -> Writer<std::remove_cvref_t<G>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_WRITER_HPP_
