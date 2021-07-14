#include "catchorg/catch/catch.hpp"
#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/canvas/compression.hpp"

using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::compression::testCompress;

TEST_CASE("compression")
{
    const auto original = readBitmapFile("resources/dansandu/canvas/expected_flower.bmp");
    const auto expected = readBitmapFile("resources/dansandu/canvas/expected_compressed.bmp");
    const auto palette = 10;
    const auto iterations = 20;
    const auto actual = testCompress(original, palette, iterations);

    REQUIRE(expected == actual);
}
