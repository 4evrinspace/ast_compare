#include "ast_compare.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

//В данном файле описаны специфичные для красивого ввода и вывода функции и взаимодействие с консолью 



string DB_DIR = "./.ast_db";

string RED = "\033[31m";
string YELLOW = "\033[33m";
string GREEN = "\033[32m";
string RESET = "\033[0m";
bool useColor = true;

//Покрасить в зависмости от вердикта 
string colorVerdict(string verdict) {
    if (!useColor) return verdict;
    if (verdict.find("HIGH") != string::npos) return RED + verdict + RESET;
    if (verdict.find("MODERATE") != string::npos) return YELLOW + verdict + RESET;
    return GREEN + verdict + RESET;
}

/// Создание отедльной директории
void ensureDb() {
    if (!fs::exists(DB_DIR))
        fs::create_directories(DB_DIR);
}

// Все файлы в дирректории
vector<fs::path> getDbFiles() {
    vector<fs::path> files;
    if (!fs::exists(DB_DIR)) return files;
    for (auto &entry : fs::directory_iterator(DB_DIR)) {
        if (!entry.is_regular_file()) continue;
        string ext = entry.path().extension().string();
        if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" ||
            ext == ".h" || ext == ".hpp")
            files.push_back(entry.path());
    }
    sort(files.begin(), files.end());
    return files;
}

// Копирование файлов в диреткорию 
bool addFile(string src) {
    if (!fs::exists(src)) {
        cout << "  File not found: " << src << endl;
        return false;
    }
    ensureDb();
    fs::path dest = fs::path(DB_DIR) / fs::path(src).filename();
    if (fs::exists(dest))
        cout << "  Warning: " << dest.filename().string() << " already exists, overwriting." << endl;
    fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
    cout << "  Added: " << dest.filename().string() << endl;
    return true;
}

string trim(string s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
    return s;
}

// Скнипорвание весов для метрик 
Weights parseWeights(string s) {
    Weights w;
    vector<double> vals;
    istringstream ss(s);
    string tok;
    while (getline(ss, tok, ',')) {
        vals.push_back(atof(tok.c_str()));
    }
    if (vals.size() != 5) {
        cout << "  Warning: expected 5 comma-separated weights, got " << vals.size()
             << ". Using defaults." << endl;
        return w;
    }
    w.lcs = vals[0];
    w.cosine = vals[1];
    w.edgeJac = vals[2];
    w.subJac = vals[3];
    w.cfJac = vals[4];
    return w;
}

// Вывод сравнений 
void printResult(string nameA, string nameB, CompareResult r) {
    cout << endl;
    cout << "  " << nameA << " vs " << nameB << endl;
    cout << fixed << setprecision(1);
    cout << "    LCS: " << r.lcs * 100 << "%"
         << "  Cos: " << r.cosine * 100 << "%"
         << "  Edge: " << r.edgeJac * 100 << "%"
         << "  Sub: " << r.subJac * 100 << "%"
         << "  CF: " << r.cfJac * 100 << "%" << endl;
    cout << "    Combined: " << r.combined * 100 << "%"
         << "  ->  " << colorVerdict(r.verdict) << endl;
}

// Еще более красивый вывод
void printMatrix(vector<string> &names, vector<vector<CompareResult>> &matrix) {
    int n = names.size();
    int nameWidth = 12;
    int colWidth = 7;

    cout << endl << "  === Similarity Matrix ===" << endl << endl;
    cout << "  " << setw(nameWidth) << left << "";
    for (int j = 0; j < n; j++) {
        string shortName = names[j];
        if ((int)shortName.size() > colWidth)
            shortName = shortName.substr(0, colWidth - 1) + "~";
        cout << setw(colWidth) << right << shortName;
    }
    cout << endl;

    cout << fixed << setprecision(0);
    for (int i = 0; i < n; i++) {
        string rowName = names[i];
        if ((int)rowName.size() > nameWidth - 1)
            rowName = rowName.substr(0, nameWidth - 2) + "~";
        cout << "  " << setw(nameWidth) << left << rowName;
        for (int j = 0; j < n; j++) {
            if (i == j)
                cout << setw(colWidth) << right << "---";
            else if (i < j)
                cout << setw(colWidth - 1) << right << (int)(matrix[i][j].combined * 100) << "%";
            else
                cout << setw(colWidth - 1) << right << (int)(matrix[j][i].combined * 100) << "%";
        }
        cout << endl;
    }
}

// Добавление файла в дирректорию 
void addDocument() {
    cout << "  Enter file path: ";
    string path;
    getline(cin, path);
    path = trim(path);
    if (path.empty()) { cout << "  Empty path." << endl; return; }
    addFile(path);
}

// Добавление нескольких файлов 
void addFromList() {
    cout << "  Enter list file path: ";
    string listPath;
    getline(cin, listPath);
    listPath = trim(listPath);

    ifstream f(listPath);
    if (!f) { cout << "  Cannot open: " << listPath << endl; return; }

    string line;
    int count = 0;
    while (getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (addFile(line)) count++;
    }
    cout << "  Total added: " << count << endl;
}

// Добавление всех файлов из данной дирректории
void addFromDirectory() {
    cout << "  Enter directory path: ";
    string dirPath;
    getline(cin, dirPath);
    dirPath = trim(dirPath);

    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        cout << "  Not a valid directory: " << dirPath << endl;
        return;
    }

    int count = 0;
    for (auto &entry : fs::recursive_directory_iterator(dirPath)) {
        if (!entry.is_regular_file()) continue;
        string ext = entry.path().extension().string();
        if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" ||
            ext == ".h" || ext == ".hpp") {
            if (addFile(entry.path().string())) count++;
        }
    }
    cout << "  Total added: " << count << endl;
}

// Анализ для всех файлов в директории куда копировали со всеми
void checkAll(Weights w = Weights(), string exportFmt = "") {
    auto files = getDbFiles();
    if (files.size() < 2) {
        cout << "  Need at least 2 files in database." << endl;
        return;
    }

    auto args = detectSysrootArgs();

    struct Entry { string name; AnalysisResult res; };
    vector<Entry> entries;
    for (int i = 0; i < (int)files.size(); i++) {
        cout << "\r  Analyzing file " << i + 1 << "/" << files.size() << "..." << flush;
        string code = readSourceFile(files[i].string());
        if (code.empty()) continue;
        entries.push_back({files[i].filename().string(), analyzeCode(code, args)});
    }
    cout << "\r  Analyzed " << entries.size() << " files.          " << endl;

    int n = entries.size();
    vector<vector<CompareResult>> matrix(n, vector<CompareResult>(n));

    struct Pair { string a, b; CompareResult r; };
    vector<Pair> pairs;
    int totalPairs = n * (n - 1) / 2;
    int pairIdx = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            pairIdx++;
            cout << "\r  Comparing pair " << pairIdx << "/" << totalPairs << "..." << flush;
            CompareResult cr = compare(entries[i].res, entries[j].res, w);
            matrix[i][j] = cr;
            pairs.push_back({entries[i].name, entries[j].name, cr});
        }
    }
    cout << "\r  Compared " << totalPairs << " pairs.            " << endl;

    sort(pairs.begin(), pairs.end(),
         [](Pair &a, Pair &b) { return a.r.combined > b.r.combined; });

    cout << endl << "  === Results (sorted by score) ===" << endl;
    for (auto &p : pairs)
        printResult(p.a, p.b, p.r);

    vector<string> names;
    for (auto &e : entries) names.push_back(e.name);
    printMatrix(names, matrix);

    if (exportFmt == "csv") {
        string csv = exportCSV(names, matrix);
        cout << endl << "  === CSV Export ===" << endl << csv;
    } else if (exportFmt == "json") {
        string json = exportJSON(names, matrix);
        cout << endl << "  === JSON Export ===" << endl << json;
    }
}

// Сравнение одного файла с другими всеми
void checkSpecific(Weights w = Weights()) {
    auto files = getDbFiles();
    if (files.size() < 2) {
        cout << "  Need at least 2 files in database." << endl;
        return;
    }

    cout << "  Files in database:" << endl;
    for (int i = 0; i < (int)files.size(); i++)
        cout << "    " << i + 1 << ". " << files[i].filename().string() << endl;

    cout << "  Enter file number: ";
    string input;
    getline(cin, input);
    int idx = atoi(input.c_str()) - 1;
    if (idx < 0 || idx >= (int)files.size()) {
        cout << "  Invalid selection." << endl;
        return;
    }

    auto args = detectSysrootArgs();
    cout << "\r  Analyzing target file..." << flush;
    string code = readSourceFile(files[idx].string());
    if (code.empty()) return;
    AnalysisResult target = analyzeCode(code, args);
    string targetName = files[idx].filename().string();

    cout << "\r  === Comparing " << targetName << " against others ===" << endl;
    int total = files.size() - 1;
    int cur = 0;
    for (int i = 0; i < (int)files.size(); i++) {
        if (i == idx) continue;
        cur++;
        cout << "\r  Comparing " << cur << "/" << total << "..." << flush;
        string c = readSourceFile(files[i].string());
        if (c.empty()) continue;
        AnalysisResult r = analyzeCode(c, args);
        printResult(targetName, files[i].filename().string(), compare(target, r, w));
    }
    cout << endl;
}

// Внешний файл со всеми файлами
void checkExternal(Weights w = Weights()) {
    cout << "  Enter file path: ";
    string path;
    getline(cin, path);
    path = trim(path);

    if (!fs::exists(path)) {
        cout << "  File not found: " << path << endl;
        return;
    }

    auto files = getDbFiles();
    if (files.empty()) {
        cout << "  Database is empty." << endl;
        return;
    }

    auto args = detectSysrootArgs();
    cout << "\r  Analyzing external file..." << flush;
    string code = readSourceFile(path);
    if (code.empty()) return;
    AnalysisResult ext = analyzeCode(code, args);
    string name = fs::path(path).filename().string();

    cout << "\r  === Comparing " << name << " against database ===" << endl;
    int total = files.size();
    int cur = 0;
    for (auto &f : files) {
        cur++;
        cout << "\r  Comparing " << cur << "/" << total << "..." << flush;
        string c = readSourceFile(f.string());
        if (c.empty()) continue;
        AnalysisResult r = analyzeCode(c, args);
        printResult(name, f.filename().string(), compare(ext, r, w));
    }
    cout << endl;
}

// Сравнить файлы по функциям
void checkFunctionLevel(string pathA = "", string pathB = "", Weights w = Weights()) {
    if (pathA.empty()) {
        cout << "  Enter first file path: ";
        getline(cin, pathA);
        pathA = trim(pathA);
    }
    if (pathB.empty()) {
        cout << "  Enter second file path: ";
        getline(cin, pathB);
        pathB = trim(pathB);
    }

    if (!fs::exists(pathA)) { cout << "  File not found: " << pathA << endl; return; }
    if (!fs::exists(pathB)) { cout << "  File not found: " << pathB << endl; return; }

    auto args = detectSysrootArgs();
    string codeA = readSourceFile(pathA);
    string codeB = readSourceFile(pathB);
    if (codeA.empty() || codeB.empty()) return;

    cout << "  Analyzing functions..." << flush;
    auto funcsA = analyzeFunctions(codeA, args);
    auto funcsB = analyzeFunctions(codeB, args);
    cout << "\r  Found " << funcsA.size() << " functions in file A, "
         << funcsB.size() << " in file B.      " << endl;

    if (funcsA.empty() || funcsB.empty()) {
        cout << "  One of the files has no top-level functions." << endl;
        return;
    }

    auto results = compareFunctions(funcsA, funcsB, w);

    string nameA = fs::path(pathA).filename().string();
    string nameB = fs::path(pathB).filename().string();

    cout << endl << "  === Function-level comparison: " << nameA << " vs " << nameB << " ===" << endl;
    for (auto &fcr : results) {
        cout << endl;
        cout << "  " << fcr.funcA << " <-> " << fcr.funcB << endl;
        cout << fixed << setprecision(1);
        cout << "    LCS: " << fcr.result.lcs * 100 << "%"
             << "  Cos: " << fcr.result.cosine * 100 << "%"
             << "  Edge: " << fcr.result.edgeJac * 100 << "%"
             << "  Sub: " << fcr.result.subJac * 100 << "%"
             << "  CF: " << fcr.result.cfJac * 100 << "%" << endl;
        cout << "    Combined: " << fcr.result.combined * 100 << "%"
             << "  ->  " << colorVerdict(fcr.result.verdict) << endl;
    }
}

// Удалить файл из базы данных 
void removeFile() {
    auto files = getDbFiles();
    if (files.empty()) {
        cout << "  Database is empty." << endl;
        return;
    }

    cout << "  Files in database:" << endl;
    for (int i = 0; i < (int)files.size(); i++)
        cout << "    " << i + 1 << ". " << files[i].filename().string() << endl;

    cout << "  Enter file number to remove: ";
    string input;
    getline(cin, input);
    int idx = atoi(input.c_str()) - 1;
    if (idx < 0 || idx >= (int)files.size()) {
        cout << "  Invalid selection." << endl;
        return;
    }

    string name = files[idx].filename().string();
    fs::remove(files[idx]);
    cout << "  Removed: " << name << endl;
}

void removeFileByName(string name) {
    fs::path target = fs::path(DB_DIR) / name;
    if (!fs::exists(target)) {
        target = fs::path(DB_DIR) / fs::path(name).filename();
    }
    if (fs::exists(target)) {
        fs::remove(target);
        cout << "  Removed: " << target.filename().string() << endl;
    } else {
        cout << "  File not found in database: " << name << endl;
    }
}

// Все файлы удалить 
void clearDb() {
    cout << "  This will remove ALL files from the database. Are you sure? (y/N): ";
    string input;
    getline(cin, input);
    input = trim(input);
    if (input != "y" && input != "Y") {
        cout << "  Cancelled." << endl;
        return;
    }

    auto files = getDbFiles();
    int count = 0;
    for (auto &f : files) {
        fs::remove(f);
        count++;
    }
    cout << "  Removed " << count << " files." << endl;
}

// Вывести все файлы 
void listDb() {
    auto files = getDbFiles();
    if (files.empty()) {
        cout << "  Database is empty." << endl;
        return;
    }
    cout << "  Database (" << files.size() << " files):" << endl;
    for (auto &f : files)
        cout << "    " << f.filename().string() << endl;
}

// Help
void printUsage() {
    cout << "Usage: ast_compare [options]" << endl;
    cout << endl;
    cout << "  --add <file>             Add a file to the database" << endl;
    cout << "  --add-dir <dir>          Add all C/C++ files from a directory (recursive)" << endl;
    cout << "  --remove <file>          Remove a file from the database" << endl;
    cout << "  --clear                  Clear the entire database" << endl;
    cout << "  --list                   List database contents" << endl;
    cout << "  --check <file>           Check an external file against the database" << endl;
    cout << "  --check-all              Check all database files against each other" << endl;
    cout << "  --compare <f1> <f2>      Compare two files directly" << endl;
    cout << "  --function-level         Modifier: use function-level comparison with --compare" << endl;
    cout << "  --weights <l,c,e,s,f>    Modifier: set metric weights (5 comma-separated values)" << endl;
    cout << "  --export csv|json        Modifier: export results for --check-all" << endl;
    cout << "  --no-color               Disable colored output" << endl;
    cout << "  --help                   Show this help" << endl;
}

// Обработка аргументов 
void handleArgs(int argc, char *argv[]) {
    string action;
    string file1, file2;
    string exportFmt;
    string weightStr;
    bool funcLevel = false;
    Weights w;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--help") { printUsage(); return; }
        else if (arg == "--no-color") { useColor = false; }
        else if (arg == "--function-level") { funcLevel = true; }
        else if (arg == "--add" && i + 1 < argc) { action = "add"; file1 = argv[++i]; }
        else if (arg == "--add-dir" && i + 1 < argc) { action = "add-dir"; file1 = argv[++i]; }
        else if (arg == "--remove" && i + 1 < argc) { action = "remove"; file1 = argv[++i]; }
        else if (arg == "--clear") { action = "clear"; }
        else if (arg == "--list") { action = "list"; }
        else if (arg == "--check" && i + 1 < argc) { action = "check"; file1 = argv[++i]; }
        else if (arg == "--check-all") { action = "check-all"; }
        else if (arg == "--compare" && i + 2 < argc) { action = "compare"; file1 = argv[++i]; file2 = argv[++i]; }
        else if (arg == "--weights" && i + 1 < argc) { weightStr = argv[++i]; }
        else if (arg == "--export" && i + 1 < argc) { exportFmt = argv[++i]; }
        else {
            cout << "Unknown option: " << arg << endl;
            printUsage();
            return;
        }
    }

    if (!weightStr.empty()) w = parseWeights(weightStr);

    if (action == "add") {
        addFile(file1);
    } else if (action == "add-dir") {
        if (!fs::exists(file1) || !fs::is_directory(file1)) {
            cout << "  Not a valid directory: " << file1 << endl;
            return;
        }
        int count = 0;
        for (auto &entry : fs::recursive_directory_iterator(file1)) {
            if (!entry.is_regular_file()) continue;
            string ext = entry.path().extension().string();
            if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" ||
                ext == ".h" || ext == ".hpp") {
                if (addFile(entry.path().string())) count++;
            }
        }
        cout << "  Total added: " << count << endl;
    } else if (action == "remove") {
        removeFileByName(file1);
    } else if (action == "clear") {
        auto files = getDbFiles();
        int count = 0;
        for (auto &f : files) { fs::remove(f); count++; }
        cout << "  Cleared " << count << " files from database." << endl;
    } else if (action == "list") {
        listDb();
    } else if (action == "check") {
        if (!fs::exists(file1)) {
            cout << "  File not found: " << file1 << endl;
            return;
        }
        auto files = getDbFiles();
        if (files.empty()) { cout << "  Database is empty." << endl; return; }
        auto args = detectSysrootArgs();
        string code = readSourceFile(file1);
        if (code.empty()) return;
        AnalysisResult ext = analyzeCode(code, args);
        string name = fs::path(file1).filename().string();
        cout << endl << "  === Comparing " << name << " against database ===" << endl;
        for (auto &f : files) {
            string c = readSourceFile(f.string());
            if (c.empty()) continue;
            AnalysisResult r = analyzeCode(c, args);
            printResult(name, f.filename().string(), compare(ext, r, w));
        }
    } else if (action == "check-all") {
        checkAll(w, exportFmt);
    } else if (action == "compare") {
        if (funcLevel) {
            checkFunctionLevel(file1, file2, w);
        } else {
            if (!fs::exists(file1)) { cout << "  File not found: " << file1 << endl; return; }
            if (!fs::exists(file2)) { cout << "  File not found: " << file2 << endl; return; }
            auto args = detectSysrootArgs();
            string codeA = readSourceFile(file1);
            string codeB = readSourceFile(file2);
            if (codeA.empty() || codeB.empty()) return;
            AnalysisResult a = analyzeCode(codeA, args);
            AnalysisResult b = analyzeCode(codeB, args);
            printResult(fs::path(file1).filename().string(),
                        fs::path(file2).filename().string(),
                        compare(a, b, w));
        }
    } else {
        printUsage();
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        handleArgs(argc, argv);
        return 0;
    }

    while (true) {
        cout << endl;
        cout << "=== AST Anti-Plagiarism Tool ===" << endl;
        cout << endl;
        cout << "  1.  Add a document to the database" << endl;
        cout << "  2.  Add documents from a list file" << endl;
        cout << "  3.  Add all C/C++ files from a directory" << endl;
        cout << "  4.  Check all database files against each other" << endl;
        cout << "  5.  Check a specific database file against all others" << endl;
        cout << "  6.  Check an external file against the database" << endl;
        cout << "  7.  Function-level comparison of two files" << endl;
        cout << "  8.  Remove a file from the database" << endl;
        cout << "  9.  Clear entire database" << endl;
        cout << "  10. List database contents" << endl;
        cout << "  11. Exit" << endl;
        cout << endl;
        cout << "  Choose [1-11]: ";

        string choice;
        if (!getline(cin, choice)) break;

        if (choice == "1") addDocument();
        else if (choice == "2") addFromList();
        else if (choice == "3") addFromDirectory();
        else if (choice == "4") checkAll();
        else if (choice == "5") checkSpecific();
        else if (choice == "6") checkExternal();
        else if (choice == "7") checkFunctionLevel();
        else if (choice == "8") removeFile();
        else if (choice == "9") clearDb();
        else if (choice == "10") listDb();
        else if (choice == "11") break;
        else cout << "  Invalid choice." << endl;
    }
    return 0;
}
