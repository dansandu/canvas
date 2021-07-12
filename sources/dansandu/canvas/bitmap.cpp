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

    const auto write = [&bytes](const int offset, const int byteCount, const uint32_t value) {
        for (auto index = 0; index < byteCount; ++index)
        {
            bytes[offset + index] = (value >> (index * bitsPerByte)) & 0xFF;
        }
    };

    bytes[0] = firstMagicByte;
    bytes[1] = secondMagicByte;

    write(0x02, 4, pixelArrayByteOffset + pixelArrayByteCount);
    write(0x0A, 4, pixelArrayByteOffset);
    write(0x0E, 4, dibHeaderByteCount);
    write(0x12, 4, image.width());
    write(0x16, 4, image.height());
    write(0x1A, 2, colorPlanesCount);
    write(0x1C, 2, bitsPerPixel);
    write(0x22, 4, pixelArrayByteCount);
    write(0x26, 4, horizontalPixelsPerMeter);
    write(0x2A, 4, verticalPixelsPerMeter);

    auto index = pixelArrayByteOffset;
    for (auto h = 0; h < image.height(); ++h)
    {
        for (auto w = 0; w < image.width(); ++w)
        {
            const auto color = image(w, image.height() - h - 1);
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

    const auto read = [&binary](const int offset, const int byteCount, uint32_t& value) {
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

    auto value = uint32_t{0};

    read(0x02, 4, value);
    if (value != static_cast<uint32_t>(binarySize))
    {
        THROW(BitmapReadException, "read bitmap file size ", value, " does not match actual size ", binarySize);
    }

    read(0x0A, 4, value);
    if (value != pixelArrayByteOffset)
    {
        THROW(BitmapReadException, "read pixel array byte offset ", value, " is not supported -- only ",
              pixelArrayByteOffset, " is supported");
    }

    read(0x0E, 4, value);
    if (value != dibHeaderByteCount)
    {
        THROW(BitmapReadException, "read bitmap DIB header size ", value, " is not supported -- only ",
              dibHeaderByteCount, " is supported");
    }

    read(0x12, 4, value);
    if (value > maximumDimension)
    {
        THROW(BitmapReadException, "read bitmap width ", value, " is larger than the maximum of ", maximumDimension);
    }
    const auto width = static_cast<int>(value);

    read(0x16, 4, value);
    if (value > maximumDimension)
    {
        THROW(BitmapReadException, "read bitmap height ", value, " is larger than the maximum of ", maximumDimension);
    }
    const auto height = static_cast<int>(value);

    read(0x1A, 2, value);
    if (value != colorPlanesCount)
    {
        THROW(BitmapReadException, "read bitmap color planes ", value, " is not supported -- only ", colorPlanesCount,
              " is supported");
    }

    read(0x1C, 2, value);
    if (value != bitsPerPixel)
    {
        THROW(BitmapReadException, "read bitmap bits per pixel ", value, " is not supported -- only ", bitsPerPixel,
              " is supported");
    }

    const auto pixelArrayByteCount = getPixelArrayByteCount(width, height);
    read(0x22, 4, value);
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
    for (auto h = 0; h < height; ++h)
    {
        for (auto w = 0; w < width; ++w)
        {
            read(index, 3, value);
            (value <<= 8) |= 0xFF;
            image(w, height - h - 1) = Color{value};
            index += 3;
        }
        index += padding;
    }

    return image;
}

}
