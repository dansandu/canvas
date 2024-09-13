#include "dansandu/canvas/common.test.hpp"
#include "catchorg/catch/catch.hpp"
#include "dansandu/ballotin/string.hpp"
#include "dansandu/canvas/bitmap.hpp"

using dansandu::ballotin::string::format;
using dansandu::canvas::bitmap::readBitmapFile;
using dansandu::canvas::bitmap::writeBitmapFile;
using dansandu::canvas::image::Image;

void requireBitmapImage(const Image& actualImage, const std::string& fileName)
{
    const auto expectedImagePath = "resources/test/dansandu/canvas/expected_" + fileName;
    const auto expectedImage = readBitmapFile(expectedImagePath);
    if (actualImage == expectedImage)
    {
        SUCCEED("images match");
    }
    else
    {
        const auto actualImagePath = "target/temporary/actual_" + fileName;
        writeBitmapFile(actualImagePath, actualImage);
        FAIL(format("actual image does not match expected image ", expectedImagePath, " -- check ", actualImagePath,
                    " for comparison"));
    }
}
