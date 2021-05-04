#include "FunctionGraph.h"
#include <stack>
#include <iostream>
#include <assert.h>

void FunctionGraphCreator::FuncVisitor::endVisit(ASTFunAppExpr* func){
    if(auto f = dynamic_cast<ASTVariableExpr*>(func->getFunction())){
        if(locals.find(f->getName()) == locals.end()){
            functions.emplace(program->findFunctionByName(f->getName()));
        } else{
            // Function variable being called
        }
    } else{
        // Function variable being called
    }
}
void FunctionGraphCreator::FuncVisitor::endVisit(ASTDeclNode* decl){
    locals.emplace(decl->getName());
}
FunctionGraphCreator::FuncVisitor::FuncVisitor(ASTProgram* p) : program{ p }{}
void FunctionGraphCreator::DfsTraverseAsDAG(FunctionGroup* current,
                                            std::set<FunctionGroup*>& visited,
                                            std::vector<FunctionGroup*>& callstack){
    if(visited.find(current) != visited.end()){
        for(int i = 0; i < callstack.size(); i++){
            if(callstack[i] == current){
                for(int j = i + 1; j < callstack.size(); j++){
                    Union(callstack[j], current);
                }
                return;
            }
        }
    } else{
        visited.emplace(current);
        callstack.emplace_back(current);
        for(auto& call : current->GetCalls()){
            DfsTraverseAsDAG(call, visited, callstack);
        }
        callstack.pop_back();
        visited.erase(current);
    }
}
FunctionGraphCreator::FunctionGraphCreator(ASTProgram* program) : program{ program }{
    BuildGraph();
    Derecursify();
}
void FunctionGraphCreator::Union(FunctionGroup* fg1, FunctionGroup* fg2){
    auto r1{ Find(fg1) };
    auto r2{ Find(fg2) };
    if(r1 == r2){
        return;
    } else if(rank[r1] > rank[r2]){
        parents[r2] = r1;
    } else if(rank[r2] > rank[r1]){
        parents[r1] = r2;
    } else{
        parents[r1] = r2;
        rank[r2] += 1;
    }
}
FunctionGroup* FunctionGraphCreator::Find(FunctionGroup* fg1){
    auto parent{ parents[fg1] };
    if(parent == fg1){
        return fg1;
    } else {
        return parents[fg1] = Find(parent);
    }
}
void FunctionGraphCreator::BuildGraph(){
    graph.erase(graph.begin(), graph.end());
    for(auto func : program->getFunctions()){
        graph.emplace(func, std::make_shared<FunctionGroup>(func));
    }

    for(auto func : program->getFunctions()){
        auto visitor{ FuncVisitor(program) };
        auto& node{ graph.at(func) };
        func->accept(&visitor);
        for(auto callTarget : visitor.functions){
            node->AddCall(graph.at(callTarget).get());
        }
    }
}
void FunctionGraphCreator::Derecursify(){
    for(auto& pair : graph){
        parents.emplace(pair.second.get(), pair.second.get());
        rank.emplace(pair.second.get(), 0);
    }
    std::set<FunctionGroup*> visited{};
    std::vector<FunctionGroup*> callstack{};
    for(auto& pair : graph){
        DfsTraverseAsDAG(pair.second.get(), visited, callstack);
    }

    for(auto& pair : graph){
        auto parent{ Find(pair.second.get()) };
        if(parent != pair.second.get()){
            pair.second = graph[*parent->GetFuncs().begin()];
            parent->Union(pair.second.get());
        } else{
            roots.emplace_back(parent);
        }
    }

    for(auto& root : roots){
        root->Finalize();
    }
}
std::queue<FunctionGroup*> FunctionGraphCreator::InverseTopoSort(){
    std::stack<FunctionGroup*> stack{};
    std::queue<FunctionGroup*> nodes{};
    for(auto& group : roots){
        if(group->IsTerminal()){
            nodes.emplace(group);
        }
    }

    std::set<FunctionGroup*> visited{};
    while(!nodes.empty()){
        auto top{ nodes.front() };
        stack.push(top);
        for(auto& child : top->GetCalls()){
            child->PruneCallsite(top);
            if(child->IsTerminal()){
                nodes.push(child);
            }
        }
        nodes.pop();
    }

    std::queue<FunctionGroup*> queue{};
    while(!stack.empty()){
        queue.emplace(stack.top());
        stack.pop();
    }

    for(auto& group : roots){
        group->RestoreCallsites();
    }
    return queue;
}
void FunctionGraphCreator::Print(){
    for(auto& group : roots){
        std::cout << group->GetName() << " calls: ";
        for(auto& node : group->GetCalls()){
            std::cout << node->GetName() << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "Traversal: ";
    auto sort{ InverseTopoSort() };
    while(!sort.empty()){
        std::cout << sort.front()->GetName() << ", ";
        sort.pop();
    }
    std::cout << std::endl;
}
bool FunctionGraphCreator::isFunctionRecursive(ASTFunction* func){
    return graph.at(func)->GetCalls().size() != 1;
}
