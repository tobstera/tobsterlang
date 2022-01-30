#ifndef TOBSTERLANG_PARSER_H
#define TOBSTERLANG_PARSER_H

#include <boost/property_tree/ptree.hpp>
#include <string>

namespace Tobsterlang {
using ASTNode = boost::property_tree::ptree;

namespace Parser {
auto parse(std::string const&) -> ASTNode;
}  // namespace Parser
}  // namespace Tobsterlang

#endif  // TOBSTERLANG_PARSER_H
