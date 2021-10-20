#pragma once

#include "dansandu/canvas/image.hpp"

#include <string>
#include <vector>

namespace dansandu::canvas::gif
{

enum class Optimization
{
    space,
    quality
};

std::pair<std::vector<uint8_t>, int> lzw(const std::vector<int>& input, const int alphabetSize);

PRALINE_EXPORT std::vector<uint8_t> getGifBinary(const dansandu::canvas::image::Image& image,
                                                 const Optimization optimization = Optimization::space);

PRALINE_EXPORT std::vector<uint8_t> getGifBinary(const std::vector<const dansandu::canvas::image::Image*>& frames,
                                                 const int periodCentiseconds,
                                                 const Optimization optimization = Optimization::space);

PRALINE_EXPORT void writeGifFile(const std::string& path, const dansandu::canvas::image::Image& image,
                                 const Optimization optimization = Optimization::space);

PRALINE_EXPORT void writeGifFile(const std::string& path,
                                 const std::vector<const dansandu::canvas::image::Image*>& frames,
                                 const int periodCentiseconds, const Optimization optimization = Optimization::space);

}
