// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <exception>
#include <utility>
#include <string>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/grid/reader.hpp>

#include "../testing.hpp"

class TestReaderException : public std::exception {
 public:
    TestReaderException(std::string m)
    : _msg{std::move(m)}
    {}

    virtual const char* what() const noexcept override {
        return _msg.data();
    }

 private:
    std::string _msg;
};

class TestReader : public GridFormat::GridReader {
    using ParentType = GridFormat::GridReader;

 private:
    virtual std::string _name() const { return "TestReader"; }
    virtual void _open(const std::string&, FieldNames&) {}
    virtual void _close() {}

    virtual std::size_t _number_of_cells() const { throw TestReaderException("number_of_cells"); }
    virtual std::size_t _number_of_points() const { throw TestReaderException("number_of_points"); }
    virtual std::size_t _number_of_pieces() const { throw TestReaderException("number_of_pieces"); }

    virtual GridFormat::FieldPtr _cell_field(std::string_view) const { throw TestReaderException("cell_field"); }
    virtual GridFormat::FieldPtr _point_field(std::string_view) const { throw TestReaderException("point_field"); }
    virtual GridFormat::FieldPtr _meta_data_field(std::string_view) const { throw TestReaderException("meta_data_field"); }

    virtual bool _is_sequence() const { throw TestReaderException("is_sequence"); }
};

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::expect;

    TestReader reader;

    "reader_visit_cells_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () {
            reader.visit_cells([] (const auto&, const auto&) {});
        }));
    };

    "reader_points_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.points(); }));
    };

    "reader_location_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.location(); }));
    };

    "reader_ordinates_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.ordinates(0); }));
    };

    "reader_ordinates_throws_for_dim_over_2"_test = [&] () {
        expect(throws<GridFormat::ValueError>([&] () { reader.ordinates(3); }));
    };

    "reader_spacing_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.spacing(); }));
    };

    "reader_origin_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.origin(); }));
    };

    "reader_num_steps_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.number_of_steps(); }));
    };

    "reader_time_at_step_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.time_at_step(0); }));
    };

    "reader_set_step_throws_per_default"_test = [&] () {
        expect(throws<GridFormat::NotImplemented>([&] () { reader.set_step(0); }));
    };

    return 0;
}
