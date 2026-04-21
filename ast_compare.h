#ifndef AST_COMPARE_H
#define AST_COMPARE_H

#include <map>
#include <string>
#include <vector>

using namespace std;

// Нода АСТ дерева 
struct TreeNode {
    string label;
    vector<int> children;
};

// Какую информацию мы будем считать во время анализа 
struct AnalysisResult {
    vector<string> normNodes;      // Поряд обхода dfs
    map<string, int> freqMap;      // Частота распределения
    vector<string> edgePairs;      // Дети данной вершины 
    vector<TreeNode> treeNodes;    // Отзеркаленное поддерево
    vector<size_t> subtreeHashes;  // Хэши, но если считать вверх
    vector<string> cfProfile;      // КФГ характеристика
};

// Ответ программы будем считать по несколько меткрикам 
struct CompareResult {
    double lcs = 0;
    double cosine = 0;
    double edgeJac = 0;
    double subJac = 0;
    double cfJac = 0;
    double combined = 0;
    string verdict;
};

// Структура для анализа функции 
struct FunctionFingerprint {
    string name;
    int arity;
    AnalysisResult result;
};

// Структура для сравнения функций по анализам
struct FunctionCompareResult {
    string funcA;
    string funcB;
    CompareResult result;
};

// Веса каждой метрики в вердикте 
struct Weights {
    double lcs = 0.30;
    double cosine = 0.20;
    double edgeJac = 0.15;
    double subJac = 0.25;
    double cfJac = 0.10;
};

string readSourceFile(const string &path);

//Специфичное для мака
vector<string> detectSysrootArgs();

// Проанализировать код 
AnalysisResult analyzeCode(const string &sourceCode, const vector<string> &clangArgs);

// Сравнить используя веса и результаты аналазиов 
  
CompareResult compare(const AnalysisResult &a, const AnalysisResult &b, const Weights &w = Weights());

// Проанализровать функции 
vector<FunctionFingerprint> analyzeFunctions(const string &sourceCode, const vector<string> &clangArgs);

// Сравнить функции используя веса и результаты анализов 

vector<FunctionCompareResult> compareFunctions(const vector<FunctionFingerprint> &a,
                                               const vector<FunctionFingerprint> &b,
                                               const Weights &w = Weights());

// В CSV
string exportCSV(const vector<string> &names, const vector<vector<CompareResult>> &matrix);

// В json
string exportJSON(const vector<string> &names, const vector<vector<CompareResult>> &matrix);

#endif
