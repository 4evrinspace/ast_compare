#include "ast_compare.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

using namespace clang;

void buildSubtreeHashes(AnalysisResult &res);

// Clang специфичная структура, позволяющая производить анализ
class ASTNodeVisitor : public RecursiveASTVisitor<ASTNodeVisitor> {
public:
    ASTNodeVisitor(AnalysisResult *res) : res(res) {}

    // Нода объявления
    bool TraverseDecl(Decl *D) {
        if (!D) return true;
        if (D->isImplicit()) return true;
        string label = getDeclLabel(D);
        return doTraverse(label, [&] { return RecursiveASTVisitor::TraverseDecl(D); });
    }

    // Нода выражения 
    bool TraverseStmt(Stmt *S) {
        if (!S) return true;
        string label = getStmtLabel(S);
        return doTraverse(label, [&] { return RecursiveASTVisitor::TraverseStmt(S); });
    }

    // Нода иного случая
    bool TraverseType(QualType) { return true; }

private:
    // Основная логика работы - обрабокта ноды 
    template <typename Fn>
    bool doTraverse(const string &label, Fn next) {
        if (label.empty()) return next();

        res->normNodes.push_back(label);
        res->freqMap[label]++;

        if (!stack.empty())
            res->edgePairs.push_back(stack.back() + "->" + label);

        int idx = res->treeNodes.size();
        res->treeNodes.push_back({label, {}});
        if (!idxStack.empty())
            res->treeNodes[idxStack.back()].children.push_back(idx);

        if (label == "IfStmt" || label == "ForStmt" || label == "WhileStmt" ||
            label == "DoStmt" || label == "SwitchStmt")
            res->cfProfile.push_back(label + "@" + to_string(stack.size()));

        stack.push_back(label);
        idxStack.push_back(idx);
        bool ret = next();
        idxStack.pop_back();
        stack.pop_back();
        return ret;
    }

    // В строку
    string getDeclLabel(Decl *D) {
        if (!D) return "";
        if (auto FD = dyn_cast<FunctionDecl>(D)) {
            if (!FD->getDeclName().isIdentifier()) return "";
            return "FunctionDecl/" + to_string(FD->getNumParams());
        }
        if (isa<ParmVarDecl>(D)) return "ParmVarDecl";
        if (isa<VarDecl>(D)) return "VarDecl";
        return "";
    }

    string getStmtLabel(Stmt *S) {
        if (isa<ImplicitCastExpr>(S) || isa<ImplicitValueInitExpr>(S) ||
            isa<ParenExpr>(S) || isa<CXXDefaultArgExpr>(S) ||
            isa<ExprWithCleanups>(S) || isa<DeclRefExpr>(S))
            return "";
        if (auto BO = dyn_cast<BinaryOperator>(S))
            return "BinaryOperator/" + BinaryOperator::getOpcodeStr(BO->getOpcode()).str();
        if (auto CE = dyn_cast<CallExpr>(S))
            return "CallExpr/" + to_string(CE->getNumArgs());
        return S->getStmtClassName();
    }

    AnalysisResult *res;
    vector<string> stack;
    vector<int> idxStack;
};

// Сlang специфичный разбор 
class MyConsumer : public ASTConsumer {
public:
    MyConsumer(AnalysisResult *res) : visitor(res) {}
    void HandleTranslationUnit(ASTContext &ctx) override {
        visitor.TraverseDecl(ctx.getTranslationUnitDecl());
    }
private:
    ASTNodeVisitor visitor;
};

// Clang специфичное frontend класс
class MyAction : public ASTFrontendAction {
public:
    MyAction(AnalysisResult *res) : res(res) {}
    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
        return make_unique<MyConsumer>(res);
    }
private:
    AnalysisResult *res;
};

// Для функций будем использовать иной разбор  
class FunctionCollectConsumer : public ASTConsumer {
public:
    FunctionCollectConsumer(vector<FunctionFingerprint> *out) : out(out) {}
    void HandleTranslationUnit(ASTContext &ctx) override {
        for (auto *D : ctx.getTranslationUnitDecl()->decls()) {
            if (D->isImplicit()) continue;
            auto *FD = dyn_cast<FunctionDecl>(D);
            if (!FD) continue;
            if (!FD->doesThisDeclarationHaveABody()) continue;
            if (!FD->getDeclName().isIdentifier()) continue;

            AnalysisResult res;
            ASTNodeVisitor visitor(&res);
            visitor.TraverseDecl(FD);
            buildSubtreeHashes(res);

            FunctionFingerprint fp;
            fp.name = FD->getNameAsString();
            fp.arity = FD->getNumParams();
            fp.result = res;
            out->push_back(fp);
        }
    }
private:
    vector<FunctionFingerprint> *out;
};

// Аналогичное frontend действие для функций
class FunctionCollectAction : public ASTFrontendAction {
public:
    FunctionCollectAction(vector<FunctionFingerprint> *out) : out(out) {}
    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
        return make_unique<FunctionCollectConsumer>(out);
    }
private:
    vector<FunctionFingerprint> *out;
};

// Легко заметить что данные операторы можно переставить 
set<string> commutativeOps = {"+", "*", "&", "|", "^", "==", "!=", "&&", "||"};

// Хэши поддеревьев
void buildSubtreeHashes(AnalysisResult &res) {
    int n = res.treeNodes.size();
    if (n == 0) return;

    vector<size_t> h(n, 0);
    hash<string> hasher;

    for (int i = n - 1; i >= 0; i--) {
        TreeNode &node = res.treeNodes[i];
        vector<size_t> ch;
        for (int c : node.children)
            ch.push_back(h[c]);

        if (node.label.size() > 15 && node.label.substr(0, 15) == "BinaryOperator/") {
            string op = node.label.substr(15);
            if (commutativeOps.count(op))
                sort(ch.begin(), ch.end());
        }

        size_t val = hasher(node.label);
        for (size_t x : ch)
            val ^= x + 0x1645d9f3b + (val << 6) + (val >> 2);

        h[i] = val;
        res.subtreeHashes.push_back(val);
    }
}

// Подсчет одной из метрик 
double calcLCS(const vector<string> &a, const vector<string> &b) {
    if (a.empty() && b.empty()) return 1.0;
    if (a.empty() || b.empty()) return 0.0;

    int n = a.size(), m = b.size();
    vector<int> prev(m + 1, 0), curr(m + 1, 0);
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            if (a[i-1] == b[j-1])
                curr[j] = prev[j-1] + 1;
            else
                curr[j] = max(prev[j], curr[j-1]);
        }
        swap(prev, curr);
        fill(curr.begin(), curr.end(), 0);
    }
    return 2.0 * prev[m] / (n + m);
}

// Подсчет еще одной метрики 
double calcCosine(const map<string, int> &f1, const map<string, int> &f2) {
    if (f1.empty() && f2.empty()) return 1.0;
    if (f1.empty() || f2.empty()) return 0.0;

    double dot = 0, m1 = 0, m2 = 0;
    for (auto it = f1.begin(); it != f1.end(); it++) {
        m1 += (double)it->second * it->second;
        auto it2 = f2.find(it->first);
        if (it2 != f2.end())
            dot += (double)it->second * it2->second;
    }
    for (auto it = f2.begin(); it != f2.end(); it++)
        m2 += (double)it->second * it->second;

    if (m1 == 0 || m2 == 0) return 0;
    return dot / (sqrt(m1) * sqrt(m2));
}

double calcJaccard(const vector<string> &v1, const vector<string> &v2) {
    if (v1.empty() && v2.empty()) return 1.0;
    if (v1.empty() || v2.empty()) return 0.0;

    map<string, int> c1, c2;
    for (auto &s : v1) c1[s]++;
    for (auto &s : v2) c2[s]++;

    int inter = 0, uni = 0;
    for (auto it = c1.begin(); it != c1.end(); it++) {
        int cnt2 = c2.count(it->first) ? c2[it->first] : 0;
        inter += min(it->second, cnt2);
        uni += max(it->second, cnt2);
    }
    for (auto it = c2.begin(); it != c2.end(); it++) {
        if (!c1.count(it->first))
            uni += it->second;
    }
    return (double)inter / uni;
}

double calcSubtreeJaccard(const vector<size_t> &h1, const vector<size_t> &h2) {
    if (h1.empty() && h2.empty()) return 1.0;
    if (h1.empty() || h2.empty()) return 0.0;

    map<size_t, int> c1, c2;
    for (auto h : h1) c1[h]++;
    for (auto h : h2) c2[h]++;

    int inter = 0, uni = 0;
    for (auto it = c1.begin(); it != c1.end(); it++) {
        int cnt2 = c2.count(it->first) ? c2[it->first] : 0;
        inter += min(it->second, cnt2);
        uni += max(it->second, cnt2);
    }
    for (auto it = c2.begin(); it != c2.end(); it++) {
        if (!c1.count(it->first))
            uni += it->second;
    }
    return (double)inter / uni;
}

string readSourceFile(const string &path) {
    ifstream f(path);
    if (!f) {
        cerr << "Cannot open: " << path << endl;
        return "";
    }
    ostringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

// Специфичное для мака для разбора 
vector<string> detectSysrootArgs() {
    vector<string> args;
    FILE *p = popen("xcrun --show-sdk-path 2>/dev/null", "r");
    if (!p) return args;

    char buf[512];
    string sdk;
    while (fgets(buf, sizeof(buf), p))
        sdk += buf;
    pclose(p);

    while (!sdk.empty() && (sdk.back() == '\n' || sdk.back() == '\r'))
        sdk.pop_back();
    if (!sdk.empty())
        args.push_back("-isysroot" + sdk);
    return args;
}

//  Сам анализатор кода 
AnalysisResult analyzeCode(const string &sourceCode, const vector<string> &clangArgs) {
    AnalysisResult res;
    clang::tooling::runToolOnCodeWithArgs(make_unique<MyAction>(&res), sourceCode, clangArgs);
    buildSubtreeHashes(res);
    return res;
}

// Выдача результата 
CompareResult compare(const AnalysisResult &a, const AnalysisResult &b, const Weights &w) {
    CompareResult r;
    r.lcs = calcLCS(a.normNodes, b.normNodes);
    r.cosine = calcCosine(a.freqMap, b.freqMap);
    r.edgeJac = calcJaccard(a.edgePairs, b.edgePairs);
    r.subJac = calcSubtreeJaccard(a.subtreeHashes, b.subtreeHashes);
    r.cfJac = calcJaccard(a.cfProfile, b.cfProfile);

    r.combined = w.lcs * r.lcs + w.cosine * r.cosine + w.edgeJac * r.edgeJac
               + w.subJac * r.subJac + w.cfJac * r.cfJac;

    if (r.combined >= 0.85) r.verdict = "HIGH SIMILARITY - likely plagiarism";
    else if (r.combined >= 0.60) r.verdict = "MODERATE SIMILARITY - suspicious";
    else if (r.combined >= 0.35) r.verdict = "LOW SIMILARITY";
    else r.verdict = "UNLIKELY PLAGIARISM";

    return r;
}

/// Парсинг фукнций в коде .
vector<FunctionFingerprint> analyzeFunctions(const string &sourceCode, const vector<string> &clangArgs) {
    vector<FunctionFingerprint> funcs;
    clang::tooling::runToolOnCodeWithArgs(make_unique<FunctionCollectAction>(&funcs), sourceCode, clangArgs);
    return funcs;
}

// Сравнение функций 
vector<FunctionCompareResult> compareFunctions(const vector<FunctionFingerprint> &a,
                                               const vector<FunctionFingerprint> &b,
                                               const Weights &w) {
    vector<FunctionCompareResult> results;
    for (int i = 0; i < (int)a.size(); i++) {
        double bestScore = -1;
        int bestJ = -1;
        for (int j = 0; j < (int)b.size(); j++) {
            CompareResult cr = compare(a[i].result, b[j].result, w);
            if (cr.combined > bestScore) {
                bestScore = cr.combined;
                bestJ = j;
            }
        }
        if (bestJ >= 0) {
            FunctionCompareResult fcr;
            fcr.funcA = a[i].name + "/" + to_string(a[i].arity);
            fcr.funcB = b[bestJ].name + "/" + to_string(b[bestJ].arity);
            fcr.result = compare(a[i].result, b[bestJ].result, w);
            results.push_back(fcr);
        }
    }
    return results;
}

string exportCSV(const vector<string> &names, const vector<vector<CompareResult>> &matrix) {
    ostringstream out;
    out << fixed << setprecision(1);
    out << "File";
    for (auto &n : names) out << "," << n;
    out << endl;
    for (int i = 0; i < (int)names.size(); i++) {
        out << names[i];
        for (int j = 0; j < (int)names.size(); j++) {
            if (i == j)
                out << ",100.0";
            else if (i < j)
                out << "," << matrix[i][j].combined * 100;
            else
                out << "," << matrix[j][i].combined * 100;
        }
        out << endl;
    }
    return out.str();
}

string exportJSON(const vector<string> &names, const vector<vector<CompareResult>> &matrix) {
    ostringstream out;
    out << fixed << setprecision(1);
    out << "{" << endl << "  \"files\": [";
    for (int i = 0; i < (int)names.size(); i++) {
        if (i > 0) out << ", ";
        out << "\"" << names[i] << "\"";
    }
    out << "]," << endl << "  \"pairs\": [" << endl;
    bool first = true;
    for (int i = 0; i < (int)names.size(); i++) {
        for (int j = i + 1; j < (int)names.size(); j++) {
            if (!first) out << "," << endl;
            first = false;
            CompareResult r = matrix[i][j];
            out << "    {\"file1\": \"" << names[i] << "\", \"file2\": \"" << names[j] << "\", "
                << "\"lcs\": " << r.lcs * 100 << ", "
                << "\"cosine\": " << r.cosine * 100 << ", "
                << "\"edgeJac\": " << r.edgeJac * 100 << ", "
                << "\"subJac\": " << r.subJac * 100 << ", "
                << "\"cfJac\": " << r.cfJac * 100 << ", "
                << "\"combined\": " << r.combined * 100 << ", "
                << "\"verdict\": \"" << r.verdict << "\"}";
        }
    }
    out << endl << "  ]" << endl << "}" << endl;
    return out.str();
}
