#include "dansandu/canvas/color.hpp"

#include <iomanip>
#include <sstream>

namespace dansandu::canvas::color
{

namespace
{

void colorToStream(std::ostream& stream, const Color& color)
{
    stream << '#' << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
           << static_cast<int>(color.getRedChannel()) << std::setw(2) << static_cast<int>(color.getGreenChannel())
           << std::setw(2) << static_cast<int>(color.getBlueChannel()) << std::setw(2)
           << static_cast<int>(color.getAlphaChannel());
}

}

std::string Color::toString() const
{
    auto stream = std::ostringstream{};

    colorToStream(stream, *this);

    return stream.str();
}

void Color::toStream(std::ostream& stream) const
{
    auto buffer = std::ostringstream{};

    colorToStream(buffer, *this);

    stream << buffer.rdbuf();
}

}
