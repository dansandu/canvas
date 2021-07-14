#pragma once

#include "dansandu/canvas/image.hpp"

namespace dansandu::canvas::compression
{

PRALINE_EXPORT dansandu::canvas::image::Image compress(const dansandu::canvas::image::Image& image, const int palette,
                                                       const int iterations);

PRALINE_EXPORT dansandu::canvas::image::Image testCompress(const dansandu::canvas::image::Image& image,
                                                           const int palette, const int iterations);

}
