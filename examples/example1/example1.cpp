// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/gridformat.hpp>

#include "image.hpp"
#include "traits.hpp"

auto generate_test_image() {
    Example::Image my_image(10, 15);
    const Example::Image::Window interior{{3, 8}, {5, 12}};
    std::ranges::for_each(Example::Image::locations_in(interior), [&] (const auto& loc) {
        my_image.set(loc, 1.0);
    });
    return my_image;
}

int main() {
    // Let us check against the concept to see if we implemented the traits correctly
    static_assert(GridFormat::Concepts::ImageGrid<Example::Image>);

    const auto my_image = generate_test_image();

    // we will write a bunch of files. This is a convenience function
    // to add a cell field to a writer and write the file.
    const auto add_data_and_write = [&] (auto& writer, const std::string& filename) {
        writer.set_cell_field("data", [&] (const auto& pixel) {
            return my_image.get(pixel);
        });
        writer.write(filename);
    };

    // First, let gridformat select a suitable default file format for us
    {
        GridFormat::Writer writer{
            GridFormat::default_for(my_image),
            my_image
        };
        add_data_and_write(writer, "my_image_default_format");
    }

    // Let's explicit ask for the .vti image grid format
    {
        GridFormat::Writer writer{
            GridFormat::vti,
            my_image
        };
        add_data_and_write(writer, "my_image_explicit_format");
    }

    // Let's explicitly ask for .vti format with raw encoding
    {
        GridFormat::Writer writer{
            GridFormat::vti({.encoder = GridFormat::Encoding::raw}),
            my_image
        };
        add_data_and_write(writer, "my_image_explicit_encoding");
    }

    // Let's explicitly ask for .vti format without compression
    {
        GridFormat::Writer writer{
            GridFormat::vti({.compressor = GridFormat::none}),
            my_image
        };
        add_data_and_write(writer, "my_image_no_compression");
    }

    return 0;
}
