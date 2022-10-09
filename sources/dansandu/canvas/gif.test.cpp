#include "dansandu/canvas/gif.hpp"
#include "catchorg/catch/catch.hpp"
#include "dansandu/ballotin/file_system.hpp"
#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"
#include "dansandu/range/range.hpp"

#include <string_view>
#include <vector>

using dansandu::ballotin::file_system::readBinaryFile;
using dansandu::ballotin::file_system::writeBinaryFile;
using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::color::Color;
using dansandu::canvas::color::Colors;
using dansandu::canvas::gif::getGifBinary;
using dansandu::canvas::gif::lzw;
using dansandu::canvas::gif::writeGifFile;
using dansandu::canvas::image::Image;

using bytes_type = std::vector<uint8_t>;

using namespace dansandu::range::range;

TEST_CASE("gif")
{
    SECTION("lzw")
    {
        SECTION("example 1")
        {
            // +--------+-------+---------+
            // | Symbol | Code  | Decimal |
            // +--------+-------+---------+
            // | A      | 00000 |  0      |
            // | B      | 00001 |  1      |
            // | C      | 00010 |  2      |
            // | D      | 00011 |  3      |
            // | E      | 00100 |  4      |
            // | F      | 00101 |  5      |
            // | G      | 00110 |  6      |
            // | H      | 00111 |  7      |
            // | I      | 01000 |  8      |
            // | J      | 01001 |  9      |
            // | Clear  | 10000 | 16      |
            // | End    | 10001 | 17      |
            // +--------+-------+---------+

            const auto symbols = std::string_view{"AAABEFGAAB"};

            const auto alphabetSize = 10;

            // +----------+-------+---------+------------+
            // | Sequence | Code  | Decimal | Dictionary |
            // +----------+-------+---------+------------+
            // | Clear    | 10000 | 16      |     -      |
            // | A        | 00000 |  0      | 18 : AA    |
            // | AA       | 10010 | 18      | 19 : AAB   |
            // | B        | 00001 |  1      | 20 : BE    |
            // | E        | 00100 |  4      | 21 : EF    |
            // | F        | 00101 |  5      | 22 : FG    |
            // | G        | 00110 |  6      | 23 : GA    |
            // | AAB      | 10011 | 19      |     -      |
            // | End      | 10001 | 17      |     -      |
            // +----------+-------+---------+------------+

            // Output: 000 10000 | 1 10010 00 | 0100 0000 | 10 00101 0 | 10011 001 | 000 10001

            const auto input = symbols | map([](auto c) { return static_cast<int>(c - 'A'); }) | toVector();

            const auto& [output, minimumCodeSize] = lzw(input, alphabetSize);

            const bytes_type expectedOutput = {0b00010000U, 0b11001000U, 0b01000000U,
                                               0b10001010U, 0b10011001U, 0b00010001U};
            const auto expectedMinimumCodeSize = 4;

            REQUIRE(minimumCodeSize == expectedMinimumCodeSize);

            REQUIRE(output == expectedOutput);
        }

        SECTION("example 2")
        {
            const auto input =
                std::vector<int>{{40, 255, 255, 255, 40, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}};
            const auto alphabetSize = 256;

            const auto& [output, minimumCodeSize] = lzw(input, alphabetSize);

            const bytes_type expectedOutput = {0x00, 0x51, 0xFC, 0x1B, 0x28, 0x70, 0xA0, 0xC1, 0x83, 0x01, 0x01};
            const auto expectedMinimumCodeSize = 8;

            REQUIRE(output == expectedOutput);

            REQUIRE(minimumCodeSize == expectedMinimumCodeSize);
        }
    }

    const auto toInt = [](auto value) { return static_cast<int>(value); };

    SECTION("small image")
    {
        // clang-format off
        const auto image = Image{3, 5, {
            Colors::red,   Colors::black, Colors::black,
            Colors::black, Colors::green, Colors::black,
            Colors::black, Colors::black, Colors::blue,
            Colors::black, Colors::black, Colors::black,
            Colors::black, Colors::black, Colors::black,
        }};
        // clang-format on

        const auto expected =
            std::vector<int>{{0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x03, 0x00, 0x05, 0x00, 0x70, 0x00, 0x00, 0x2C, 0x00,
                              0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x81, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0xFF, 0x00, 0x00, 0x00, 0xFF, 0x02, 0x05, 0x44, 0x2E, 0x17, 0xA3, 0x5A, 0x00, 0x3B}};

        const auto binary = getGifBinary(image);
        const auto actual = binary | map(toInt) | toVector();

        REQUIRE(expected == actual);
    }

    SECTION("small animation")
    {
        const auto images = std::vector<Color>{{Colors::red, Colors::green, Colors::blue}} |
                            map(
                                [](const auto color) {
                                    return Image{5, 5, color};
                                }) |
                            toVector();
        const auto frames = images | map([](const auto& f) { return &f; }) | toVector();
        const auto periodCentiseconds = 100;

        const auto expected = std::vector<int>{
            {0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x05, 0x00, 0x05, 0x00, 0x70, 0x00, 0x00, 0x21, 0xFF, 0x0B,
             0x4E, 0x45, 0x54, 0x53, 0x43, 0x41, 0x50, 0x45, 0x32, 0x2E, 0x30, 0x03, 0x01, 0x00, 0x00, 0x00,
             0x21, 0xF9, 0x04, 0x00, 0x64, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05,
             0x00, 0x81, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04,
             0x84, 0x8F, 0xA9, 0x58, 0x00, 0x21, 0xF9, 0x04, 0x00, 0x64, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00,
             0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x81, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x02, 0x04, 0x84, 0x8F, 0xA9, 0x58, 0x00, 0x21, 0xF9, 0x04, 0x00, 0x64, 0x00,
             0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x81, 0x00, 0x00, 0xFF, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x84, 0x8F, 0xA9, 0x58, 0x00, 0x3B}};

        const auto binary = getGifBinary(frames, periodCentiseconds);
        const auto actual = binary | map(toInt) | toVector();

        REQUIRE(expected == actual);
    }

    SECTION("rgb")
    {
        auto image = Image{2, 3};
        image(0, 0) = Colors::red;
        image(1, 0) = Colors::green;
        image(0, 1) = Colors::blue;
        image(1, 1) = Colors::magenta;
        image(0, 2) = Colors::pink;
        image(1, 2) = Colors::darkGreen;

        const auto expected = readBinaryFile("resources/dansandu/canvas/expected_rgb.gif");
        const auto actual = getGifBinary(image);

        if (expected == actual)
        {
            SUCCEED("images match");
        }
        else
        {
            writeBinaryFile("target/actual_rgb.gif", actual);
            FAIL("actual image does not match expected image resources/dansandu/canvas/expected_rgb.gif -- check "
                 "target/actual_rgb.gif for comparison");
        }
    }

    SECTION("large image")
    {
        const auto expected = readBinaryFile("resources/dansandu/canvas/expected_image.gif");
        const auto image = readBitmapFile("resources/dansandu/canvas/expected_flower.bmp");
        const auto actual = getGifBinary(image);

        if (expected != actual)
        {
            writeBinaryFile("target/test_actual_image_space.gif", actual);

            FAIL("image binary do not match -- check target/test_actual_image.gif");
        }
        else
        {
            SUCCEED("image binary match");
        }
    }

    SECTION("large animation")
    {
        const auto expected = readBinaryFile("resources/dansandu/canvas/expected_animation.gif");

        const auto images = std::vector<Image>{{readBitmapFile("resources/dansandu/canvas/frame0.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame1.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame2.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame3.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame4.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame5.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame6.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame7.bmp"),
                                                readBitmapFile("resources/dansandu/canvas/frame8.bmp")}};

        const auto frames = images | map([](const auto& image) { return &image; }) | toVector();
        const auto delayCentiseconds = 20;
        const auto actual = getGifBinary(frames, delayCentiseconds);

        if (expected != actual)
        {
            writeBinaryFile("target/test_actual_animation.gif", actual);

            FAIL("animation binary do not match -- check target/test_actual_animation.gif");
        }
        else
        {
            SUCCEED("animation binary match");
        }
    }
}
