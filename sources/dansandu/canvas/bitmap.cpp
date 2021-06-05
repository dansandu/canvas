#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/ballotin/exception.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"

#include <cstdint>
#include <fstream>

using dansandu::canvas::color::Color;
using dansandu::canvas::image::Image;

namespace dansandu::canvas::bitmap
{

static constexpr auto bitsPerByte = 8;
static constexpr auto firstMagicByte = 0x42;
static constexpr auto secondMagicByte = 0x4D;
static constexpr auto dibHeaderByteCount = 40;
static constexpr auto bitsPerPixel = 24;
static constexpr auto horizontalPixelsPerMeter = 2835;
static constexpr auto verticalPixelsPerMeter = 2835;
static constexpr auto pixelArrayByteOffset = 54;
static constexpr auto rowRoundUpByteCount = 4;
static constexpr auto colorPlanesCount = 1;
static constexpr auto pixelByteCount = 3;

void writeBitmapFile(const std::string& path, const Image& image)
{
    const auto rowRoundUpBitCount = rowRoundUpByteCount * bitsPerByte;
    const auto rowPaddingBitCount =
        (rowRoundUpBitCount - image.width() * bitsPerPixel % rowRoundUpBitCount) % rowRoundUpBitCount;
    const auto pixelArrayByteCount = image.height() * (image.width() * bitsPerPixel + rowPaddingBitCount) / bitsPerByte;

    auto bytes = std::vector<uint8_t>(pixelArrayByteOffset + pixelArrayByteCount, 0);
    bytes[0] = firstMagicByte;
    bytes[1] = secondMagicByte;

    const auto littleEndianDoubleWord = [&bytes](const int offset, const uint32_t value) {
        bytes[offset] = value & 0xFF;
        bytes[offset + 1] = (value >> 8) & 0xFF;
        bytes[offset + 2] = (value >> 16) & 0xFF;
        bytes[offset + 3] = (value >> 24) & 0xFF;
    };

    const auto littleEndianSingleWord = [&bytes](const int offset, const uint32_t value) {
        bytes[offset] = value & 0xFF;
        bytes[offset + 1] = (value >> 8) & 0xFF;
    };

    littleEndianDoubleWord(2, pixelArrayByteOffset + pixelArrayByteCount);
    littleEndianDoubleWord(10, pixelArrayByteOffset);
    littleEndianDoubleWord(14, dibHeaderByteCount);
    littleEndianDoubleWord(18, image.width());
    littleEndianDoubleWord(22, image.height());

    littleEndianSingleWord(26, colorPlanesCount);
    littleEndianSingleWord(28, bitsPerPixel);

    littleEndianDoubleWord(34, pixelArrayByteCount);
    littleEndianDoubleWord(38, horizontalPixelsPerMeter);
    littleEndianDoubleWord(42, verticalPixelsPerMeter);

    auto index = 0;
    for (auto height = 0; height < image.height(); height++)
    {
        for (auto width = 0; width < image.width(); width++)
        {
            const auto color = image(width, height);
            bytes[pixelArrayByteOffset + index] = color.blue();
            bytes[pixelArrayByteOffset + index + 1] = color.green();
            bytes[pixelArrayByteOffset + index + 2] = color.red();
            index += 3;
        }
        index += (rowRoundUpByteCount - index % rowRoundUpByteCount) % rowRoundUpByteCount;
    }

    auto file = std::ofstream{path, std::ios_base::binary};
    file << std::noskipws;
    for (auto byte : bytes)
    {
        if (!(file << byte))
        {
            THROW(std::runtime_error, "could not write bitmap bytes to file");
        }
    }
    file.close();
}

Image readBitmapFile(const std::string& path)
{
    auto file = std::ifstream{path, std::ios_base::binary};
    if (!(file >> std::noskipws))
    {
        THROW(BitmapReadException, "could not read bitmap file '", path, "'");
    }

    const auto littleEndianSingleWord = [&file](const int offset, const char* const errorMessage) {
        constexpr auto count = 2;

        uint8_t buffer[count];
        file.seekg(offset);
        if (!static_cast<bool>(file.read(reinterpret_cast<char*>(buffer), count)))
        {
            THROW(BitmapReadException, errorMessage);
        }

        auto value = static_cast<uint32_t>(buffer[1]);
        (value <<= 8) |= buffer[0];
        return value;
    };

    const auto littleEndianDoubleWord = [&file](const int offset, const char* const errorMessage) {
        constexpr auto count = 4;

        uint8_t buffer[count];
        file.seekg(offset);
        if (!static_cast<bool>(file.read(reinterpret_cast<char*>(buffer), count)))
        {
            THROW(BitmapReadException, errorMessage);
        }

        auto value = static_cast<uint32_t>(buffer[3]);
        (value <<= 8) |= buffer[2];
        (value <<= 8) |= buffer[1];
        (value <<= 8) |= buffer[0];
        return value;
    };

    uint8_t magicWord[2] = {0x00, 0x00};
    file >> magicWord[0] >> magicWord[1];
    if (!file || magicWord[0] != firstMagicByte || magicWord[1] != secondMagicByte)
    {
        THROW(BitmapReadException, "invalid magic words in header -- ", firstMagicByte, " ", secondMagicByte,
              " were expected instead of ", static_cast<int>(magicWord[0]), " ", static_cast<int>(magicWord[1]));
    }

    const auto readPixelArrayByteOffset =
        littleEndianDoubleWord(10, "could not read pixel array byte offset from bitmap header");
    if (readPixelArrayByteOffset != pixelArrayByteOffset)
    {
        THROW(BitmapReadException, "unsupported bitmap pixel array offset of ", readPixelArrayByteOffset, " -- only ",
              pixelArrayByteOffset, " is supported");
    }

    const auto readDibHeaderByteCount =
        littleEndianDoubleWord(14, "could not read DIB header byte count from bitmap header");
    if (readDibHeaderByteCount != dibHeaderByteCount)
    {
        THROW(BitmapReadException, "unsupported bitmap DIB header size of ", readDibHeaderByteCount, " -- only ",
              dibHeaderByteCount, " is supported");
    }

    const auto maximumDimension = 1048576U;

    const auto readWidth = littleEndianDoubleWord(18, "could not read width from bitmap header");
    if (readWidth > maximumDimension)
    {
        THROW(BitmapReadException, "the width ", readWidth, " is larger than the maximum of ", maximumDimension);
    }

    const auto readHeight = littleEndianDoubleWord(22, "could not read height from bitmap header");
    if (readHeight > maximumDimension)
    {
        THROW(BitmapReadException, "the height ", readHeight, " is larger than the maximum of ", maximumDimension);
    }

    const auto readColorPlanesCount =
        littleEndianSingleWord(26, "could not read color planes count from bitmap header");
    if (readColorPlanesCount != colorPlanesCount)
    {
        THROW(BitmapReadException, "unsupported bitmap color planes of ", readColorPlanesCount, " -- only ",
              colorPlanesCount, " is supported");
    }

    const auto readBitsPerPixel = littleEndianSingleWord(28, "could not read bits per pixel from bitmap header");
    if (readBitsPerPixel != bitsPerPixel)
    {
        THROW(BitmapReadException, "unsupported bitmap bits per pixel of ", readBitsPerPixel, " -- only ", bitsPerPixel,
              " is supported");
    }

    const auto readPixelArrayByteCount =
        littleEndianDoubleWord(34, "could not read pixel array byte count from bitmap header");

    const auto width = static_cast<int>(readWidth);
    const auto height = static_cast<int>(readHeight);

    auto image = Image{width, height};
    auto pixelArrayByteCount = 0;

    uint8_t buffer[pixelByteCount + rowRoundUpByteCount];

    file.seekg(pixelArrayByteOffset);
    for (auto i = 0; i < height; i++)
    {
        for (auto j = 0; j < width; j++)
        {
            if (!static_cast<bool>(file.read(reinterpret_cast<char*>(buffer), pixelByteCount)))
            {
                THROW(BitmapReadException, "could not read pixel data from pixel array at position (", j, ", ", i, ")");
            }

            auto code = static_cast<uint32_t>(buffer[2]);
            (code <<= 8) |= buffer[1];
            (code <<= 8) |= buffer[0];
            (code <<= 8) |= 0xFF;
            image(j, i) = Color{code};

            pixelArrayByteCount += 3;
        }
        const auto padding = (rowRoundUpByteCount - pixelArrayByteCount % rowRoundUpByteCount) % rowRoundUpByteCount;
        if (!static_cast<bool>(file.read(reinterpret_cast<char*>(buffer), padding)))
        {
            THROW(BitmapReadException, "could not read padding bytes from pixel array at row ", i);
        }
        pixelArrayByteCount += padding;
    }

    if (readPixelArrayByteCount != static_cast<uint32_t>(pixelArrayByteCount))
    {
        THROW(BitmapReadException, "mismatch between read pixel array size ", readPixelArrayByteCount,
              " and actual size ", pixelArrayByteCount);
    }

    return image;
}

}
