#include "dansandu/canvas/compression.hpp"
#include "dansandu/canvas/color.hpp"
#include "dansandu/canvas/image.hpp"
#include "dansandu/math/clustering.hpp"
#include "dansandu/math/matrix.hpp"
#include "dansandu/range/range.hpp"

#include <vector>

using dansandu::canvas::color::Color;
using dansandu::canvas::image::Image;
using dansandu::math::clustering::kMeans;
using dansandu::math::matrix::dynamic;
using dansandu::math::matrix::Matrix;
using dansandu::math::matrix::sliceRow;
using dansandu::range::range::consume;
using dansandu::range::range::map;
using dansandu::range::range::toVector;
using dansandu::range::range::operator|;

namespace dansandu::canvas::compression
{

Image compress(const Image& image, const int palette, const int iterations)
{
    auto samples = Matrix<float, dynamic, 3>{image.width() * image.height(), 3};
    image | consume([i = 0, &samples](const auto color) mutable {
        samples(i, 0) = color.red();
        samples(i, 1) = color.green();
        samples(i, 2) = color.blue();
        ++i;
    });
    const auto centroidsAndLabels = kMeans(samples, palette, iterations);
    auto pixels = centroidsAndLabels.second | map([&centroidsAndLabels](const auto label) {
                      return Color{static_cast<Color::value_type>(centroidsAndLabels.first(label, 0)),
                                   static_cast<Color::value_type>(centroidsAndLabels.first(label, 1)),
                                   static_cast<Color::value_type>(centroidsAndLabels.first(label, 2))};
                  }) |
                  toVector();
    return Image{image.width(), image.height(), std::move(pixels)};
}

}
