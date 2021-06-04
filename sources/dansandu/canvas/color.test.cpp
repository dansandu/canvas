#include "catchorg/catch/catch.hpp"
#include "dansandu/canvas/color.hpp"

using dansandu::canvas::color::Color;
using dansandu::canvas::color::Colors;

TEST_CASE("Color")
{
    SECTION("code channels")
    {
        auto code = 0xF5654321U;
        auto color = Color{code};

        REQUIRE(color.red() == 0xF5U);
        REQUIRE(color.green() == 0x65U);
        REQUIRE(color.blue() == 0x43U);
        REQUIRE(color.alpha() == 0x21U);
        REQUIRE(color.code() == code);
    }

    SECTION("equality")
    {
        auto red = Color{Colors::red};
        auto green = Color{Colors::green};

        REQUIRE(red == red);

        REQUIRE(red != green);
    }

    SECTION("red channel")
    {
        auto color = Color{Colors::red};

        REQUIRE(color.red() == 255);
        REQUIRE(color.green() == 0);
        REQUIRE(color.blue() == 0);
        REQUIRE(color.code() == 0xFF0000FFu);
    }

    SECTION("green channel")
    {
        auto color = Color{Colors::green};

        REQUIRE(color.red() == 0);
        REQUIRE(color.green() == 255);
        REQUIRE(color.blue() == 0);
        REQUIRE(color.code() == 0x00FF00FFu);
    }

    SECTION("blue channel")
    {
        auto color = Color{Colors::blue};

        REQUIRE(color.red() == 0);
        REQUIRE(color.green() == 0);
        REQUIRE(color.blue() == 255);
        REQUIRE(color.alpha() == 255);
        REQUIRE(color.code() == 0x0000FFFFu);
    }

    SECTION("alpha channel")
    {
        auto color = Color{Colors::black};

        REQUIRE(color.red() == 0);
        REQUIRE(color.green() == 0);
        REQUIRE(color.blue() == 0);
        REQUIRE(color.alpha() == 255);
        REQUIRE(color.code() == 0x000000FFu);
    }

    SECTION("color from code")
    {
        REQUIRE(Colors::magenta == Color{0xFF00FFFFu});
    }

    SECTION("hash")
    {
        auto color = Color{Colors::rust};
        auto actualHash = std::hash<Color>{}(color);
        auto expectedHash = std::hash<uint32_t>{}(static_cast<uint32_t>(Colors::rust));

        REQUIRE(actualHash == expectedHash);
    }
}
