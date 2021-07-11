#include "dansandu/canvas/bitmap.hpp"
#include "dansandu/ballotin/exception.hpp"
#include "dansandu/ballotin/file_system.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"

#include <cstdint>

using dansandu::ballotin::file_system::readBinaryFile;
using dansandu::ballotin::file_system::writeBinaryFile;
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
static constexpr auto doubleWordByteCount = 4;
static constexpr auto singleWordByteCount = 2;
static constexpr auto maximumDimension = 1048576U;

static int getPixelArrayPaddingBitCount(const int width)
{
    const auto rowRoundUpBitCount = rowRoundUpByteCount * bitsPerByte;
    const auto rowPaddingBitCount =
        (rowRoundUpBitCount - width * bitsPerPixel % rowRoundUpBitCount) % rowRoundUpBitCount;
    return rowPaddingBitCount;
}

static int getPixelArrayPaddingByteCount(const int width)
{
    return getPixelArrayPaddingBitCount(width) / bitsPerByte;
}

static int getPixelArrayByteCount(const int width, const int height)
{
    const auto rowPaddingBitCount = getPixelArrayPaddingBitCount(width);
    const auto pixelArrayByteCount = height * (width * bitsPerPixel + rowPaddingBitCount) / bitsPerByte;
    return pixelArrayByteCount;
}

void writeBitmapFile(const std::string& path, const Image& image)
{
    const auto pixelArrayByteCount = getPixelArrayByteCount(image.width(), image.height());
    const auto padding = getPixelArrayPaddingByteCount(image.width());

    auto bytes = std::vector<uint8_t>(pixelArrayByteOffset + pixelArrayByteCount, 0);

    const auto littleEndian = [&bytes](const int offset, const int byteCount, const uint32_t value) {
        for (auto index = 0; index < byteCount; ++index)
        {
            bytes[offset + index] = (value >> (index * bitsPerByte)) & 0xFF;
        }
    };

    bytes[0] = firstMagicByte;
    bytes[1] = secondMagicByte;

    littleEndian(0x02, doubleWordByteCount, pixelArrayByteOffset + pixelArrayByteCount);
    littleEndian(0x0A, doubleWordByteCount, pixelArrayByteOffset);
    littleEndian(0x0E, doubleWordByteCount, dibHeaderByteCount);
    littleEndian(0x12, doubleWordByteCount, image.width());
    littleEndian(0x16, doubleWordByteCount, image.height());
    littleEndian(0x1A, singleWordByteCount, colorPlanesCount);
    littleEndian(0x1C, singleWordByteCount, bitsPerPixel);
    littleEndian(0x22, doubleWordByteCount, pixelArrayByteCount);
    littleEndian(0x26, doubleWordByteCount, horizontalPixelsPerMeter);
    littleEndian(0x2A, doubleWordByteCount, verticalPixelsPerMeter);

    auto index = pixelArrayByteOffset;
    for (auto height = 0; height < image.height(); ++height)
    {
        for (auto width = 0; width < image.width(); ++width)
        {
            const auto color = image(width, height);
            bytes[index] = color.blue();
            bytes[index + 1] = color.green();
            bytes[index + 2] = color.red();
            index += 3;
        }
        index += padding;
    }

    writeBinaryFile(path, bytes);
}

Image readBitmapFile(const std::string& path)
{
    const auto binary = readBinaryFile(path);
    const auto binarySize = static_cast<int>(binary.size());

    auto value = uint32_t{0};

    const auto littleEndian = [&binary, &value](const int offset, const int byteCount) {
        value = 0;
        for (auto index = 0; index < byteCount; ++index)
        {
            value <<= bitsPerByte;
            value |= binary[offset + byteCount - index - 1];
        }
    };

    if (binarySize < dibHeaderByteCount)
    {
        THROW(BitmapReadException, "bitmap is missing bytes from the DIB header");
    }

    if (binary[0] != firstMagicByte || binary[1] != secondMagicByte)
    {
        THROW(BitmapReadException, "read magic words ", static_cast<int>(binary[0]), " and ",
              static_cast<int>(binary[1]), " do not match actual magic words ", firstMagicByte, " and ",
              secondMagicByte);
    }

    littleEndian(0x02, doubleWordByteCount);
    if (value != static_cast<uint32_t>(binarySize))
    {
        THROW(BitmapReadException, "read bitmap file size ", value, " does not match actual size ", binarySize);
    }

    littleEndian(0x0A, doubleWordByteCount);
    if (value != pixelArrayByteOffset)
    {
        THROW(BitmapReadException, "read pixel array byte offset ", value, " is not supported -- only ",
              pixelArrayByteOffset, " is supported");
    }

    littleEndian(0x0E, doubleWordByteCount);
    if (value != dibHeaderByteCount)
    {
        THROW(BitmapReadException, "read bitmap DIB header size ", value, " is not supported -- only ",
              dibHeaderByteCount, " is supported");
    }

    littleEndian(0x12, doubleWordByteCount);
    if (value > maximumDimension)
    {
        THROW(BitmapReadException, "read bitmap width ", value, " is larger than the maximum of ", maximumDimension);
    }
    const auto width = static_cast<int>(value);

    littleEndian(0x16, doubleWordByteCount);
    if (value > maximumDimension)
    {
        THROW(BitmapReadException, "read bitmap height ", value, " is larger than the maximum of ", maximumDimension);
    }
    const auto height = static_cast<int>(value);

    littleEndian(0x1A, singleWordByteCount);
    if (value != colorPlanesCount)
    {
        THROW(BitmapReadException, "read bitmap color planes ", value, " is not supported -- only ", colorPlanesCount,
              " is supported");
    }

    littleEndian(0x1C, singleWordByteCount);
    if (value != bitsPerPixel)
    {
        THROW(BitmapReadException, "read bitmap bits per pixel ", value, " is not supported -- only ", bitsPerPixel,
              " is supported");
    }

    littleEndian(0x22, doubleWordByteCount);
    const auto pixelArrayByteCount = getPixelArrayByteCount(width, height);
    if (value != static_cast<uint32_t>(pixelArrayByteCount))
    {
        THROW(BitmapReadException, "read bitmap pixel array size (with padding) ", value,
              " does not match expected size ", pixelArrayByteCount);
    }

    if (binarySize != pixelArrayByteOffset + pixelArrayByteCount)
    {
        THROW(BitmapReadException, "bitmap file size ", binarySize, " does not match expected size ",
              pixelArrayByteOffset + pixelArrayByteCount);
    }

    const auto padding = getPixelArrayPaddingByteCount(width);

    auto index = pixelArrayByteOffset;
    auto image = Image{width, height};
    for (auto i = 0; i < height; i++)
    {
        for (auto j = 0; j < width; j++)
        {
            littleEndian(index, 3);
            (value <<= 8) |= 0xFF;
            image(j, i) = Color{value};
            index += 3;
        }
        index += padding;
    }

    return image;
}

}
