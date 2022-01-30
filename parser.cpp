#include "parser.h"
#include <boost/property_tree/xml_parser.hpp>

namespace Tobsterlang {
ASTNode Parser::parse(std::string const& filename) {
    ASTNode tree;
    boost::property_tree::read_xml(filename, tree);

    return tree;
}
}  // namespace Tobsterlang
