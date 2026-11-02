// IMPLEMENTATION_FILE
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

struct Token {
    std::string text;
    std::string word;
    std::string category;
    bool wordToken;
};

namespace {

const std::string RESET = "\033[0m";

bool isWordCharacter(char character) {
    return std::isalnum(static_cast<unsigned char>(character)) != 0 ||
           static_cast<unsigned char>(character) >= 128;
}

std::string lowerAscii(std::string value) {
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

bool isBoundary(const std::string& value, std::size_t position,
                std::size_t length) {
    const bool before = position == 0 ||
        !isWordCharacter(value[position - 1]);
    const std::size_t end = position + length;
    const bool after = end >= value.size() ||
        !isWordCharacter(value[end]);
    return before && after;
}

void replaceWord(std::string& value, const std::string& search,
                 const std::string& replacement) {
    std::size_t position = 0;
    std::string lowered = lowerAscii(value);
    position = lowered.find(search, position);
    while (position != std::string::npos) {
        if (isBoundary(lowered, position, search.size())) {
            value.replace(position, search.size(), replacement);
            lowered = lowerAscii(value);
            position += replacement.size();
        } else {
            position += search.size();
        }
        position = lowered.find(search, position);
    }
}

std::vector<Token> tokenizeText(const std::string& input) {
    std::vector<Token> tokens;
    std::string current;
    auto flush = [&]() {
        if (!current.empty()) {
            tokens.push_back({current, lowerAscii(current), "", true});
            current.clear();
        }
    };
    for (char character : input) {
        if (std::isspace(static_cast<unsigned char>(character)) ||
            character == ',' || character == '.') {
            flush();
        } else if (std::ispunct(static_cast<unsigned char>(character)) &&
                   character != '-' && character != '\'') {
            flush();
            tokens.push_back({std::string(1, character), "", "Puntuación", false});
        } else {
            current += character;
        }
    }
    flush();
    return tokens;
}

const std::unordered_set<std::string> PREPOSITIONS = {
    "a", "ante", "bajo", "con", "contra", "de", "desde", "durante", "en",
    "entre", "hacia", "hasta", "para", "por", "sin", "sobre", "tras"
};
const std::unordered_set<std::string> ARTICLES = {
    "el", "la", "los", "las", "un", "una", "unos", "unas", "lo", "al", "del"
};
const std::unordered_set<std::string> PRONOUNS = {
    "yo", "tú", "tu", "él", "ella", "ello", "nosotros", "nosotras",
    "vosotros", "vosotras", "ellos", "ellas", "me", "te", "se", "nos",
    "os", "le", "les", "mí", "ti", "sí"
};
const std::unordered_set<std::string> ADVERBS = {
    "aquí", "ahí", "allí", "ahora", "ayer", "hoy", "mañana", "muy", "más",
    "menos", "bien", "mal", "sí", "nunca", "siempre", "ya", "también",
    "solo", "sólo", "quizá", "quizás", "no"
};
const std::unordered_set<std::string> VERBS = {
    "es", "soy", "eres", "somos", "son", "está", "estoy", "estás",
    "estamos", "están", "hay", "hace", "hacen", "tiene", "tienen",
    "puede", "pueden", "debe", "deben", "va", "van", "viene", "ve",
    "dice", "sabe", "lee", "escribe", "analiza", "ama", "come", "vive",
    "estudia", "trabaja", "funciona", "corre", "corren", "habla", "hablan",
    "llega", "llegan", "quiere", "quieren", "conoce", "conocen", "usa",
    "usan"
};

bool looksLikeVerb(const std::string& word) {
    if (VERBS.count(word) != 0) return true;
    const std::vector<std::string> endings = {
        "ar", "er", "ir", "ando", "iendo", "ado", "ido", "aba", "ía",
        "arán", "erán", "irán", "aría", "ería", "iría", "é", "aste",
        "iste", "ó", "imos", "ieron"
    };
    for (const std::string& ending : endings) {
        if (word.size() > ending.size() &&
            word.compare(word.size() - ending.size(), ending.size(), ending) == 0) {
            return true;
        }
    }
    return false;
}

void classify(std::vector<Token>& tokens) {
    bool verbSeen = false;
    for (Token& token : tokens) {
        if (!token.wordToken) continue;
        if (PREPOSITIONS.count(token.word) != 0) token.category = "Preposición";
        else if (ARTICLES.count(token.word) != 0) token.category = "Artículo";
        else if (PRONOUNS.count(token.word) != 0) token.category = "Pronombre";
        else if (ADVERBS.count(token.word) != 0) token.category = "Adverbio";
        else if (looksLikeVerb(token.word)) {
            token.category = "Verbo";
            verbSeen = true;
        } else {
            token.category = verbSeen ? "Palabra" : "Sujeto";
        }
    }
}

void printText(const std::vector<Token>& tokens, bool colors) {
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        const Token& token = tokens[index];
        if (!token.wordToken || !colors) std::cout << token.text;
        else {
            std::string color = "\033[1;37m";
            if (token.category == "Sujeto") color = "\033[1;36m";
            if (token.category == "Verbo") color = "\033[1;32m";
            if (token.category == "Nexo") color = "\033[1;33m";
            std::cout << color << token.text << RESET;
        }
        if (index + 1 < tokens.size() && tokens[index + 1].wordToken) {
            std::cout << ' ';
        }
    }
    std::cout << "\n";
}

struct Formula {
    enum class Kind { Proposition, Not, And, Or, Implication, Biconditional, Neither };
    Kind kind = Kind::Proposition;
    std::string name;
    std::shared_ptr<Formula> left;
    std::shared_ptr<Formula> right;
};
using FormulaPtr = std::shared_ptr<Formula>;

FormulaPtr makeFormula(Formula::Kind kind, const FormulaPtr& left = nullptr,
                       const FormulaPtr& right = nullptr) {
    auto result = std::make_shared<Formula>();
    result->kind = kind;
    result->left = left;
    result->right = right;
    return result;
}

class FormulaParser {
public:
    explicit FormulaParser(const std::string& input) {
        std::string normalized = lowerAscii(input);
        replaceAll(normalized, "(and)", " AND ");
        replaceAll(normalized, "(or)", " OR ");
        replaceAll(normalized, "(->)", " -> ");
        replaceAll(normalized, "(<->)", " <-> ");
        replaceAll(normalized, "si y solo si", " <-> ");
        replaceAll(normalized, "si solo si", " <-> ");
        replaceAll(normalized, "dado que", " <-> ");
        replaceAll(normalized, "ya que", " <-> ");
        replaceAll(normalized, "por lo tanto", " THEN ");
        replaceAll(normalized, "entonces", " THEN ");
        replaceAll(normalized, "<->", " BICOND ");
        replaceAll(normalized, "->", " -> ");
        replaceAll(normalized, "BICOND", " <-> ");
        replaceAll(normalized, "(", " ( ");
        replaceAll(normalized, ")", " ) ");
        replaceAll(normalized, ",", " COMMA ");
        replaceWord(normalized, "and", " AND ");
        replaceWord(normalized, "or", " OR ");
        replaceWord(normalized, "y", " AND ");
        replaceWord(normalized, "e", " AND ");
        replaceWord(normalized, "o", " OR ");
        replaceWord(normalized, "u", " OR ");
        replaceWord(normalized, "no", " NOT ");
        replaceWord(normalized, "ni", " NI ");
        replaceWord(normalized, "si", " SI ");
        replaceWord(normalized, "and", " AND ");
        std::istringstream stream(normalized);
        std::string token;
        while (stream >> token) tokens.push_back(token);
    }

    FormulaPtr parse() {
        FormulaPtr result = parseImplication();
        if (!result && error.empty()) error = "fórmula vacía";
        if (error.empty() && position != tokens.size()) {
            error = "símbolo inesperado: " + tokens[position];
        }
        return result;
    }

    const std::string& getError() const { return error; }

private:
    std::vector<std::string> tokens;
    std::size_t position = 0;
    std::string error;

    bool take(const std::string& token) {
        if (position < tokens.size() && tokens[position] == token) {
            ++position;
            return true;
        }
        return false;
    }

    FormulaPtr fail(const std::string& message) {
        if (error.empty()) error = message;
        return nullptr;
    }

    FormulaPtr parseImplication() {
        if (take("SI")) {
            FormulaPtr premise = parseOr();
            if (!take("THEN") && !take("COMMA")) {
                return fail("falta 'entonces' o una coma después del antecedente");
            }
            FormulaPtr conclusion = parseImplication();
            if (!conclusion) return fail("falta la conclusión de la regla");
            return makeFormula(Formula::Kind::Implication, premise, conclusion);
        }
        FormulaPtr left = parseOr();
        if (take("->")) {
            FormulaPtr right = parseImplication();
            if (!right) return fail("falta la conclusión de la implicación");
            return makeFormula(Formula::Kind::Implication, left, right);
        }
        return left;
    }

    FormulaPtr parseOr() {
        FormulaPtr result = parseAnd();
        while (take("OR")) {
            FormulaPtr right = parseAnd();
            if (!right) return fail("falta un operando para OR");
            result = makeFormula(Formula::Kind::Or, result, right);
        }
        if (take("<->")) {
            FormulaPtr right = parseOr();
            if (!right) return fail("falta un operando para ↔");
            result = makeFormula(Formula::Kind::Biconditional, result, right);
        }
        if (take("NI")) {
            FormulaPtr right = parseOr();
            if (!right) return fail("falta un operando para NI");
            result = makeFormula(Formula::Kind::Neither, result, right);
        }
        return result;
    }

    FormulaPtr parseAnd() {
        FormulaPtr result = parseUnary();
        while (take("AND")) {
            FormulaPtr right = parseUnary();
            if (!right) return fail("falta un operando para AND");
            result = makeFormula(Formula::Kind::And, result, right);
        }
        return result;
    }

    FormulaPtr parseUnary() {
        if (take("NOT")) {
            FormulaPtr operand = parseUnary();
            if (!operand) return fail("falta el operando de la negación");
            return makeFormula(Formula::Kind::Not, operand);
        }
        if (take("(")) {
            FormulaPtr result = parseImplication();
            if (!take(")")) return fail("falta ')'");
            return result;
        }
        if (position >= tokens.size() || tokens[position] == "AND" ||
            tokens[position] == "OR" || tokens[position] == "THEN" ||
            tokens[position] == "NI" || tokens[position] == ")" ||
            tokens[position] == "<->" || tokens[position] == "->" ||
            tokens[position] == "COMMA") {
            return nullptr;
        }
        std::string name;
        while (position < tokens.size() && tokens[position] != "AND" &&
               tokens[position] != "OR" && tokens[position] != "THEN" &&
               tokens[position] != ")" && tokens[position] != "<->" &&
               tokens[position] != "->" && tokens[position] != "NI" &&
               tokens[position] != "COMMA") {
            if (!name.empty()) name += ' ';
            name += tokens[position++];
        }
        auto result = makeFormula(Formula::Kind::Proposition);
        result->name = name;
        return result;
    }
};

void collectAtoms(const FormulaPtr& formula, std::vector<std::string>& atoms) {
    if (formula->kind == Formula::Kind::Proposition) {
        if (std::find(atoms.begin(), atoms.end(), formula->name) == atoms.end()) {
            atoms.push_back(formula->name);
        }
        return;
    }
    collectAtoms(formula->left, atoms);
    if (formula->right) collectAtoms(formula->right, atoms);
}

int precedence(Formula::Kind kind) {
    if (kind == Formula::Kind::Biconditional) return 1;
    if (kind == Formula::Kind::Implication) return 2;
    if (kind == Formula::Kind::Or) return 3;
    if (kind == Formula::Kind::And) return 4;
    if (kind == Formula::Kind::Not) return 5;
    return 6;
}

std::string atomLetter(std::size_t index) {
    if (index < 10) return std::string(1, static_cast<char>('p' + index));
    return "p" + std::to_string(index - 9);
}

std::string formatFormula(const FormulaPtr& formula,
                          const std::map<std::string, std::string>& letters,
                          int parentPrecedence = 0) {
    if (formula->kind == Formula::Kind::Proposition) return letters.at(formula->name);
    if (formula->kind == Formula::Kind::Not) {
        std::string value = "¬" + formatFormula(formula->left, letters, 5);
        if (formula->left->kind != Formula::Kind::Proposition &&
            formula->left->kind != Formula::Kind::Not) value = "¬(" +
                formatFormula(formula->left, letters) + ")";
        return value;
    }
    const int current = precedence(formula->kind);
    std::string symbol;
    if (formula->kind == Formula::Kind::And) symbol = "∧";
    else if (formula->kind == Formula::Kind::Or) symbol = "∨";
    else if (formula->kind == Formula::Kind::Implication) symbol = "→";
    else if (formula->kind == Formula::Kind::Biconditional) symbol = "↔";
    else symbol = "NI";
    std::string value = formatFormula(formula->left, letters, current) + " " +
        symbol + " " +
        formatFormula(formula->right, letters, current);
    if (current < parentPrecedence) return "(" + value + ")";
    return value;
}

struct SignedFormula {
    FormulaPtr formula;
    bool value;
    bool applied = false;
};
using Branch = std::vector<SignedFormula>;

bool isLiteral(const SignedFormula& item) {
    return item.formula->kind == Formula::Kind::Proposition;
}

bool closes(const Branch& branch) {
    std::unordered_map<std::string, bool> values;
    for (const SignedFormula& item : branch) {
        if (!isLiteral(item)) continue;
        auto found = values.find(item.formula->name);
        if (found != values.end() && found->second != item.value) return true;
        values[item.formula->name] = item.value;
    }
    return false;
}

void printSigned(const SignedFormula& item,
                 const std::map<std::string, std::string>& letters) {
    std::cout << (item.value ? "  " : "  ¬")
              << formatFormula(item.formula, letters)
              << (item.applied ? " [aplicada]" : "");
}

std::vector<Branch> expandBranch(Branch branch,
                                 const std::map<std::string, std::string>& letters) {
    (void)letters;
    for (SignedFormula& item : branch) {
        if (item.applied || isLiteral(item)) continue;
        item.applied = true;
        const auto kind = item.formula->kind;
        if (kind == Formula::Kind::Not) {
            branch.push_back({item.formula->left, !item.value, false});
            return {branch};
        }
        const FormulaPtr& left = item.formula->left;
        const FormulaPtr& right = item.formula->right;
        if (kind == Formula::Kind::And && item.value) {
            branch.push_back({left, true});
            branch.push_back({right, true});
            return {branch};
        }
        if (kind == Formula::Kind::Or && !item.value) {
            branch.push_back({left, false});
            branch.push_back({right, false});
            return {branch};
        }
        if (kind == Formula::Kind::Implication && !item.value) {
            branch.push_back({left, true});
            branch.push_back({right, false});
            return {branch};
        }
        if (kind == Formula::Kind::And && !item.value) {
            Branch first = branch;
            Branch second = branch;
            first.push_back({left, false});
            second.push_back({right, false});
            return {first, second};
        }
        if (kind == Formula::Kind::Or && item.value) {
            Branch first = branch;
            Branch second = branch;
            first.push_back({left, true});
            second.push_back({right, true});
            return {first, second};
        }
        if (kind == Formula::Kind::Implication && item.value) {
            Branch first = branch;
            Branch second = branch;
            first.push_back({left, false});
            second.push_back({right, true});
            return {first, second};
        }
        if (kind == Formula::Kind::Neither && item.value) {
            branch.push_back({left, false});
            branch.push_back({right, false});
            return {branch};
        }
        if (kind == Formula::Kind::Neither && !item.value) {
            Branch first = branch;
            Branch second = branch;
            first.push_back({left, true});
            second.push_back({right, true});
            return {first, second};
        }
        if (kind == Formula::Kind::Biconditional) {
            Branch first = branch;
            Branch second = branch;
            first.push_back({left, item.value});
            first.push_back({right, item.value});
            second.push_back({left, !item.value});
            second.push_back({right, !item.value});
            return {first, second};
        }
    }
    return {branch};
}

void printTableau(const std::vector<FormulaPtr>& rules) {
    std::vector<std::string> atoms;
    for (const FormulaPtr& rule : rules) collectAtoms(rule, atoms);
    std::map<std::string, std::string> letters;
    for (std::size_t index = 0; index < atoms.size(); ++index) {
        letters[atoms[index]] = atomLetter(index);
    }

    std::cout << "\nTabla de átomos:\n";
    for (const std::string& atom : atoms) {
        std::cout << "  " << letters[atom] << " = " << atom << "\n";
    }
    std::cout << "\nReglas y fórmulas:\n";
    for (std::size_t index = 0; index < rules.size(); ++index) {
        std::cout << "  R" << index + 1 << ": "
                  << formatFormula(rules[index], letters) << "\n";
    }
    std::cout << "\nTableau semántico (se supone que todas las reglas son verdaderas):\n";

    Branch initial;
    for (const FormulaPtr& rule : rules) initial.push_back({rule, true, false});
    std::vector<Branch> branches = {initial};
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<Branch> next;
        for (const Branch& branch : branches) {
            if (closes(branch)) {
                next.push_back(branch);
                continue;
            }
            std::vector<Branch> expanded = expandBranch(branch, letters);
            if (expanded.size() != 1 || expanded.front().size() != branch.size()) {
                changed = true;
            }
            next.insert(next.end(), expanded.begin(), expanded.end());
        }
        branches = next;
    }

    bool hasOpen = false;
    for (std::size_t index = 0; index < branches.size(); ++index) {
        const Branch& branch = branches[index];
        const bool closed = closes(branch);
        hasOpen = hasOpen || !closed;
        std::cout << "\nRama " << index + 1 << (closed ? " ✕ CERRADA" : " ○ ABIERTA") << ":\n";
        for (const SignedFormula& item : branch) {
            printSigned(item, letters);
            std::cout << "\n";
        }
        if (!closed) {
            std::cout << "  Modelo: ";
            bool first = true;
            std::map<std::string, bool> model;
            for (const SignedFormula& item : branch) {
                if (isLiteral(item)) model[item.formula->name] = item.value;
            }
            for (const std::string& atom : atoms) {
                if (!first) std::cout << ", ";
                first = false;
                std::cout << letters[atom] << " = "
                          << (model.count(atom) != 0 && model[atom] ? "V" : "F");
            }
            std::cout << "\n";
        }
    }
    std::cout << "\nConclusión: la base de conocimiento es "
              << (hasOpen ? "SATISFACIBLE (tiene al menos un modelo)."
                          : "INSATISFACIBLE (todas las ramas se cierran).")
              << "\n";
}

void enableColors() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

bool hasLogicalConnector(const std::string& input) {
    const std::string value = lowerAscii(input);
    const std::vector<std::string> connectors = {
        " and ", " or ", " y ", " e ", " o ", " u ", " ni ",
        " entonces ", " por lo tanto ", "si y solo si", "si solo si",
        " dado que ", " ya que ", "si ", "->", "<->"
    };
    for (const std::string& connector : connectors) {
        if (value.find(connector) != std::string::npos) return true;
    }
    return value.find("(and)") != std::string::npos ||
           value.find("(or)") != std::string::npos ||
           value.find("(->)") != std::string::npos ||
           value.find("(<->)") != std::string::npos;
}

}  // namespace

int main(int argc, char* argv[]) {
    bool textMode = false;
    bool colors = true;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--texto") textMode = true;
        else if (argument == "--sin-color") colors = false;
        else if (argument == "--ayuda" || argument == "-h") {
            std::cout << "Uso: lector [--sin-color] [--texto]\n"
                      << "Por defecto lee una base de reglas, una regla por línea.\n"
                      << "--texto conserva el analizador léxico anterior.\n";
            return 0;
        }
    }

    enableColors();
    if (textMode) {
        std::cout << "Procesador de texto en español (Ctrl+Z para salir)\n";
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input.empty() || input[0] == '#') continue;
            std::vector<Token> tokens = tokenizeText(input);
            classify(tokens);
            printText(tokens, colors);
        }
        return 0;
    }

    std::cout << "Revisor de reglas proposicionales en español.\n"
              << "Escriba una regla o hecho por línea; Ctrl+Z para terminar.\n";
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input.empty() || input[0] == '#') continue;
        if (!hasLogicalConnector(input)) {
            std::cout << "No se detectó una relación lógica en: \""
                      << input << "\"\n"
                      << "Use y, o, ni, entonces, por lo tanto, dado que, "
                         "ya que, ->, <->, AND u OR.\n";
            continue;
        }
        FormulaParser parser(input);
        FormulaPtr rule = parser.parse();
        if (!rule) {
            std::cout << "Error en la regla \"" << input << "\": "
                      << parser.getError() << "\n";
            continue;
        }
        std::cout << "\nFrase recibida y verificada: " << input << "\n";
        printTableau({rule});
    }
    return 0;
}
