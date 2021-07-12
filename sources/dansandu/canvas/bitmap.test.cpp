#include "catchorg/catch/catch.hpp"
#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"

using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::bitmap::writeBitmapFile;
using dansandu::canvas::color::Colors;
using dansandu::canvas::image::Image;

TEST_CASE("bitmap")
{
    SECTION("chessboard")
    {
        const auto numberOfSquares = 4;
        const auto squareSize = 10;
        auto actual = Image{squareSize * numberOfSquares, squareSize * numberOfSquares};
        for (auto y = 0; y < actual.height(); ++y)
        {
            for (auto x = 0; x < actual.width(); ++x)
            {
                actual(x, y) = (x / squareSize + y / squareSize) % 2 ? Colors::white : Colors::turquoise;
            }
        }

        const auto expected = readBitmapFile("resources/dansandu/canvas/expected_chessboard.bmp");

        REQUIRE(expected == actual);
    }

    SECTION("flower")
    {
        const auto expected = readBitmapFile("resources/dansandu/canvas/expected_flower.bmp");

        writeBitmapFile("resources/dansandu/canvas/actual_flower.bmp", expected);

        const auto actual = readBitmapFile("resources/dansandu/canvas/actual_flower.bmp");

        REQUIRE(expected == actual);
    }
}
