#define CATCH_CONFIG_RUNNER
#include "catchorg/catch/catch.hpp"
#include "dansandu/ballotin/logging.hpp"

using dansandu::ballotin::logging::Level;
using dansandu::ballotin::logging::Logger;
using dansandu::ballotin::logging::UnitTestsHandler;

int main(const int argumentsCount, const char* const* const arguments)
{
    auto& logger = Logger::globalInstance();
    logger.setLevel(Level::debug);
    logger.addHandler("UnitTests", Level::debug, UnitTestsHandler{"unit_tests.log"});

    const auto catchResult = Catch::Session().run(argumentsCount, arguments);

    return catchResult;
}
