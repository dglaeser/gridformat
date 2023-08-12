// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <iterator>
#include <optional>
#include <algorithm>
#include <filesystem>

#include <gridformat/gridformat.hpp>
#include "common.hpp"

std::string wrapped(std::string word, std::string prefix, std::optional<std::string> suffix = {}) {
    auto suffix_str = std::move(suffix).value_or(prefix);
    prefix.reserve(prefix.size() + word.size() + suffix_str.size());
    std::ranges::move(std::move(word), std::back_inserter(prefix));
    std::ranges::move(std::move(suffix_str), std::back_inserter(prefix));
    return prefix;
}

std::string as_cell(std::string word, std::size_t count = 20) {
    if (word.size() < count)
        word.resize(count, ' ');
    return word;
}

template<std::ranges::range R>
void print_fields_info(R&& field_range) {
    std::vector<std::string> names;
    std::vector<std::string> shapes;
    for (const auto& [name, field_ptr] : field_range) {
        names.push_back(name);
        shapes.push_back(GridFormat::as_string(field_ptr->layout()));
    }

    if (names.empty())
        return;

    const auto max_cell_width = std::ranges::max_element(names, [] (const auto& a, const auto& b) {
        return a.size() < b.size();
    })->size();

    for (unsigned int i = 0; i < names.size(); ++i)
        std::cout << " - " << as_cell(wrapped(names[i], "'"), max_cell_width + 3)
                           << " shape=(" << shapes[i] << ")"
                           << std::endl;
    std::cout << " total: " + GridFormat::as_string(names.size()) << std::endl;
}

void print_file_info(const std::string& filename) {
    if (!std::filesystem::exists(filename))
        throw std::runtime_error("File '" + filename + "' does not exist.");

    GridFormat::Reader reader;
    reader.open(filename);

    std::cout << as_cell("Filename:" ) << filename << std::endl;
    std::cout << as_cell("Reader:") << reader.name() << std::endl;
    std::cout << as_cell("Number of cells:")  << reader.number_of_cells() << std::endl;
    std::cout << as_cell("Number of points:") << reader.number_of_points() << std::endl;
    std::cout << as_cell("Number of pieces:") << reader.number_of_pieces() << std::endl;

    std::cout << "Cell fields:" << std::endl;
    print_fields_info(cell_fields(reader));

    std::cout << "Point fields:" << std::endl;
    print_fields_info(point_fields(reader));
}

void print_help() {
    std::cout << "usage: gridformat-info FILE1 FILES...\n" << std::endl;
}

void print_info(int argc, char** argv) {
    if (GridFormat::Apps::args_ask_for_help(argc, argv)) {
        print_help();
        return;
    }

    if (argc < 2) {
        print_help();
        throw std::runtime_error("Invalid number of arguments.");
    }

    std::ranges::for_each(std::views::iota(1, argc), [&] (int arg_idx) {
        print_file_info(argv[arg_idx]);
        if (arg_idx < argc - 1)
            std::cout << std::endl;
    });
}

int main(int argc, char** argv) {
    int ret_code = EXIT_SUCCESS;
    try {
        print_info(argc, argv);
    } catch (const std::exception& e) {
        std::cout << GridFormat::as_error(e.what()) << std::endl;
        ret_code = EXIT_FAILURE;
    }
    return ret_code;
}
