#pragma once

#include "dansandu/canvas/image.hpp"

#include <string>
#include <vector>

namespace dansandu::canvas::gif
{

std::pair<std::vector<uint8_t>, int> lzw(const std::vector<int>& input, const int alphabetSize);

std::vector<uint8_t> getGifBinary(const dansandu::canvas::image::Image& image);

std::vector<uint8_t> getGifBinary(const std::vector<const dansandu::canvas::image::Image*>& frames,
                                  const int periodCentiseconds);

std::vector<uint8_t> testGetGifBinary(const dansandu::canvas::image::Image& image);

std::vector<uint8_t> testGetGifBinary(const std::vector<const dansandu::canvas::image::Image*>& frames,
                                      const int periodCentiseconds);

PRALINE_EXPORT void writeGifFile(const std::string& path, const dansandu::canvas::image::Image& image);

PRALINE_EXPORT void writeGifFile(const std::string& path,
                                 const std::vector<const dansandu::canvas::image::Image*>& frames,
                                 const int periodCentiseconds);

}
