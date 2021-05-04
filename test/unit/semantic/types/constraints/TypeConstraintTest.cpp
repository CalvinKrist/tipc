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
std::string ToString(const std::shared_ptr<TipType>& thing){
    std::stringstream ss;
    ss << *thing;
    return ss.str();
}

TEST_CASE("T1: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(rec(){ return rec(); } nonRec() { return 0; })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };

    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("rec")));
    REQUIRE(!analyzer.isFunctionRecursive(ast->findFunctionByName("nonRec")));
}

TEST_CASE("T2: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(nonRec(){ return 0; } rec() { var x; x = nonRec(); return rec(); })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };

    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("rec")));
    REQUIRE(!analyzer.isFunctionRecursive(ast->findFunctionByName("nonRec")));
}

TEST_CASE("T3: Interprocedural Recursion", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(rec1() { return rec2(); } rec2() { return rec1(); })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };

    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("rec1")));
    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("rec2")));
}

TEST_CASE("T4: Overlapping Recursive Groups", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(a(){ return b(); } b(){ var x; x = a(); return c(); } c(){ return b(); })";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };

    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("a")));
    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("b")));
    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("c")));
}

TEST_CASE("T5: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(d() {return 0;} b() {var x; x = d(); return c();} c() {return b();} a() {return b();})";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };

    REQUIRE(!analyzer.isFunctionRecursive(ast->findFunctionByName("a")));
    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("b")));
    REQUIRE(analyzer.isFunctionRecursive(ast->findFunctionByName("c")));
    REQUIRE(!analyzer.isFunctionRecursive(ast->findFunctionByName("d")));
}

TEST_CASE("T6: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(c() {return 0;} b() {return c();} a() {return b();})";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto q1{ analyzer.InverseTopoSort() };
    std::queue<std::string> q2;
    q2.push("c");
    q2.push("b");
    q2.push("a");

    while(!q1.empty()) {
        auto val = q1.front();
        q1.pop();

        auto val2 =q2.front();
        q2.pop();

        REQUIRE(val->GetName() == val2);
    }
}

TEST_CASE("T7: FlowAnalysis", "[FlowAnalysis]") {
    std::stringstream stream;
    stream << R"(d() {return 0;} c() {return d();} b() {return d();} a() { var x; x = b(); return c();})";

    auto ast = ASTHelper::build_ast(stream);

    FunctionGraphCreator analyzer{ ast.get() };
    auto queue{ analyzer.InverseTopoSort() };
    
    REQUIRE(queue.front()->GetName() == "d");
    REQUIRE(queue.back()->GetName() == "a");

    queue.pop();
    REQUIRE((queue.front()->GetName() == "b" || queue.front()->GetName() == "c"));
    queue.pop();
    REQUIRE((queue.front()->GetName() == "b" || queue.front()->GetName() == "c"));
}

TEST_CASE("T8: Let-Polymorphism", "[Let-Polymorphism]") {
    std::stringstream stream;
    stream << R"(id(a) { return a; } x(y){ return y; })";

    auto ast = ASTHelper::build_ast(stream);
    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    REQUIRE(ToString(types->getInferredType(ast->findFunctionByName("id")->getDecl())) == "(α<a>) -> α<a>");
}

TEST_CASE("T9: Let-Polymorphism", "[Let-Polymorphism]") {
    std::stringstream stream;
    stream << R"(id(a) { return a; } f2() { var x, z; x = id(0); z = {f : 1}; z = id(z); return 0;} )";

    auto ast = ASTHelper::build_ast(stream);
    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();
    auto symbols = analysis->getSymbolTable();

    for(auto l : symbols->getLocals(ast->findFunctionByName("f2")->getDecl())){
        if(l->getName() == "x")
            REQUIRE("int" == ToString(types->getInferredType(l)));
        else if(l->getName() == "z")
            REQUIRE("{f:int}" == ToString(types->getInferredType(l)));
    }
}

TEST_CASE("T10: Let-Polymorphism", "[Let-Polymorphism]") {
    std::stringstream stream;
    stream << R"( base() {return 0;} r1(y) { var x; if(y == 0) {x = base();} else {x = r1(x - 1); } return x;} )";

    auto ast = ASTHelper::build_ast(stream);
    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    REQUIRE(ToString(types->getInferredType(ast->findFunctionByName("base")->getDecl())) == "() -> int");
    REQUIRE(ToString(types->getInferredType(ast->findFunctionByName("r1")->getDecl())) == "(int) -> int");
}

TEST_CASE("T11: Integration", "[Integration]") {
    std::stringstream stream;
    stream << R"(ret(n) {if(n == 0){n = ret(n - 1);} return n;} id(a) { var b; b = ret(1); return a; } f2() { var x, z; x = id(0); z = {f : 1}; z = id(z); return 0;} )";

    auto ast = ASTHelper::build_ast(stream);
    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();
    auto symbols = analysis->getSymbolTable();

    for (auto l : symbols->getLocals(ast->findFunctionByName("f2")->getDecl())) {
        if(l->getName() == "x")
            REQUIRE("int" == ToString(types->getInferredType(l)));
        else if(l->getName() == "z")
            REQUIRE("{f:int}" == ToString(types->getInferredType(l)));
    }
}

TEST_CASE("T12: Integration", "[Integration]"){
    std::stringstream stream;
    stream << R"( id(x) {return x;} b() {var p, q; p = id(0); q = id(alloc 0); return b(); } a() {return b();} )";

    auto ast = ASTHelper::build_ast(stream);
    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    REQUIRE(ToString(types->getInferredType(ast->findFunctionByName("id")->getDecl())) == "(α<x>) -> α<x>");
    REQUIRE(ToString(types->getInferredType(ast->findFunctionByName("b")->getDecl())) == "() -> α<b()>");
    REQUIRE(ToString(types->getInferredType(ast->findFunctionByName("a")->getDecl())) == "() -> α<b()>");
}

TEST_CASE("T13: Failure - Recursive Polymorphic Function", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(swapper(a, b){ var p; p = swapper(b, a); return a; } x(){ return swapper(1, alloc 3); })";

    auto ast = ASTHelper::build_ast(stream);
    REQUIRE_THROWS(SemanticAnalysis::analyze(ast.get()));
}

TEST_CASE("T14: Failure - Function Pointers", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(
        ident(x){ return x; }
        caller(a, b, f){
                var x;
                x = f(a);
                return f(b);
        }
        test(){
                return caller(1, alloc 3, ident);
        }
    )";

    auto ast = ASTHelper::build_ast(stream);
    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    // this is unable to identify the return type of `caller` as `f` is never unified with `ident`
    REQUIRE(ToString(types->getInferredType(ast->findFunctionByName("test")->getDecl())) == "() -> α<caller(1, alloc 3, ident)>");
}

TEST_CASE("T15: Failure - Recursive Function Pointer Detection", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(rec(){ var x; x = rec; return x(); })";

    auto ast = ASTHelper::build_ast(stream);
    FunctionGraphCreator analyzer{ ast.get() };

    REQUIRE(!analyzer.isFunctionRecursive(ast->findFunctionByName("rec")));
}

TEST_CASE("T16: Failure - Flow Polymorphism", "[Failing Tests]") {
    std::stringstream stream;
    stream << R"(
        poly(x) { 
            var y; 
            if(y == 0) {
                x = 0;
            } else {
                x = {d:1};
            } 
            return x; 
        })";

    auto ast = ASTHelper::build_ast(stream);
    REQUIRE_THROWS(SemanticAnalysis::analyze(ast.get()));
}