#include "catch.hpp"
#include "TypeConstraint.h"
#include "TipFunction.h"
#include "TipInt.h"
#include "SemanticAnalysis.h"
#include "ASTHelper.h"
#include <vector>
#include <sstream>

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

TEST_CASE("Let Polymorphism: function duplication", "[TypeConstraint]") {
    std::stringstream stream;
    stream << R"(f1() { var x; x=0; return x; } f2() { var y, z; y=f1(); z=f1(); return 0;} )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    std::stringstream ss;
    ss << types;
    auto s = ss.str();
    std::cout << "Types v1: " << s << std::endl;

    std::string yCons, zCons;

    auto constraints = types->unifier->getConstraints();
    for(auto c : constraints) {
        auto conStr = constraintToString(c);
        if(conStr.find("[[y") != std::string::npos)
            yCons = conStr;
        else if(conStr.find("[[z") != std::string::npos)
            zCons = conStr;
    }

    yCons = yCons.substr(yCons.find("[[f"));
    yCons = yCons.substr(0, yCons.find("@"));
    zCons = zCons.substr(zCons.find("[[f"));
    zCons = zCons.substr(0, zCons.find("@"));

    std::stringstream ySS, zSS;
    std::string yType, zType;

    for (auto f : symbols->getFunctions()) {
        for (auto l : symbols->getLocals(f)) {
            if(l->getName() == "y")
                ySS << *(types->getInferredType(l));
            else if(l->getName() == "z")
                zSS << *(types->getInferredType(l));
        }
    }
    yType = ySS.str();
    zType = zSS.str();

    REQUIRE(yType == "int");
    REQUIRE(zType == "int");
    REQUIRE(yCons != zCons);
}

TEST_CASE("Let Polymorphism: variable typing", "[TypeConstraint]") {
    std::stringstream stream;
    stream << R"(id(a) { return a; } f2() { var x, y, z; x = id(0); y = id(0); z = {f : 1}; z = id(z); return 0;} )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    std::stringstream ySS, xSS, zSS;
    std::string yType, xType, zType;

    for (auto f : symbols->getFunctions()) {
        for (auto l : symbols->getLocals(f)) {
            if(l->getName() == "y")
                ySS << *(types->getInferredType(l));
            else if(l->getName() == "x")
                xSS << *(types->getInferredType(l));
            else if(l->getName() == "z")
                zSS << *(types->getInferredType(l));
        }
    }
    yType = ySS.str();
    xType = xSS.str();
    zType = zSS.str();

    REQUIRE(yType == "int");
    REQUIRE(xType == "int");
    REQUIRE(zType == "{f:int}");
}

TEST_CASE("Let Polymorphism: function typing", "[TypeConstraint]") {
    std::stringstream stream;
    stream << R"(f() { return 0; } g(x) { return x; })";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    std::stringstream ss;
    std::string res;

    types->print(ss);
    res = ss.str();

    std::size_t found = res.find(": () -> int");
    REQUIRE(found!=std::string::npos);

    found = res.find(": (α<x>) -> α<x>");
    REQUIRE(found!=std::string::npos);
}

TEST_CASE("Let Polymorphism: simple non-recirsive function detection", "[TypeConstraint]") {
    std::stringstream stream;
    stream << R"(f1(x) { return x; } f2() { var y; y = f1(10); return y; } f3() { var z; z = {f:1}; z.f = f2(); return z; } )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    for (auto f : symbols->getFunctions()) {
        REQUIRE(types->isRecursive(f) == false);
    }
}

TEST_CASE("Let Polymorphism: simple recirsive function detection", "[TypeConstraint]") {
    std::stringstream stream;
    stream << R"(f1(x) { if(x > 0) {x = x + f1(x - 1);} return 0; } )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    for (auto f : symbols->getFunctions()) {
        REQUIRE(types->isRecursive(f));
    }
}

TEST_CASE("Let Polymorphism: complex recirsive function detection", "[TypeConstraint]") {
    std::stringstream stream;
    stream << R"(f1(x) { if(x > 0) {x = x + f2(x - 1);} return 0; } f2(x) { if(x > 0) {x = x + f1(x - 1);} return 0; } )";

    auto ast = ASTHelper::build_ast(stream);

    std::unique_ptr<SymbolTable> symbols;
    REQUIRE_NOTHROW(symbols = SymbolTable::build(ast.get()));

    auto analysis = SemanticAnalysis::analyze(ast.get());
    auto types = analysis->getTypeResults();

    for (auto f : symbols->getFunctions()) {
        REQUIRE(types->isRecursive(f));
    }
}