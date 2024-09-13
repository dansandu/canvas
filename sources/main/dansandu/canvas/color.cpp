#include "dansandu/canvas/color.hpp"

#include <iomanip>
#include <sstream>

namespace dansandu::canvas::color
{

std::ostream& operator<<(std::ostream& stream, const Color color)
{
    auto buffer = std::stringstream{};
    buffer << '#' << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(color.red())
           << std::setw(2) << static_cast<int>(color.green()) << std::setw(2) << static_cast<int>(color.blue())
           << std::setw(2) << static_cast<int>(color.alpha());
    return stream << buffer.rdbuf();
}

}
