#include "FunctionGraph.h"
#include <stack>
#include <iostream>
#include <assert.h>

FunctionGraphCreator::Node::Node(ASTFunction* func) : associatedFunction{ func }{}
void FunctionGraphCreator::Node::addCall(FunctionGraphCreator::Node* func){
    possibleCalls.emplace(func);
    func->callsiteFuncs.emplace(this);
}
void FunctionGraphCreator::FuncVisitor::endVisit(ASTFunAppExpr* func){
    if(auto f = dynamic_cast<ASTVariableExpr*>(func->getFunction())){
        if(locals.find(f->getName()) == locals.end()){
            functions.emplace(program->findFunctionByName(f->getName()));
        } else{
            functions.emplace(program->findFunctionByName(f->getName()));
        }
    } else{
        // Function variable being called
    }
}
void FunctionGraphCreator::FuncVisitor::endVisit(ASTDeclNode* decl){
    locals.emplace(decl->getName());
}
FunctionGraphCreator::FuncVisitor::FuncVisitor(ASTProgram* p) : program{ p }{}
bool FunctionGraphCreator::dfsTraverseAsDAG(Node* current, std::set<Node*>& visited) const{
    if(visited.find(current) != visited.end()){
        return false;
    }
    visited.emplace(current);
    for(auto call : current->possibleCalls){
        if(!dfsTraverseAsDAG(call, visited)){
            return false;
        }
    }
    visited.erase(current);
    return true;
}
FunctionGraphCreator::FunctionGraphCreator(ASTProgram* program) : program{ program }{
    buildGraph();
}
void FunctionGraphCreator::buildGraph(){
    graph.erase(graph.begin(), graph.end());
    for(auto func : program->getFunctions()){
        graph.emplace(func, std::make_unique<Node>(func));
    }

    for(auto func : program->getFunctions()){
        auto visitor{ FuncVisitor(program) };
        auto& node{ graph.at(func) };
        func->accept(&visitor);
        for(auto callTarget : visitor.functions){
            node->addCall(graph.at(callTarget).get());
        }
    }
}
bool FunctionGraphCreator::isDAG() const {
    for(auto& pair : graph){
        std::set<Node*> visited{};
        if(!dfsTraverseAsDAG(pair.second.get(), visited)){
            return false;
        }
    }
    return true;
}
bool FunctionGraphCreator::isFunctionRecursive(ASTFunction* func) const {
    std::set<Node*> visited{};
    return graph.find(func) != graph.end() && !dfsTraverseAsDAG(graph.at(func).get(), visited);
}
std::set<ASTFunction*> FunctionGraphCreator::getRecursiveFunctions() const {
    std::set<ASTFunction*> functions{};
    std::set<ASTFunction*> visited;

    for(auto& pair : graph) {
        if(isFunctionRecursive(pair.first) && visited.find(pair.first) == visited.end()) {

            // Add entire call tree of function to list
            std::queue<ASTFunction*> queue;
            queue.push(pair.first);
            visited.emplace(pair.first);

            while(queue.size() > 0) {
                auto func{ queue.front() };
                queue.pop();
                functions.emplace(func);

                for(auto& node : graph.find(func)->second->possibleCalls) {
                    if(visited.find(node->associatedFunction) == visited.end())
                        queue.emplace(node->associatedFunction);
                }
            }
        }
    }
    return functions;
}
std::queue<ASTFunction*> FunctionGraphCreator::inverseTopoSort(){
    std::set<ASTFunction*> recursives{ getRecursiveFunctions() };

    std::stack<ASTFunction*> stack{};
    std::queue<Node*> nodes{};
    for(auto& pair : graph){
        if(pair.second->callsiteFuncs.empty()){
            nodes.emplace(pair.second.get());
        }
    }

    std::set<Node*> visited{};
    while(!nodes.empty()){
        auto& top = nodes.front();
        stack.push(top->associatedFunction);
        for(auto child : top->possibleCalls){
            child->callsiteFuncs.erase(top);
            if(child->callsiteFuncs.empty()){
                nodes.push(child);
            }
        }
        nodes.pop();
    }

    std::queue<ASTFunction*> queue{};
    while(!stack.empty()){
        queue.emplace(stack.top());
        stack.pop();
    }

    buildGraph();
    return queue;
}
void FunctionGraphCreator::print(){
    for(auto& pair : graph){
        std::cout << pair.first->getName() << " calls: ";
        for(auto& node : pair.second->possibleCalls){
            std::cout << node->associatedFunction->getName() << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "Traversal: ";
    auto sort{ inverseTopoSort() };
    while(!sort.empty()){
        std::cout << sort.front()->getName() << ", ";
        sort.pop();
    }
    std::cout << std::endl;
}