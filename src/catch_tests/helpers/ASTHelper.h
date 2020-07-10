#pragma once

#include "ASTBuilder.h"
#include <istream>

class ASTHelper {
public:
  static std::unique_ptr<AST::Program> build_ast(std::istream &stream);
  static bool check_parse(std::istream &stream);
};
