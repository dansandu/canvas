#pragma once

#include "dansandu/canvas/image.hpp"

#include <exception>
#include <string>

namespace dansandu::canvas::bitmap
{

class BitmapReadException : public std::exception
{
public:
    explicit BitmapReadException(std::string message) : message_{std::move(message)}
    {
    }

    const char* what() const noexcept override
    {
        return message_.c_str();
    }

private:
    std::string message_;
};

PRALINE_EXPORT dansandu::canvas::image::Image readBitmapFile(const std::string& path);

PRALINE_EXPORT void writeBitmapFile(const std::string& path, const dansandu::canvas::image::Image& image);

}
