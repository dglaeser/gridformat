// SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <concepts>
#include <string_view>
#include <algorithm>
#include <fstream>
#include <vector>

namespace GridFormat::Benchmark {

struct Result {
    std::string name;
    std::vector<double> measurements;
};

template<std::invocable F>
double measure(const F& action) {
    const auto t0 = std::chrono::steady_clock::now();
    action();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(t1-t0).count();
}

template<typename Writer>
std::vector<double> measure_writer(const Writer& writer,
                                   const std::string_view name,
                                   int num_repetitions = 5) {
    std::cout << "Measuring writer output ('" << name << "')" << std::endl;
    std::string filename = "benchmark_vtu_tmp";

    std::vector<double> results;
    for (int i = 0; i < num_repetitions; ++i) {
        std::filesystem::remove(filename);
        results.push_back(GridFormat::Benchmark::measure([&] () {
            writer.write(filename);
        }));
        std::cout << " -- run " << i << ": " << results.back() << "s" << std::endl;
    }

    std::filesystem::remove(filename);
    return results;
}

bool write_results_to(const std::string& filename, const std::vector<Result>& results) {
    if (results.empty())
        return false;

    std::cout << "Writing results to '" << filename << "'" << std::endl;
    std::ofstream out_file(filename, std::ios::out);
    const auto write_header = [&] () {
        out_file << "i";
        std::ranges::for_each(results, [&] (const auto& result) {
            out_file << "," << result.name;
        });
        out_file << "\n";
    };

    const auto write_row = [&] (unsigned int i) {
        out_file << i;
        std::ranges::for_each(results, [&] (const auto& result) {
            out_file << "," << (
                result.measurements.size() > i
                    ? GridFormat::as_string(result.measurements[i])
                    : "-"
            );
        });
        out_file << "\n";
    };

    write_header();
    const auto max_repetitions = std::max_element(results.begin(), results.end(), [] (const auto& a, const auto& b) {
        return a.measurements.size() < b.measurements.size();
    })->measurements.size();
    for (unsigned int i = 0; i < max_repetitions; ++i)
        write_row(i);

    return true;
}

}  // namespace GridFormat::Benchmark
