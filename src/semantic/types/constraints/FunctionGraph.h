#pragma once

#include "AST.h"
#include "ASTVisitor.h"
#include "FunctionGroup.h"
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <set>

class FunctionGraphCreator {
  std::map<FunctionGroup*, FunctionGroup*> parents;
  std::map<FunctionGroup*, int> rank;

  void Union(FunctionGroup* f1, FunctionGroup* f2);
  FunctionGroup* Find(FunctionGroup* f1);

  class FuncVisitor : public ASTVisitor {
    friend class FunctionGraphCreator;
    std::set<ASTFunction*> functions;
    std::set<std::string> locals;
    ASTProgram* program;

  public:
    FuncVisitor(ASTProgram* program);
    void endVisit(ASTDeclNode* call) override;
    void endVisit(ASTFunAppExpr* call) override;
  };

  std::map<ASTFunction*, std::shared_ptr<FunctionGroup>> graph;
  std::vector<FunctionGroup*> roots;
  ASTProgram* program;

  void BuildGraph();
  void Derecursify();
  void DfsTraverseAsDAG(FunctionGroup* current, 
                        std::set<FunctionGroup*>& visited, 
                        std::vector<FunctionGroup*>& callstack);

public:
  FunctionGraphCreator(ASTProgram* program);
  std::queue<FunctionGroup*> InverseTopoSort();
  bool isFunctionRecursive(ASTFunction* func);
  void Print();
};
