#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include <bits/stdc++.h>
#include <cstdio>

using namespace clang;
using namespace std;

vector<string> nodes1;
vector<string> nodes2;
bool first = true;

vector<string> normalize(const vector<string> &nodes)
{
    vector<string> result;
    set<string> skip = {"ImplicitCastExpr", "ParenExpr", "DeclRefExpr", "ImplicitValueInitExpr"};

    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (skip.count(nodes[i]))
            continue;

        if (nodes[i] == "CallExpr" && i + 1 < nodes.size())
        {
            int depth = 1;
            size_t j = i + 1;
            while (j < nodes.size() && depth > 0)
            {
                if (nodes[j] == "CallExpr" || nodes[j] == "CompoundStmt")
                    depth++;
                if (nodes[j] == "ReturnStmt")
                    depth--;
                j++;
            }
            if (j - i == 2)
                continue;
        }

        result.push_back(nodes[i]);
    }
    return result;
}

class SimpleVisitor : public RecursiveASTVisitor<SimpleVisitor>
{
  public:
    explicit SimpleVisitor(ASTContext *Context) : Context(Context)
    {
    }

    bool VisitDecl(Decl *D)
    {
        if (first)
        {
            nodes1.push_back(D->getDeclKindName());
        }
        else
        {
            nodes2.push_back(D->getDeclKindName());
        }
        return true;
    }

    bool VisitStmt(Stmt *S)
    {
        if (first)
        {
            nodes1.push_back(S->getStmtClassName());
        }
        else
        {
            nodes2.push_back(S->getStmtClassName());
        }
        return true;
    }

    bool VisitType(Type *T)
    {
        if (first)
        {
            nodes1.push_back(T->getTypeClassName());
        }
        else
        {
            nodes2.push_back(T->getTypeClassName());
        }
        return true;
    }

  private:
    ASTContext *Context;
};

class SimpleConsumer : public ASTConsumer
{
  public:
    explicit SimpleConsumer(ASTContext *Context) : Visitor(Context)
    {
    }

    virtual void HandleTranslationUnit(ASTContext &Context)
    {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

  private:
    SimpleVisitor Visitor;
};

class SimpleAction : public ASTFrontendAction
{
  public:
    virtual unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler, llvm::StringRef InFile)
    {
        return make_unique<SimpleConsumer>(&Compiler.getASTContext());
    }
};

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        stderr << "Error usage, not 2 arguments were passed";
        return 1;
    }

    ifstream file1(argv[1]) stringstream buffer;
    buffer << file1.rdbuf();
    string code1 = buffer.str();

    buffer.clear();

    ifstream file2(argv[2]) buffer << file2.rdbuf();
    string code2 = buffer.str();

    first = true;
    clang::tooling::runToolOnCode(make_unique<SimpleAction>(), code1);

    first = false;
    clang::tooling::runToolOnCode(make_unique<SimpleAction>(), code2);

    auto norm1 = normalize(nodes1);
    auto norm2 = normalize(nodes2);

    if (norm1 == norm2)
    {
        llvm::outs() << "equal\n";
    }
    else
    {
        llvm::outs() << "not equal\n";
    }

    return 0;
}
