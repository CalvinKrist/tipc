#pragma once

#include <vector>
#include <set>

#include "AST.h"

class FunctionGroup {
    std::set<ASTFunction*> associatedFunctions;
    std::set<FunctionGroup*> possibleCalls;
    std::set<FunctionGroup*> callsiteFuncs;

    std::set<FunctionGroup*> callsiteFuncsDuplicate;
    bool alive{ true };
public:

    FunctionGroup(ASTFunction* func);
    const std::set<FunctionGroup*>& GetCalls() const;
    const std::set<ASTFunction*>& GetFuncs() const;
    void AddCall(FunctionGroup* group);
    void Union(FunctionGroup* group);
    void Finalize();
    bool IsTerminal() const;
    void PruneCallsite(FunctionGroup* group);
    void RestoreCallsites();
    std::string GetName();
};