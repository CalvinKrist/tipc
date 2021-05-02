#include "catch.hpp"
#include "TypeConstraint.h"
#include "TipFunction.h"
#include "TipInt.h"
#include "SemanticAnalysis.h"
#include "ASTHelper.h"
#include <vector>
#include <sstream>
#include <queue>
#include "FunctionGraph.h"
#include "TypeConstraintCollectVisitor.h"

TEST_CASE("TypeConstraint: Constraints are compared term-wise", "[TypeConstraint]") {
    std::vector<std::shared_ptr<TipType>> args;
    args.push_back(std::make_shared<TipInt>());
    auto function = std::make_shared<TipFunction>(args, std::make_shared<TipInt>());


    TypeConstraint constraint(function, function);
    TypeConstraint constraint2(function, function);
    REQUIRE(constraint == constraint2);
    REQUIRE_FALSE(constraint != constraint2);
}

TEST_CASE("TypeConstraint: Test output", "[TypeConstraint]") {
    std::vector<std::shared_ptr<TipType>> args;
    args.push_back(std::make_shared<TipInt>());
    auto function = std::make_shared<TipFunction>(args, std::make_shared<TipInt>());
    TypeConstraint constraint(function, function);

    std::stringstream sstream;
    sstream << constraint;
    REQUIRE_THAT(sstream.str(), Catch::Matches("^.* = .*$"));
}

std::string constraintToString(TypeConstraint cons) {
    std::stringstream ss;
    ss << cons;
    return ss.str();
}

TEST_CASE("T1: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(rec(){ return rec(); } nonRec() { return 0; })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto recursives{ analyzer.getRecursiveFunctions() };

    bool covered = false;

    for(auto func : recursives) {
        REQUIRE(func->getName() == "rec");
        covered = true;
    }

    REQUIRE(covered);
}

TEST_CASE("T2: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(nonRec(){ return 0; } rec() { var x; x = nonRec(); return rec(); })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto recursives{ analyzer.getRecursiveFunctions() };

    bool recCovered = false;
    bool nonRecCovered = false;

    for(auto func : recursives) {
        if(func->getName() == "rec")
            recCovered = true;
        else if(func->getName() == "nonRec")
            nonRecCovered = true;
    }

    REQUIRE(recCovered);
    REQUIRE(nonRecCovered);
}

TEST_CASE("T3: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(rec1() { return rec1(); } rec2() { return rec2(); })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto recursives{ analyzer.getRecursiveFunctions() };

    bool rec1Covered = false;
    bool rec2Covered = false;

    for(auto func : recursives)
        if(func->getName() == "rec1")
            rec1Covered = true;
        else if(func->getName() == "rec2")
            rec2Covered = true;

    REQUIRE(rec1Covered);
    REQUIRE(rec2Covered);
}

TEST_CASE("T4: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(rec(x){ if(x != 0) {x = rec(x - 1); } return x;} )";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto recursives{ analyzer.getRecursiveFunctions() };

    bool visited = recursives.size() == 1;
    REQUIRE(visited);
}

TEST_CASE("T5: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(d() {return 0;} b() {var x; x = d(); return c();} c() {return b();} a() {return b();})";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto recursives{ analyzer.getRecursiveFunctions() };

    bool bCovered = false;
    bool cCovered = false;
    bool dCovered = false;
    bool aCovered = false;

    for(auto func : recursives)
        if(func->getName() == "b")
            bCovered = true;
        else if(func->getName() == "c")
            cCovered = true;
        else if(func->getName() == "d")
            dCovered = true;
        else if(func->getName() == "a")
            aCovered = true;

    REQUIRE(bCovered);
    REQUIRE(cCovered);
    REQUIRE(dCovered);
    REQUIRE(!aCovered);
}

TEST_CASE("T6: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(c() {return 0;} b() {return c();} a() {return b();})";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto q1{ analyzer.inverseTopoSort() };
    std::queue<std::string> q2;
    q2.push("c");
    q2.push("b");
    q2.push("a");

    while(!q1.empty()) {
        auto val = q1.front();
        q1.pop();

        auto val2 =q2.front();
        q2.pop();

        REQUIRE(val->getName() == val2);
    }
}

TEST_CASE("T7: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(d() {return 0;} c() {return d();} b() {return d();} a() { var x; x = b(); return c();})";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto queue{ analyzer.inverseTopoSort() };
    
    REQUIRE(queue.front()->getName() == "d");
    REQUIRE(queue.back()->getName() == "a");

    queue.pop();
    REQUIRE((queue.front()->getName() == "b" || queue.front()->getName() == "c"));
    queue.pop();
    REQUIRE((queue.front()->getName() == "b" || queue.front()->getName() == "c"));
}

TEST_CASE("T8: Let-Polymorphism", "[Let-Polymorphism]") {
    std::stringstream stream;
    stream << R"(id(a) { return a; } )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    auto typeSignatures = types->unifier->getTypeSignatures();

    std::stringstream ss;
    for (auto const &pair: typeSignatures) 
        ss << *(pair.second.get());

    
    std::string funcType;
    funcType = ss.str();

    REQUIRE(funcType == "(α<a>) -> α<a>");
}

TEST_CASE("T9: Let-Polymorphism", "[Let-Polymorphism]") {
    std::stringstream stream;
    stream << R"(id(a) { return a; } f2() { var x, z; x = id(0); z = {f : 1}; z = id(z); return 0;} )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    std::stringstream xSS, zSS;
    std::string xType, zType;

    for (auto f : symbols->getFunctions()) {
        for (auto l : symbols->getLocals(f)) {
            if(l->getName() == "x")
                xSS << *(types->getInferredType(l));
            else if(l->getName() == "z")
                zSS << *(types->getInferredType(l));
        }
    }

    xType = xSS.str();
    zType = zSS.str();

    REQUIRE(xType == "int");
    REQUIRE(zType == "{f:int}");
}

TEST_CASE("T10: Let-Polymorphism", "[Let-Polymorphism]") {
    std::stringstream stream;
    stream << R"( base() {return 0;} r1(y) { var x; if(y == 0) {x = base();} else {x = r1(x - 1); } return x;} )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto unifier{ std::make_unique<Unifier>() };
    TypeConstraintCollectVisitor visitor(symbols.get());

    ast->accept(&visitor);

    unifier->solve(visitor.getCollectedConstraints());

    auto typeSignatures = unifier->getTypeSignatures();

    std::stringstream ss1, ss2;
    bool first = true;
    for (auto const &pair: typeSignatures) 
        if(first) {
            ss1 << *(pair.second.get());
            first = false;
        }
        else {
            ss2 << *(pair.second.get());
        }

    
    std::string f1Type, f2Type;
    f1Type = ss1.str();
    f2Type = ss2.str();

    REQUIRE(f1Type == "() -> int");
    REQUIRE(f2Type == "(int) -> int");
}

TEST_CASE("T11: Integration", "[Integration]") {
    std::stringstream stream;
    stream << R"(ret(n) {if(n == 0){n = ret(n - 1);} return n;} id(a) { var b; b = ret(1); return a; } f2() { var x, z; x = id(0); z = {f : 1}; z = id(z); return 0;} )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    std::stringstream xSS, zSS;
    std::string xType, zType;

    for (auto f : symbols->getFunctions()) {
        for (auto l : symbols->getLocals(f)) {
            if(l->getName() == "x")
                xSS << *(types->getInferredType(l));
            else if(l->getName() == "z")
                zSS << *(types->getInferredType(l));
        }
    }

    xType = xSS.str();
    zType = zSS.str();

    REQUIRE(xType == "int");
    REQUIRE(zType == "{f:int}");
}

TEST_CASE("T12: Integration", "[Integration]") {
    std::stringstream stream;
    stream << R"( c() {return 0;} b() {return c;} a() {return b();} )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    auto typeSignatures = types->unifier->getTypeSignatures();

    std::stringstream ss1, ss2, ss3;
    bool count = 1;
    for (auto const &pair: typeSignatures) 
        if(count == 1) {
            ss1 << *(pair.second.get());
            count = 2;
        }
        else if (count == 2) {
            ss2 << *(pair.second.get());
            count = 3;
        } else if(count == 3) {
            ss3 << *(pair.second.get());
        }

    
    std::string f1Type, f2Type, f3Type;
    f1Type = ss1.str();
    f2Type = ss2.str();
    f3Type = ss3.str();

    REQUIRE(f1Type == "() -> int(int)");
    REQUIRE(f2Type == "() -> int(int)");
    REQUIRE(f3Type == "() -> int(int)");
}

TEST_CASE("T13: Failing Tests", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(id(a, x) { if(x != 0){ a = id(a, x-1); } return a; } )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    auto typeSignatures = types->unifier->getTypeSignatures();

    std::stringstream ss;
    for (auto const &pair: typeSignatures) 
        ss << *(pair.second.get());

    
    std::string funcType;
    funcType = ss.str();

    REQUIRE(funcType == "(α<a>, int) -> α<a>");
}

TEST_CASE("T14: Failing Tests", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(id(a) {  return a; } rec(x) {x = id(x); if(x != 0) {x = rec(x-1);} return x; } )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    auto typeSignatures = types->unifier->getTypeSignatures();

    std::stringstream s1, s2;
    int count = 0;
    for (auto const &pair: typeSignatures) 
        if(count == 0) {
            s1 << *(pair.second.get());
            count += 1;
        } else {
            s2 << *(pair.second.get());
        }
        

    std::string f1Type, f2Type;
    f1Type = s1.str();
    f2Type = s2.str();

    REQUIRE(f1Type == "(α<a>) -> α<a>");
    REQUIRE(f2Type == "(int) -> int");
}

TEST_CASE("T15: Failing Tests", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(rec(){ var x; x = rec; return x(); })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto recursives{ analyzer.getRecursiveFunctions() };

    bool covered = false;

    for(auto func : recursives) {
        REQUIRE(func->getName() == "rec");
        covered = true;
    }

    REQUIRE(covered);
}

TEST_CASE("T16: Failing Tests", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(poly(x){ var y; if(y == 0) {x = 0;} else {x = {d:1}; } return x; })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto recursives{ analyzer.getRecursiveFunctions() };

    bool covered = false;

    for(auto func : recursives) {
        REQUIRE(func->getName() == "rec");
        covered = true;
    }

    REQUIRE(covered);
}