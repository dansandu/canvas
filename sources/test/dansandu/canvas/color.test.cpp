#include "dansandu/canvas/color.hpp"
#include "dansandu/radiance/radiance.hpp"

#include <sstream>

using dansandu::canvas::color::Color;

TEST_CASE("color")
{
    SECTION("code channels")
    {
        const auto code = 0xF5654321u;
        const auto color = Color{code};

        REQUIRE(color.getRedChannel() == 0xF5u);
        REQUIRE(color.getGreenChannel() == 0x65u);
        REQUIRE(color.getBlueChannel() == 0x43u);
        REQUIRE(color.getAlphaChannel() == 0x21u);
        REQUIRE(color.getCode() == code);
    }

    SECTION("equality")
    {
        const Color red = Color::red;

        const Color green = Color::green;

        REQUIRE(red == red);

        REQUIRE(red != green);
    }

    SECTION("red channel")
    {
        const Color color = Color::red;

        REQUIRE(color.getRedChannel() == 255);
        REQUIRE(color.getGreenChannel() == 0);
        REQUIRE(color.getBlueChannel() == 0);
        REQUIRE(color.getCode() == 0xFF0000FFu);
    }

    SECTION("green channel")
    {
        const Color color = Color::green;

        REQUIRE(color.getRedChannel() == 0);
        REQUIRE(color.getGreenChannel() == 0xFFu);
        REQUIRE(color.getBlueChannel() == 0);
        REQUIRE(color.getAlphaChannel() == 0xFFu);
        REQUIRE(color.getCode() == 0x00FF00FFu);
    }

    SECTION("blue channel")
    {
        const Color color = Color::blue;

        REQUIRE(color.getRedChannel() == 0);
        REQUIRE(color.getGreenChannel() == 0);
        REQUIRE(color.getBlueChannel() == 0xFFu);
        REQUIRE(color.getAlphaChannel() == 0xFFu);
        REQUIRE(color.getCode() == 0x0000FFFFu);
    }

    SECTION("alpha channel")
    {
        const Color color = Color::black;

        REQUIRE(color.getRedChannel() == 0);

        REQUIRE(color.getGreenChannel() == 0);

        REQUIRE(color.getBlueChannel() == 0);

        REQUIRE(color.getAlphaChannel() == 0xFFu);

        REQUIRE(color.getCode() == 0x000000FFu);
    }

    SECTION("color from code")
    {
        const Color color = Color::magenta;

        REQUIRE(color == Color{0xFF00FFFFu});
    }

    SECTION("hash")
    {
        const Color color = Color::rust;

        const auto actualHash = std::hash<Color>{}(color);

        const auto expectedHash = std::hash<uint32_t>{}(color.getCode());

        REQUIRE(actualHash == expectedHash);
    }

    SECTION("string")
    {
        const Color color = Color::khaki;

        REQUIRE(color.toString() == "#C3B091FF");
    }
}
