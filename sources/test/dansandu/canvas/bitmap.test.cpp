#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/ballotin/string.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"
#include "dansandu/journey/exception.hpp"
#include "dansandu/radiance/radiance.hpp"

using dansandu::ballotin::string::format;
using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::bitmap::writeBitmapFile;
using dansandu::canvas::color::Color;
using dansandu::canvas::image::Image;

namespace
{

bool checkBitmap(const Image& actualImage, const std::string& fileName)
{
    const auto expectedImagePath = "resources/test/dansandu/canvas/expected_" + fileName;
    const auto expectedImage = readBitmapFile(expectedImagePath);
    if (actualImage != expectedImage)
    {
        const auto actualImagePath = "target/temporary/actual_" + fileName;
        writeBitmapFile(actualImagePath, actualImage);

        THROW(std::runtime_error, "actual image does not match expected image ", expectedImagePath, " -- check ",
              actualImagePath, " for comparison");
    }

    return true;
}

}

TEST_CASE("bitmap")
{
    SECTION("rgb")
    {
        auto image = Image{2, 3};
        image(0, 0) = Color::red;
        image(1, 0) = Color::green;
        image(0, 1) = Color::blue;
        image(1, 1) = Color::magenta;
        image(0, 2) = Color::pink;
        image(1, 2) = Color::darkGreen;

        REQUIRE(checkBitmap(image, "rgb.bmp"));
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
                image(x, y) = (x / squareSize + y / squareSize) % 2 ? Color::white : Color::turquoise;
            }
        }

        REQUIRE(checkBitmap(image, "chessboard.bmp"));
    }

    SECTION("flower")
    {
        const auto expected = readBitmapFile("resources/test/dansandu/canvas/expected_flower.bmp");

        writeBitmapFile("target/temporary/actual_flower.bmp", expected);

        const auto actual = readBitmapFile("target/temporary/actual_flower.bmp");

        REQUIRE(expected == actual);
    }
}
