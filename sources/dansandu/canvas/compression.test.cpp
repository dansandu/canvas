#include "catchorg/catch/catch.hpp"
#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/canvas/compression.hpp"

using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::bitmap::writeBitmapFile;
using dansandu::canvas::compression::compress;

TEST_CASE("compression")
{
    const auto original = readBitmapFile("resources/dansandu/canvas/expected_flower.bmp");
    const auto palette = 10;
    const auto iterations = 20;
    const auto compressed = compress(original, palette, iterations);
    writeBitmapFile("resources/dansandu/canvas/expected_compressed_flower.bmp", compressed);
}
