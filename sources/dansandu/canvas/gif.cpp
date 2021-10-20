#include "dansandu/canvas/gif.hpp"
#include "dansandu/ballotin/binary.hpp"
#include "dansandu/ballotin/exception.hpp"
#include "dansandu/ballotin/file_system.hpp"
#include "dansandu/ballotin/logging.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/range/range.hpp"

#include <algorithm>
#include <vector>

using dansandu::ballotin::binary::pushBits;
using dansandu::ballotin::file_system::writeBinaryFile;
using dansandu::canvas::color::Color;
using dansandu::canvas::color::Colors;
using dansandu::canvas::image::Image;

using namespace dansandu::range::range;

namespace dansandu::canvas::gif
{

static constexpr auto extensionIntroducer = 0x21;
static constexpr auto minimumColorsPerTable = 4;
static constexpr auto maximumColorsPerTable = 256;
static constexpr auto maximumDataSubBlockSize = 255;
static constexpr auto blockTerminator = 0x00;
static constexpr auto trailer = 0x3B;

std::pair<std::vector<uint8_t>, int> lzw(const std::vector<int>& input, const int alphabetSize)
{
    constexpr auto maximumCodeSize = 12;

    auto minimumCodeSize = 0;
    while ((1 << minimumCodeSize) < alphabetSize)
    {
        ++minimumCodeSize;
    }

    const auto clearCode = (1 << minimumCodeSize);
    const auto endCode = clearCode + 1;

    if (minimumCodeSize > maximumCodeSize)
    {
        THROW(std::invalid_argument, "alphabet size is too large and requires ", minimumCodeSize,
              " bits thus exceeding the maximum of ", maximumCodeSize, " bits");
    }

    auto dictionary = std::vector<std::vector<int>>{};

    auto sequence = std::vector<int>{};
    auto index = 0;

    auto output = std::vector<uint8_t>{};
    auto bitsCount = 0;
    auto code = 0;
    auto codeSize = minimumCodeSize + 1;

    pushBits(output, bitsCount, clearCode, codeSize);

    while (index < static_cast<int>(input.size()))
    {
        sequence.push_back(input[index]);

        if (sequence.size() == 1)
        {
            code = sequence.back();
            ++index;
        }
        else if (auto entry = std::find(dictionary.cbegin(), dictionary.cend(), sequence); entry != dictionary.end())
        {
            code = clearCode + 2 + (entry - dictionary.cbegin());
            ++index;
        }
        else
        {
            pushBits(output, bitsCount, code, codeSize);

            if ((1 << codeSize) <= clearCode + 2 + dictionary.size())
            {
                if (codeSize < maximumCodeSize)
                {
                    dictionary.push_back(std::move(sequence));
                    ++codeSize;
                }
                else
                {
                    sequence.clear();
                }
            }
            else
            {
                dictionary.push_back(std::move(sequence));
            }
        }
    }

    if (!sequence.empty())
    {
        pushBits(output, bitsCount, code, codeSize);
    }

    pushBits(output, bitsCount, endCode, codeSize);

    return {std::move(output), minimumCodeSize};
}

static void writeHeader(std::vector<uint8_t>& bytes)
{
    const auto signatureAndVersion = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61};
    bytes.insert(bytes.end(), signatureAndVersion.begin(), signatureAndVersion.end());
}

static void writeLogicalScreen(std::vector<uint8_t>& bytes, const unsigned width, const unsigned height,
                               const int globalColorTableSize)
{
    bytes.push_back((width >> 0) & 0xFF);
    bytes.push_back((width >> 8) & 0xFF);

    bytes.push_back((height >> 0) & 0xFF);
    bytes.push_back((height >> 8) & 0xFF);

    // +-------------------------+------------------+--------------------+-------------------------+
    // | Global Color Table Flag | Color resolution | Sorted Colors Flag | Global Color Table Size |
    // +-------------------------+------------------+--------------------+-------------------------+
    // | 0                       | 000              | 0                  | 000                     |
    // +-------------------------+------------------+--------------------+-------------------------+
    const auto hasGlobalColorTable = globalColorTableSize > 0;
    const auto colorResolution = 8;
    const auto hasSortedColors = false;

    auto globalColorTableSizeField = 0;
    while ((1 << (globalColorTableSizeField + 1)) < globalColorTableSize)
    {
        ++globalColorTableSizeField;
    }

    const auto packedFields =
        (hasGlobalColorTable << 7) | ((colorResolution - 1) << 4) | (hasSortedColors << 3) | globalColorTableSizeField;

    bytes.push_back(packedFields);

    const auto backgroundColorIndex = 0x00;
    bytes.push_back(backgroundColorIndex);

    const auto pixelAspectRatio = 0x00;
    bytes.push_back(pixelAspectRatio);
}

static void writeAnimationApplicationExtension(std::vector<uint8_t>& bytes, const unsigned repetitions = 0U)
{
    bytes.push_back(extensionIntroducer);

    const auto applicationExtensionLabel = 0xFF;
    bytes.push_back(applicationExtensionLabel);

    const auto blockSize = 0x0B;
    bytes.push_back(blockSize);

    const auto applicationIdentifier = {0x4E, 0x45, 0x54, 0x53, 0x43, 0x41, 0x50, 0x45};
    bytes.insert(bytes.end(), applicationIdentifier.begin(), applicationIdentifier.end());

    const auto authenticationCode = {0x32, 0x2E, 0x30};
    bytes.insert(bytes.end(), authenticationCode.begin(), authenticationCode.end());

    const auto subBlockSize = 0x03;
    bytes.push_back(subBlockSize);

    const auto currentSubBlockIndex = 0x01;
    bytes.push_back(currentSubBlockIndex);

    bytes.push_back((repetitions >> 0) & 0xFF);
    bytes.push_back((repetitions >> 8) & 0xFF);

    bytes.push_back(blockTerminator);
}

static void writeGraphicControlExtension(std::vector<uint8_t>& bytes, const unsigned delayCentiseconds)
{
    bytes.push_back(extensionIntroducer);

    const auto graphicControlLabel = 0xF9;
    bytes.push_back(graphicControlLabel);

    const auto blockSize = 0x04;
    bytes.push_back(blockSize);

    // +----------+-----------------+-----------------+------------------------+
    // | Reserved | Disposal Method | User Input Flag | Transparent Color Flag |
    // +----------+-----------------+-----------------+------------------------+
    // | 000      | 000             | 0               | 0                      |
    // +----------+-----------------+-----------------+------------------------+
    const auto disposal = 0;
    const auto userInput = false;
    const auto transparentColor = false;
    const auto packedFields = (disposal << 2) | (userInput << 1) | transparentColor;
    bytes.push_back(packedFields);

    bytes.push_back((delayCentiseconds >> 0) & 0xFF);
    bytes.push_back((delayCentiseconds >> 8) & 0xFF);

    const auto transparentBackgroundColorIndex = 0x00;
    bytes.push_back(transparentBackgroundColorIndex);

    bytes.push_back(blockTerminator);
}

static void writeImageDescriptor(std::vector<uint8_t>& bytes, const unsigned width, const unsigned height,
                                 const int localColorTableSize)
{
    const auto imageDescriptorLabel = 0x2C;
    bytes.push_back(imageDescriptorLabel);

    const auto imageLeftPosition = 0;
    bytes.push_back((imageLeftPosition >> 0) & 0xFF);
    bytes.push_back((imageLeftPosition >> 8) & 0xFF);

    const auto imageTopPosition = 0;
    bytes.push_back((imageTopPosition >> 0) & 0xFF);
    bytes.push_back((imageTopPosition >> 8) & 0xFF);

    bytes.push_back((width >> 0) & 0xFF);
    bytes.push_back((width >> 8) & 0xFF);

    bytes.push_back((height >> 0) & 0xFF);
    bytes.push_back((height >> 8) & 0xFF);

    // +------------------------+----------------+-------------+----------+------------------------+
    // | Local Color Table Flag | Interlace Flag | Sorted Flag | Reserved | Local Color Table Size |
    // +------------------------+----------------+-------------+----------+------------------------+
    // | 0                      | 0              | 0           | 00       | 000                    |
    // +------------------------+----------------+-------------+----------+------------------------+
    const auto hasLocalColorTable = localColorTableSize > 0;
    const auto isInterlaced = false;
    const auto hasSortedColors = false;

    auto localColorTableSizeField = 0;
    while ((1 << (localColorTableSizeField + 1)) < localColorTableSize)
    {
        ++localColorTableSizeField;
    }

    const auto packedFields =
        (hasLocalColorTable << 7) | (isInterlaced << 6) | (hasSortedColors << 5) | localColorTableSizeField;

    bytes.push_back(packedFields);
}

static void writeColorTable(std::vector<uint8_t>& bytes, const std::vector<Color>& colors)
{
    LOG_DEBUG("writing color table with ", colors.size(), " colors");

    for (const auto color : colors)
    {
        bytes.push_back(color.red());
        bytes.push_back(color.green());
        bytes.push_back(color.blue());
    }

    const auto colorsSize = static_cast<int>(colors.size());

    auto colorTableSize = 1;
    while (colorTableSize < colorsSize)
    {
        colorTableSize <<= 1;
    }

    for (auto padding = colorsSize; padding < colorTableSize; ++padding)
    {
        bytes.push_back(0);
        bytes.push_back(0);
        bytes.push_back(0);
    }
}

static void writeImageData(std::vector<uint8_t>& bytes, const std::vector<int>& indexes, const int codeSize)
{
    const auto output = lzw(indexes, codeSize);
    const auto& lzwOutput = output.first;
    const auto minimumCodeSize = output.second;

    LOG_DEBUG("lzw coding with minimum code size ", minimumCodeSize, ", code size ", codeSize, " and output of ",
              lzwOutput.size(), " bytes");

    const auto lzwOutputSize = static_cast<int>(lzwOutput.size());

    bytes.push_back(minimumCodeSize);

    auto blockStart = 0;
    while (blockStart < lzwOutputSize)
    {
        const auto count = std::min(maximumDataSubBlockSize, lzwOutputSize - blockStart);

        bytes.push_back(count);

        for (auto index = 0; index < count; ++index)
        {
            bytes.push_back(lzwOutput[blockStart + index]);
        }

        blockStart += count;
    }

    bytes.push_back(blockTerminator);
}

static std::pair<std::vector<Color>, std::vector<int>> getImageColors(const Image& image)
{
    auto colors = std::vector<Color>{};
    auto indexes = std::vector<int>{};

    for (const auto color : image)
    {
        const auto position = std::find(colors.cbegin(), colors.cend(), color);
        indexes.push_back(position - colors.cbegin());
        if (position == colors.cend())
        {
            colors.push_back(color);
        }
    }

    if (static_cast<int>(colors.size()) > maximumColorsPerTable)
    {
        LOG_DEBUG("gif image color palette is of size ", colors.size(), " which exceeds maximum palette size of ",
                  maximumColorsPerTable, " and requires compression");

        auto reducedColors = std::vector<Color>{};
        auto reducedIndexes = std::vector<int>{};

        const auto redSamples = 4;
        const auto greenSamples = 8;
        const auto blueSamples = 8;

        const auto channelDepth = 255.0f;

        const auto redSampling = channelDepth / (redSamples - 1);
        const auto greenSampling = channelDepth / (greenSamples - 1);
        const auto blueSampling = channelDepth / (blueSamples - 1);

        for (const auto index : indexes)
        {
            const auto color = colors[index];
            const auto red = static_cast<Color::value_type>(std::round(color.red() / redSampling) * redSampling);
            const auto green =
                static_cast<Color::value_type>(std::round(color.green() / greenSampling) * greenSampling);
            const auto blue = static_cast<Color::value_type>(std::round(color.blue() / blueSampling) * blueSampling);
            const auto reducedColor = Color{red, green, blue};

            const auto position = std::find(reducedColors.cbegin(), reducedColors.cend(), reducedColor);
            reducedIndexes.push_back(position - reducedColors.cbegin());
            if (position == reducedColors.cend())
            {
                reducedColors.push_back(reducedColor);
            }
        }

        colors = std::move(reducedColors);
        indexes = std::move(reducedIndexes);

        LOG_DEBUG("gif image color palette was reduced to ", colors.size(), " colors ");
    }

    while (colors.size() < minimumColorsPerTable)
    {
        colors.push_back(Colors::black);
    }

    return {std::move(colors), std::move(indexes)};
}

std::vector<uint8_t> getGifBinary(const Image& image, const Optimization)
{
    LOG_DEBUG("generating gif image binary");

    if (image.empty())
    {
        THROW(std::invalid_argument, "gif image cannot be empty");
    }

    auto bytes = std::vector<uint8_t>{};

    writeHeader(bytes);

    const auto globalColorsCount = 0;

    writeLogicalScreen(bytes, image.width(), image.height(), globalColorsCount);

    const auto delay = 0;

    writeGraphicControlExtension(bytes, delay);

    const auto [colors, indexes] = getImageColors(image);
    const auto localColorsCount = static_cast<int>(colors.size());

    writeImageDescriptor(bytes, image.width(), image.height(), localColorsCount);
    writeColorTable(bytes, colors);
    writeImageData(bytes, indexes, localColorsCount);

    bytes.push_back(trailer);

    return bytes;
}

std::vector<uint8_t> getGifBinary(const std::vector<const Image*>& frames, const int periodCentiseconds,
                                  const Optimization)
{
    LOG_DEBUG("generating gif animation binary with ", frames.size(), " frames and ", periodCentiseconds, " cs period");

    if (frames.empty())
    {
        THROW(std::invalid_argument, "gif animation frames cannot be empty");
    }

    auto bytes = std::vector<uint8_t>{};

    writeHeader(bytes);

    const auto width = frames.front()->width();
    const auto height = frames.front()->height();
    const auto globalColorsCount = 0;

    writeLogicalScreen(bytes, width, height, globalColorsCount);
    writeAnimationApplicationExtension(bytes);

    for (const auto frame : frames)
    {
        if (!frame)
        {
            THROW(std::invalid_argument, "gif animation frame cannot be null");
        }

        if (frame->empty())
        {
            THROW(std::invalid_argument, "gif animation frame cannot be empty");
        }

        if (frame->width() != width || frame->height() != height)
        {
            THROW(std::invalid_argument, "gif animation frames do not match in size");
        }

        writeGraphicControlExtension(bytes, periodCentiseconds);

        const auto [colors, indexes] = getImageColors(*frame);
        const auto localColorsCount = static_cast<int>(colors.size());

        writeImageDescriptor(bytes, width, height, localColorsCount);
        writeColorTable(bytes, colors);
        writeImageData(bytes, indexes, localColorsCount);
    }

    bytes.push_back(trailer);

    return bytes;
}

void writeGifFile(const std::string& path, const dansandu::canvas::image::Image& image, const Optimization optimization)
{
    const auto binary = getGifBinary(image, optimization);
    writeBinaryFile(path, binary);
}

void writeGifFile(const std::string& path, const std::vector<const dansandu::canvas::image::Image*>& frames,
                  const int periodCentiseconds, const Optimization optimization)
{
    const auto binary = getGifBinary(frames, periodCentiseconds, optimization);
    writeBinaryFile(path, binary);
}

}
