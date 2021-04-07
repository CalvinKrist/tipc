#include "TypeInference.h"
#include "TypeConstraint.h"
#include "TypeConstraintCollectVisitor.h"
#include "Unifier.h"
#include "FunctionGraph.h"
#include <sstream> 
#include <memory>

/*
 * This implementation collects the constraints and then solves them with a
 * unifier instance.  The unifier then records the inferred type results that
 * can be subsequently queried.
 */
std::unique_ptr<TypeInference> TypeInference::check(ASTProgram* ast, SymbolTable* symbols) {
  TypeConstraintCollectVisitor visitor(symbols);
  ast->accept(&visitor);
  // Create unifier with all constriants so the unionFind is correct
  auto unifier = std::make_unique<Unifier>(visitor.getCollectedConstraints());

  for(ASTFunction* func : ast->getFunctions()) {
    std::cout << "Processing function " << func->getName() << std::endl;
    visitor = TypeConstraintCollectVisitor(symbols);
    func->accept(&visitor);

    std::cout << "Solving function " << func->getName() << std::endl;
    // Solve only on the constraints in this single function
    unifier->solve(visitor.getCollectedConstraints());
  }

  auto r = std::make_unique<TypeInference>(symbols, std::move(unifier));

  std::cout << "\nFunctions : {\n"; 
  auto skip = true;
  for (auto f : symbols->getFunctions()) {
    if (skip) {
      skip = false;
      std::cout << "  " << f->getName() << " : " << r->getInferredType(f);
      continue;
    }
    std::cout << ",\n  " + f->getName() << " : " << r->getInferredType(f); 
  }
  std::cout << "\n}\n";

  for (auto f : symbols->getFunctions()) {
    std::cout << "\nLocals for function " + f->getName() + " : {\n";
    skip = true;
    for (auto l : symbols->getLocals(f)) {
      auto lT = r->getInferredType(l);
      if (skip) {
        skip = false;
        std::cout << "  " << l->getName() << " : " << *lT;
        continue;
      }
      std::cout << ",\n  " + l->getName() << " : " << *lT;
      std::cout << std::flush;
    }
    std::cout << "\n}\n";
  }
 
  return std::move(r);
}

std::shared_ptr<TipType> TypeInference::getInferredType(ASTDeclNode *node) {
  auto var = std::make_shared<TipVar>(node);
  return unifier->inferred(var);
};

bool TypeInference::isRecursive(ASTDeclNode *node) {
  return false;
}

void TypeInference::print(std::ostream &s) {
  std::cout << "\nFunctions : {\n"; 
  auto skip = true;
  for (auto f : symbols->getFunctions()) {
    if (skip) {
      skip = false;
      std::cout << "  " << f->getName() << " : " << *getInferredType(f);
      continue;
    }
    std::cout << ",\n  " + f->getName() << " : " << *getInferredType(f); 
  }
  std::cout << "\n}\n";

  for (auto f : symbols->getFunctions()) {
    std::cout << "\nLocals for function " + f->getName() + " : {\n";
    skip = true;
    for (auto l : symbols->getLocals(f)) {
      auto lT = getInferredType(l);
      if (skip) {
        skip = false;
        std::cout << "  " << l->getName() << " : " << *lT;
        continue;
      }
      std::cout << ",\n  " + l->getName() << " : " << *lT;
      std::cout << std::flush;
    }
    std::cout << "\n}\n";
  }
}

