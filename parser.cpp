#include "parser.h"
#include <boost/property_tree/xml_parser.hpp>

namespace Tobsterlang::Parser {
auto parse(std::string const& filename) -> ASTNode {
    ASTNode tree;
    boost::property_tree::read_xml(filename, tree);

    return tree;
}
}  // namespace Tobsterlang::Parser