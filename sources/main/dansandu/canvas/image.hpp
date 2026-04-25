#pragma once

#include "dansandu/canvas/color.hpp"
#include "dansandu/journey/exception.hpp"

#include <algorithm>
#include <vector>

namespace dansandu::canvas::image
{

class Image
{
public:
    using size_type = int;
    using Color = dansandu::canvas::color::Color;
    using const_iterator = std::vector<Color>::const_iterator;
    using iterator = std::vector<Color>::iterator;

    Image() : width_{0}, height_{0}
    {
    }

    Image(const size_type width, const size_type height) : Image{width, height, Color::black}
    {
    }

    Image(const size_type width, const size_type height, const Color fill)
        : Image{width, height, std::vector<Color>(width * height, fill)}
    {
    }

    Image(const size_type width, const size_type height, std::vector<Color> colors) : width_{width}, height_{height}
    {
        if (width_ < 0 || height_ < 0)
        {
            THROW(std::invalid_argument, "width x height dimensions ", width_, "x", height_,
                  " must be greater than or equal to zero");
        }

        if (width_ * height_ != static_cast<int>(colors.size()))
        {
            THROW(std::invalid_argument, "colors size ", colors.size(), " must match image area ", width_ * height_);
        }

        if (width_ == 0 || height_ == 0)
        {
            width_ = height_ = 0;
        }

        colors_ = std::move(colors);
    }

    Image(const Image&) = default;

    Image(Image&& other) noexcept : width_{other.width_}, height_{other.height_}, colors_{std::move(other.colors_)}
    {
        other.width_ = other.height_ = 0;
    }

    Image& operator=(const Image& other)
    {
        if (this != &other)
        {
            width_ = other.width_;
            height_ = other.height_;
            colors_ = other.colors_;
        }

        return *this;
    }

    Image& operator=(Image&& other) noexcept
    {
        if (this != &other)
        {
            width_ = other.width_;
            height_ = other.height_;
            colors_ = std::move(other.colors_);

            other.width_ = other.height_ = 0;
        }

        return *this;
    }

    Color& operator()(const size_type x, const size_type y)
    {
        return colors_[index(x, y)];
    }

    const Color& operator()(const size_type x, const size_type y) const
    {
        return colors_[index(x, y)];
    }

    Color& clampedIndex(const size_type x, const size_type y)
    {
        return colors_[getClampedIndex(x, y)];
    }

    const Color& clampedIndex(const size_type x, const size_type y) const
    {
        return colors_[getClampedIndex(x, y)];
    }

    void clear(const Color color = Color::black)
    {
        std::fill(colors_.begin(), colors_.end(), color);
    }

    size_type width() const noexcept
    {
        return width_;
    }

    size_type height() const noexcept
    {
        return height_;
    }

    size_type size() const noexcept
    {
        return width_ * height_;
    }

    bool empty() const noexcept
    {
        return (width_ == 0) | (height_ == 0);
    }

    const uint8_t* bytes() const noexcept
    {
        return static_cast<const uint8_t*>(static_cast<const void*>(colors_.data()));
    }

    auto begin()
    {
        return colors_.begin();
    }

    auto end()
    {
        return colors_.end();
    }

    auto begin() const
    {
        return colors_.begin();
    }

    auto end() const
    {
        return colors_.end();
    }

    auto cbegin() const
    {
        return colors_.cbegin();
    }

    auto cend() const
    {
        return colors_.cend();
    }

private:
    size_type index(const size_type x, const size_type y) const
    {
        if ((x < 0) | (x >= width_) | (y < 0) | (y >= height_))
        {
            THROW(std::out_of_range, "cannot index the (", x, ", ", y, ") pixel in an ", width_, "x", height_,
                  " image -- indices are out of bounds");
        }
        return x + y * width_;
    }

    size_type getClampedIndex(const size_type x, const size_type y) const
    {
        if ((width_ == 0) | (height_ == 0))
        {
            THROW(std::out_of_range, "Cannot index an empty image");
        }

        const auto xAboveZero = (x > 0);
        const auto xBelowWidth = (x < width_);
        const auto cx = (xAboveZero & xBelowWidth) * x + !xBelowWidth * (width_ - 1);

        const auto yAboveZero = (y > 0);
        const auto yBelowHeight = (y < height_);
        const auto cy = (yAboveZero & yBelowHeight) * y + !yBelowHeight * (height_ - 1);

        return cx + cy * width_;
    }

    size_type width_;
    size_type height_;
    std::vector<Color> colors_;
};

inline bool operator==(const Image& lhs, const Image& rhs)
{
    return lhs.width() == rhs.width() && lhs.height() == rhs.height() &&
           std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

inline bool operator!=(const Image& lhs, const Image& rhs)
{
    return !(lhs == rhs);
}

}
