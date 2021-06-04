#include "catchorg/catch/catch.hpp"
#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"

using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::bitmap::writeBitmapFile;
using dansandu::canvas::color::Colors;
using dansandu::canvas::image::Image;

TEST_CASE("Bitmap")
{
    SECTION("chessboard")
    {
        const auto numberOfSquares = 4;
        const auto squareSize = 10;
        auto expectedChessboard = Image{squareSize * numberOfSquares, squareSize * numberOfSquares};
        for (auto x = 0; x < expectedChessboard.width(); ++x)
        {
            for (auto y = 0; y < expectedChessboard.height(); ++y)
            {
                expectedChessboard(x, y) = (x / squareSize + y / squareSize) % 2 ? Colors::white : Colors::turquoise;
            }
        }
        const auto actualChessboard = readBitmapFile("resources/dansandu/canvas/expected_chessboard.bmp");

        REQUIRE(actualChessboard == expectedChessboard);
    }

    SECTION("flower")
    {
        const auto original = readBitmapFile("resources/dansandu/canvas/flower.bmp");

        writeBitmapFile("resources/dansandu/canvas/flower_copy.bmp", original);

        const auto copy = readBitmapFile("resources/dansandu/canvas/flower_copy.bmp");

        REQUIRE(original == copy);
    }
}
