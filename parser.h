#ifndef TOBSTERLANG_PARSER_H
#define TOBSTERLANG_PARSER_H

#include <boost/property_tree/ptree.hpp>
#include <string>

namespace Tobsterlang {
using ASTNode = boost::property_tree::ptree;

class Parser {
public:
    ASTNode parse(std::string const&);
};
}  // namespace Tobsterlang

#endif  // TOBSTERLANG_PARSER_H
