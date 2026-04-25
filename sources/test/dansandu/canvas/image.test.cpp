#include "dansandu/canvas/image.hpp"
#include "dansandu/radiance/radiance.hpp"

#include <cstdint>

using dansandu::canvas::color::Color;
using dansandu::canvas::image::Image;

TEST_CASE("image")
{
    SECTION("empty")
    {
        auto image = Image{};

        REQUIRE_THROW(std::out_of_range, image(0, 0));

        REQUIRE_THROW(std::out_of_range, image.clampedIndex(0, 0));

        REQUIRE(image.width() == 0);

        REQUIRE(image.height() == 0);
    }

    SECTION("indexing")
    {
        auto image = Image{2, 2, {Color::cadet, Color::bronze, Color::coconut, Color::coffee}};

        REQUIRE_THROW(std::out_of_range, image(2, 0));

        REQUIRE_THROW(std::out_of_range, image(0, 2));

        REQUIRE_THROW(std::out_of_range, image(-1, 0));

        REQUIRE_THROW(std::out_of_range, image(0, -1));

        REQUIRE(image(0, 0) == Color::cadet);

        REQUIRE(image(1, 0) == Color::bronze);

        REQUIRE(image(0, 1) == Color::coconut);

        REQUIRE(image(1, 1) == Color::coffee);

        image(1, 0) = Color::magenta;

        REQUIRE(image(1, 0) == Color::magenta);
    }

    SECTION("clamped indexing")
    {
        auto image = Image{2, 2, {Color::cadet, Color::bronze, Color::coconut, Color::coffee}};

        REQUIRE(image.clampedIndex(0, 0) == Color::cadet);

        REQUIRE(image.clampedIndex(1, 0) == Color::bronze);

        REQUIRE(image.clampedIndex(0, 1) == Color::coconut);

        REQUIRE(image.clampedIndex(1, 1) == Color::coffee);

        REQUIRE(image.clampedIndex(2, 0) == Color::bronze);

        REQUIRE(image.clampedIndex(0, 2) == Color::coconut);

        REQUIRE(image.clampedIndex(2, 2) == Color::coffee);

        REQUIRE(image.clampedIndex(-1, 0) == Color::cadet);

        REQUIRE(image.clampedIndex(0, -1) == Color::cadet);
    }

    SECTION("byte array")
    {
        const auto image = Image{2, 2, {Color::cadet, Color::bronze, Color::coconut, Color::coffee}};
        std::vector<uint8_t> actual{image.bytes(), image.bytes() + image.height() * image.width() * 4};
        std::vector<uint8_t> expected = {0x53, 0x68, 0x72, 0xFF, 0xCD, 0x7F, 0x32, 0xFF,
                                         0x96, 0x5A, 0x3E, 0xFF, 0x6F, 0x4E, 0x37, 0xFF};

        REQUIRE(actual == expected);
    }
}
