#ifndef TOBSTERLANG_OPTIONS_H
#define TOBSTERLANG_OPTIONS_H

#include <llvm/Passes/PassBuilder.h>
#include <boost/program_options.hpp>

namespace Tobsterlang::Options {
using OptionsDescription = boost::program_options::options_description;
using VariablesMap = boost::program_options::variables_map;

struct Options {
    OptionsDescription description;
    VariablesMap variables;
};

auto parse_optimization_level(std::string const& s) {
    if (s.find("-O") == 0) {
        return std::make_pair(std::string("optimization"), s.substr(2));
    }

    return std::make_pair(std::string(), std::string());
}

auto get_optimization_level(std::string const& level) {
    if (level == "1") {
        return llvm::PassBuilder::OptimizationLevel::O1;
    } else if (level == "2") {
        return llvm::PassBuilder::OptimizationLevel::O2;
    } else if (level == "3") {
        return llvm::PassBuilder::OptimizationLevel::O3;
    } else if (level == "s") {
        return llvm::PassBuilder::OptimizationLevel::Os;
    } else if (level == "z") {
        return llvm::PassBuilder::OptimizationLevel::Oz;
    }

    return llvm::PassBuilder::OptimizationLevel::O0;
}

Options parse(int argc, char** argv) {
    namespace po = boost::program_options;

    auto options_description = OptionsDescription("options");

    auto opt_adder = options_description.add_options();

    opt_adder("help,h", "print help message");
    opt_adder("optimization,O", po::value<std::string>(), "optimization level");
    opt_adder("output,o", po::value<std::string>(), "output file");
    opt_adder("input", po::value<std::string>(), "input file");

    auto positional_options_description = po::positional_options_description();
    positional_options_description.add("input", -1);

    VariablesMap args;
    po::store(boost::program_options::command_line_parser(argc, argv)
                  .options(options_description)
                  .positional(positional_options_description)
                  .extra_parser(parse_optimization_level)
                  .run(),
              args);

    return Options{.description = options_description,
                   .variables = args};
}
}  // namespace Tobsterlang::Options

#endif  // TOBSTERLANG_OPTIONS_H
