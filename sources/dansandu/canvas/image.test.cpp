#include "catchorg/catch/catch.hpp"
#include "dansandu/canvas/image.hpp"

#include <cstdint>

using dansandu::canvas::color::Color;
using dansandu::canvas::color::Colors;
using dansandu::canvas::image::Image;

TEST_CASE("image")
{
    SECTION("empty")
    {
        auto image = Image{};

        REQUIRE(image.width() == 0);
        REQUIRE(image.height() == 0);
    }

    SECTION("solid")
    {
        auto image = Image{10, 20, Colors::fuchsia};

        SECTION("indexing within bounds")
        {
            image(5, 5) = Colors::magenta;

            REQUIRE(image(5, 5) == Colors::magenta);
        }

        SECTION("indexing outside bounds")
        {
            REQUIRE_THROWS_AS(image(10, 20), std::out_of_range);
            REQUIRE_THROWS_AS(image(15, 10), std::out_of_range);
        }
    }

    SECTION("byte array")
    {
        const auto image = Image{2, 2, {Colors::cadet, Colors::bronze, Colors::coconut, Colors::coffee}};
        std::vector<uint8_t> actual{image.bytes(), image.bytes() + image.height() * image.width() * 4};
        std::vector<uint8_t> expected = {0x53, 0x68, 0x72, 0xFF, 0xCD, 0x7F, 0x32, 0xFF,
                                         0x96, 0x5A, 0x3E, 0xFF, 0x6F, 0x4E, 0x37, 0xFF};

        REQUIRE(actual == expected);
    }
}
