#include "dansandu/canvas/bitmap.hpp"
#include "catchorg/catch/catch.hpp"
#include "dansandu/ballotin/string.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"

using dansandu::ballotin::string::format;
using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::bitmap::writeBitmapFile;
using dansandu::canvas::color::Colors;
using dansandu::canvas::image::Image;

static void REQUIRE_IMAGE(const Image& actualImage, const std::string& fileName)
{
    const auto expectedImagePath = "resources/dansandu/canvas/expected_" + fileName;
    const auto expectedImage = readBitmapFile(expectedImagePath);
    if (actualImage == expectedImage)
    {
        SUCCEED("images match");
    }
    else
    {
        const auto actualImagePath = "target/actual_" + fileName;
        writeBitmapFile(actualImagePath, actualImage);
        FAIL(format("actual image does not match expected image ", expectedImagePath, " -- check ", actualImagePath,
                    " for comparison"));
    }
}

TEST_CASE("bitmap")
{
    SECTION("rgb")
    {
        auto image = Image{2, 3};
        image(0, 0) = Colors::red;
        image(1, 0) = Colors::green;
        image(0, 1) = Colors::blue;
        image(1, 1) = Colors::magenta;
        image(0, 2) = Colors::pink;
        image(1, 2) = Colors::darkGreen;

        REQUIRE_IMAGE(image, "rgb.bmp");
    }

    SECTION("chessboard")
    {
        const auto numberOfSquares = 4;
        const auto squareSize = 10;
        auto image = Image{squareSize * numberOfSquares, squareSize * numberOfSquares};
        for (auto y = 0; y < image.height(); ++y)
        {
            for (auto x = 0; x < image.width(); ++x)
            {
                image(x, y) = (x / squareSize + y / squareSize) % 2 ? Colors::white : Colors::turquoise;
            }
        }

        REQUIRE_IMAGE(image, "chessboard.bmp");
    }

    SECTION("flower")
    {
        const auto expected = readBitmapFile("resources/dansandu/canvas/expected_flower.bmp");

        writeBitmapFile("target/actual_flower.bmp", expected);

        const auto actual = readBitmapFile("target/actual_flower.bmp");

        REQUIRE(expected == actual);
    }
}
