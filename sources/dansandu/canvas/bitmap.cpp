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
static constexpr auto paddingByteCount = 4;
static constexpr auto colorPlanesCount = 1;

void writeBitmapFile(const std::string& path, const Image& image)
{
    const auto paddingBitCount = paddingByteCount * bitsPerByte;
    const auto rowPaddingBitCount =
        (paddingBitCount - image.width() * bitsPerPixel % paddingBitCount) % paddingBitCount;
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
        index += (paddingByteCount - index % paddingByteCount) % paddingByteCount;
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

class BitmapFileReader
{
public:
    explicit BitmapFileReader(const std::string& path) : file_{path, std::ios_base::binary}, currentSize_{0}
    {
        file_ >> std::noskipws;
        if (!file_)
        {
            THROW(BitmapReadException, "file '", path, "' does not exist");
        }
    }

    void read(const int size, const char* const errorMessage)
    {
        currentSize_ = 0;

        if (size < 0 || size > bufferSize)
        {
            THROW(BitmapReadException, "the ", size, " bytes read exceeded the buffer size of ", bufferSize);
        }

        if (!static_cast<bool>(file_.read(reinterpret_cast<char*>(buffer_), size)))
        {
            THROW(BitmapReadException, errorMessage);
        }

        currentSize_ = size;
    }

    uint32_t readLittleEndianSingleWord(const char* const errorMessage)
    {
        read(2, errorMessage);
        auto value = static_cast<uint32_t>(buffer_[1]);
        (value <<= 8) |= buffer_[0];
        return value;
    }

    uint32_t readLittleEndianDoubleWord(const char* const errorMessage)
    {
        read(4, errorMessage);
        auto value = static_cast<uint32_t>(buffer_[3]);
        (value <<= 8) |= buffer_[2];
        (value <<= 8) |= buffer_[1];
        (value <<= 8) |= buffer_[0];
        return value;
    }

    Color readLittleEndianColor(const char* const errorMessage)
    {
        read(3, errorMessage);
        auto colorCode = static_cast<uint32_t>(buffer_[2]);
        (colorCode <<= 8) |= buffer_[1];
        (colorCode <<= 8) |= buffer_[0];
        (colorCode <<= 8) |= 0xFF;
        return Color{colorCode};
    }

    uint8_t operator[](int index) const
    {
        if (index < 0 || index >= currentSize_)
        {
            THROW(BitmapReadException, "cannot index into unread bytes");
        }
        return buffer_[index];
    }

private:
    static constexpr auto bufferSize = 32;

    std::ifstream file_;
    uint8_t buffer_[bufferSize];
    int currentSize_;
};

Image readBitmapFile(const std::string& path)
{
    auto reader = BitmapFileReader{path};

    reader.read(2, "could not read bitmap magic words");
    if (reader[0] != firstMagicByte || reader[1] != secondMagicByte)
    {
        THROW(BitmapReadException, "invalid magic words in header -- ", firstMagicByte, " ", secondMagicByte,
              " were expected instead of ", static_cast<int>(reader[0]), " ", static_cast<int>(reader[1]));
    }

    reader.read(8, "bytes 2 through 9 are missing from bitmap header");

    const auto readPixelArrayByteOffset = reader.readLittleEndianDoubleWord("could not read bitmap pixel array offset");
    if (readPixelArrayByteOffset != pixelArrayByteOffset)
    {
        THROW(BitmapReadException, "unsupported bitmap pixel array offset of ", readPixelArrayByteOffset, " -- only ",
              pixelArrayByteOffset, " is supported");
    }

    const auto readdibHeaderByteCount = reader.readLittleEndianDoubleWord("could not read bitmap DIB header");
    if (readdibHeaderByteCount != dibHeaderByteCount)
    {
        THROW(BitmapReadException, "unsupported bitmap DIB header size of ", readdibHeaderByteCount, " -- only ",
              dibHeaderByteCount, " is supported");
    }

    const auto maximumDimension = 1048576U;

    const auto readWidth = reader.readLittleEndianDoubleWord("could not read bitmap width from header");
    if (readWidth > maximumDimension)
    {
        THROW(BitmapReadException, "the width ", readWidth, " is larger than the maximum of ", maximumDimension);
    }
    const auto width = static_cast<int>(readWidth);

    const auto readHeight = reader.readLittleEndianDoubleWord("could not read bitmap height from header");
    if (readHeight > maximumDimension)
    {
        THROW(BitmapReadException, "the height ", readHeight, " is larger than the maximum of ", maximumDimension);
    }
    const auto height = static_cast<int>(readHeight);

    const auto readcolorPlanesCount = reader.readLittleEndianSingleWord("could not read bitmap color planes");
    if (readcolorPlanesCount != colorPlanesCount)
    {
        THROW(BitmapReadException, "unsupported bitmap color planes of ", readcolorPlanesCount, " -- only ",
              colorPlanesCount, " is supported");
    }

    const auto readBitsPerPixel = reader.readLittleEndianSingleWord("could not read bitmap color planes");
    if (readBitsPerPixel != bitsPerPixel)
    {
        THROW(BitmapReadException, "unsupported bitmap bits per pixel of ", readBitsPerPixel, " -- only ", bitsPerPixel,
              " is supported");
    }

    reader.read(4, "bytes 30 through 33 are missing from bitmap header");

    const auto readPixelArrayByteCount = reader.readLittleEndianDoubleWord("could not read bitmap pixel array size");

    reader.read(16, "bytes 38 through 53 are missing from bitmap header");

    auto image = Image{width, height};
    auto pixelArrayByteCount = 0;
    for (auto i = 0; i < height; i++)
    {
        for (auto j = 0; j < width; j++)
        {
            image(j, i) = reader.readLittleEndianColor("color bytes missing from bitmap pixel array");
            pixelArrayByteCount += 3;
        }
        const auto padding = (paddingByteCount - pixelArrayByteCount % paddingByteCount) % paddingByteCount;
        reader.read(padding, "padding bytes missing from bitmap pixel array");
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
