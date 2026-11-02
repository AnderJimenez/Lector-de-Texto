#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool isWordCharacter(char character) {
    return std::isalnum(static_cast<unsigned char>(character)) != 0 ||
           static_cast<unsigned char>(character) >= 128;
}

enum class Kind { Atom, And, Or, Implication, Biconditional, Neither };

struct Formula {
    Kind kind = Kind::Atom;
    std::string name;
    std::shared_ptr<Formula> left;
    std::shared_ptr<Formula> right;
};

using FormulaPtr = std::shared_ptr<Formula>;

std::string lower(std::string value) {
    for (char& character : value) {
        if (static_cast<unsigned char>(character) < 128) {
            character = static_cast<char>(
                std::tolower(static_cast<unsigned char>(character)));
        }
    }
    return value;
}

void replaceAll(std::string& value, const std::string& search,
                const std::string& replacement) {
    std::size_t position = value.find(search);
    while (position != std::string::npos) {
        value.replace(position, search.size(), replacement);
        position = value.find(search, position + replacement.size());
    }
}

class Parser {
public:
    explicit Parser(std::string input) {
        input = lower(input);
        replaceAll(input, "por lo tanto", " -> ");
        replaceAll(input, "entonces", " -> ");
        replaceAll(input, "si ", " SI ");
        replaceAll(input, "si ", " SI ");
        replaceAll(input, "dado que", " <-> ");
        replaceAll(input, "ya que", " <-> ");
        replaceAll(input, "and", " AND ");
        replaceAll(input, "or", " OR ");
        replaceAll(input, " y ", " AND ");
        replaceAll(input, " o ", " OR ");
        replaceAll(input, " ni ", " NI ");
        replaceAll(input, ",", " COMMA ");
        replaceAll(input, ".", " ");
        for (std::size_t index = 0; index < input.size();) {
            if (std::isspace(static_cast<unsigned char>(input[index]))) {
                ++index;
            } else if (input.compare(index, 3, "<->") == 0) {
                tokens.push_back("<->");
                index += 3;
            } else if (input.compare(index, 2, "->") == 0) {
                tokens.push_back("->");
                index += 2;
            } else if (input[index] == '(' || input[index] == ')') {
                tokens.emplace_back(1, input[index++]);
            } else {
                std::size_t start = index;
                while (index < input.size() && isWordCharacter(input[index])) {
                    ++index;
                }
                if (start == index) {
                    error = "carácter no reconocido";
                    return;
                }
                tokens.push_back(input.substr(start, index - start));
            }
        }
        for (std::string& token : tokens) {
            if (token == "and") token = "AND";
            if (token == "or") token = "OR";
            if (token == "ni") token = "NI";
            if (token == "si") token = "SI";
        }
    }

    FormulaPtr parse() {
        FormulaPtr result = parseBiconditional();
        if (error.empty() && position != tokens.size()) {
            error = "símbolo inesperado: " + tokens[position];
        }
        return result;
    }

    std::string error;

private:
    std::vector<std::string> tokens;
    std::size_t position = 0;

    FormulaPtr make(Kind kind, FormulaPtr left = nullptr,
                    FormulaPtr right = nullptr) {
        auto result = std::make_shared<Formula>();
        result->kind = kind;
        result->left = left;
        result->right = right;
        return result;
    }

    bool take(const std::string& token) {
        if (position < tokens.size() && tokens[position] == token) {
            ++position;
            return true;
        }
        return false;
    }

    FormulaPtr parseBiconditional() {
        FormulaPtr left = parseImplication();
        if (take("<->")) return make(Kind::Biconditional, left, parseBiconditional());
        if (take("NI")) return make(Kind::Neither, left, parseBiconditional());
        return left;
    }

    FormulaPtr parseImplication() {
        if (take("SI")) {
            FormulaPtr premise = parseOr();
            if (!take("->") && !take("COMMA")) {
                error = "falta 'entonces' o una coma después del antecedente";
                return nullptr;
            }
            return make(Kind::Implication, premise, parseImplication());
        }
        FormulaPtr left = parseOr();
        if (take("->")) return make(Kind::Implication, left, parseImplication());
        return left;
    }

    FormulaPtr parseOr() {
        FormulaPtr result = parseAnd();
        while (take("OR")) result = make(Kind::Or, result, parseAnd());
        return result;
    }

    FormulaPtr parseAnd() {
        FormulaPtr result = parsePrimary();
        while (take("AND")) result = make(Kind::And, result, parsePrimary());
        return result;
    }

    FormulaPtr parsePrimary() {
        if (take("(")) {
            FormulaPtr result = parseBiconditional();
            if (!take(")")) error = "falta ')'";
            return result;
        }
        if (position >= tokens.size() || tokens[position] == "AND" ||
            tokens[position] == "OR" || tokens[position] == "->" ||
            tokens[position] == "<->" || tokens[position] == "NI" ||
            tokens[position] == "COMMA" || tokens[position] == ")") {
            error = "falta una proposición";
            return nullptr;
        }
        auto result = make(Kind::Atom);
        result->name = tokens[position++];
        while (position < tokens.size() && tokens[position] != "AND" &&
               tokens[position] != "OR" && tokens[position] != "->" &&
               tokens[position] != "<->" && tokens[position] != "NI" &&
               tokens[position] != "COMMA" && tokens[position] != ")") {
            result->name += " " + tokens[position++];
        }
        return result;
    }
};

void collectAtoms(const FormulaPtr& formula, std::vector<std::string>& atoms) {
    if (formula->kind == Kind::Atom) {
        if (std::find(atoms.begin(), atoms.end(), formula->name) == atoms.end()) {
            atoms.push_back(formula->name);
        }
        return;
    }
    collectAtoms(formula->left, atoms);
    collectAtoms(formula->right, atoms);
}

bool evaluate(const FormulaPtr& formula,
              const std::vector<std::string>& atoms, unsigned long row) {
    if (formula->kind == Kind::Atom) {
        const auto iterator = std::find(atoms.begin(), atoms.end(), formula->name);
        const std::size_t index = static_cast<std::size_t>(
            std::distance(atoms.begin(), iterator));
        return (row & (1UL << (atoms.size() - index - 1))) != 0;
    }
    const bool left = evaluate(formula->left, atoms, row);
    const bool right = evaluate(formula->right, atoms, row);
    if (formula->kind == Kind::And) return left && right;
    if (formula->kind == Kind::Or) return left || right;
    if (formula->kind == Kind::Implication) return !left || right;
    if (formula->kind == Kind::Neither) return !left && !right;
    return left == right;
}

void appendLine(std::string& output, const std::string& line) {
    output += line + "\r\n";
}

std::string makeResult(const std::string& input) {
    Parser parser(input);
    FormulaPtr formula = parser.parse();
    if (!formula || !parser.error.empty()) {
        return "No se pudo analizar la frase: " + parser.error + "\r\n";
    }
    std::vector<std::string> atoms;
    collectAtoms(formula, atoms);
    if (atoms.empty() || atoms.size() > 10) {
        return "La frase debe contener entre 1 y 10 proposiciones.\r\n";
    }

    std::string output;
    appendLine(output, "Relación detectada: " + input);
    appendLine(output, "Tabla de verdad (V = verdadero, F = falso)");
    const std::size_t columnWidth = 24;
    std::ostringstream table;
    table << std::left << std::setw(static_cast<int>(columnWidth)) << "Proposición";
    for (const std::string& atom : atoms) {
        table << std::left << std::setw(static_cast<int>(columnWidth)) << atom;
    }
    table << "Resultado";
    appendLine(output, table.str());

    const unsigned long rows = 1UL << atoms.size();
    for (unsigned long row = 0; row < rows; ++row) {
        table.str("");
        table.clear();
        for (std::size_t index = 0; index < atoms.size(); ++index) {
            table << std::left << std::setw(static_cast<int>(columnWidth))
                  << ((row & (1UL << (atoms.size() - index - 1))) ? "V" : "F");
        }
        table << std::string(columnWidth - 1, ' ')
              << (evaluate(formula, atoms, row) ? "V" : "F");
        appendLine(output, table.str());
    }
    appendLine(output, "La tabla se generó correctamente.");
    return output;
}

}  // namespace

#ifdef _WIN32
HWND inputBox;
HWND outputBox;
HFONT outputFont;

std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) return L"";
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(),
                                         static_cast<int>(value.size()),
                                         nullptr, 0);
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(),
                        static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string wideToUtf8(const std::wstring& value) {
    if (value.empty()) return "";
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                         static_cast<int>(value.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(),
                        static_cast<int>(value.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message,
                                 WPARAM wParam, LPARAM lParam) {
    if (message == WM_COMMAND && LOWORD(wParam) == 2) {
        int length = GetWindowTextLengthW(inputBox);
        std::wstring input(static_cast<std::size_t>(length) + 1, L'\0');
        GetWindowTextW(inputBox, input.data(), length + 1);
        input.resize(static_cast<std::size_t>(length));
        SetWindowTextW(outputBox, utf8ToWide(makeResult(wideToUtf8(input))).c_str());
        return 0;
    }
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    const wchar_t* className = L"ArbolSemanticoWindow";
    WNDCLASSW windowClass = {};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.lpszClassName = className;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&windowClass);

    HWND window = CreateWindowW(className, L"Árbol semántico - tabla de verdad",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                1200, 700, nullptr, nullptr, instance, nullptr);
    CreateWindowW(L"STATIC", L"Escribe una frase lógica:", WS_VISIBLE | WS_CHILD,
                  20, 20, 300, 25, window, nullptr, instance, nullptr);
    inputBox = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
                             20, 50, 1120, 30, window, nullptr, instance, nullptr);
    CreateWindowW(L"BUTTON", L"Analizar", WS_VISIBLE | WS_CHILD,
                  20, 95, 120, 35, window, reinterpret_cast<HMENU>(2),
                  instance, nullptr);
    outputBox = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER |
                              ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY |
                              WS_VSCROLL | ES_WANTRETURN, 20, 145, 1120, 500, window, nullptr,
                              instance, nullptr);
    outputFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             FIXED_PITCH | FF_MODERN, L"Consolas");
    SendMessageW(outputBox, WM_SETFONT, reinterpret_cast<WPARAM>(outputFont), TRUE);
    ShowWindow(window, showCommand);
    UpdateWindow(window);
    MSG message;
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return 0;
}
#else
int main() {
    return 0;
}
#endif
