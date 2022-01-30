#include <boost/program_options.hpp>
#include <iostream>
#include "codegen.h"
#include "compiler.h"
#include "options.h"
#include "parser.h"

int main(int argc, char** argv) {
    using namespace Tobsterlang;

    auto options = Options::parse(argc, argv);

    if (options.variables.count("help")) {
        std::cout << options.description << std::endl;

        return 0;
    }

    auto input_file = options.variables["input"].as<std::string>();

    auto tree = Parser::parse(input_file);

    Codegen codegen;
    auto module = codegen.generate(tree);

    std::string object_file =
        options.variables.count("output")
            ? options.variables["output"].as<std::string>()
            : "./" + std::string(module->getName().data()) + ".o";

    auto optimization_level = Options::get_optimization_level(
        options.variables["optimization"].as<std::string>());

    Compiler::compile(std::move(module), object_file, optimization_level);

    return 0;
}
