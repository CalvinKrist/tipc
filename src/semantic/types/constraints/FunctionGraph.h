#pragma once

#include "AST.h"
#include "ASTVisitor.h"
#include <map>
#include <set>
#include <memory>
#include <queue>

class FunctionGraphCreator {
    class Node {
        friend class FunctionGraphCreator;
        ASTFunction* associatedFunction;
        std::set<Node*> possibleCalls;
        std::set<Node*> callsiteFuncs;

    public:
        Node(ASTFunction* func);
        void addCall(Node* node);
    };

    class FuncVisitor: public ASTVisitor {
        friend class FunctionGraphCreator;
        std::set<ASTFunction*> functions;
        std::set<std::string> locals;
        ASTProgram* program;
    public:
        FuncVisitor(ASTProgram* program);
        void endVisit(ASTDeclNode* call) override;
        void endVisit(ASTFunAppExpr* call) override;
    };

    std::map<ASTFunction*, std::unique_ptr<Node>> graph;
    ASTProgram* program;
    void buildGraph();
    bool dfsTraverseAsDAG(Node* current, std::set<Node*>& visited) const;
public:

    FunctionGraphCreator(ASTProgram* program);
    bool isDAG() const;
    bool isFunctionRecursive(ASTFunction* func) const;
    std::queue<ASTFunction*> inverseTopoSort();
    void print();
};
