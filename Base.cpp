#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

/**.\lector.exe --arbol*/
struct Token {
    std::string text;
    std::string word;
    std::string category;
    bool wordToken;
};

namespace {
const char* RESET = "\033[0m";

const std::unordered_set<std::string> PREPOSITIONS = {
    "a", "ante", "bajo", "con", "contra", "de", "desde", "durante",
    "en", "entre", "hacia", "hasta", "mediante", "para", "por",
    "según", "sin", "sobre", "tras", "versus", "vía"
};
const std::unordered_set<std::string> CONNECTORS = {
    "y", "e", "ni", "o", "u", "pero", "sino", "aunque", "porque",
    "pues", "que", "si", "cuando", "mientras", "como", "donde",
    "cual", "cuales", "quien", "quienes", "además", "también"
};
const std::unordered_set<std::string> ARTICLES = {
    "el", "la", "los", "las", "un", "una", "unos", "unas", "lo", "al", "del"
};
const std::unordered_set<std::string> PRONOUNS = {
    "yo", "tú", "tu", "él", "ella", "ello", "nosotros", "nosotras",
    "vosotros", "vosotras", "ellos", "ellas", "me", "te", "se", "nos",
    "os", "lo", "la", "los", "las", "le", "les", "mí", "ti", "sí",
    "esto", "eso", "aquello", "quien", "quienes"
};
const std::unordered_set<std::string> ADVERBS = {
    "aquí", "ahí", "allí", "ahora", "ayer", "hoy", "mañana", "muy",
    "más", "menos", "bien", "mal", "no", "sí", "nunca", "siempre",
    "ya", "también", "solo", "sólo", "quizá", "quizás"
};
const std::unordered_set<std::string> VERBS = {
    "ser", "es", "soy", "eres", "somos", "son", "estar", "está",
    "estoy", "estás", "estamos", "están", "haber", "hay", "he",
    "has", "han", "hacer", "hace", "hacen", "tener", "tiene", "tienen",
    "poder", "puede", "pueden", "deber", "debe", "deben", "ir", "va",
    "van", "venir", "viene", "ver", "ve", "decir", "dice", "saber",
    "sabe", "leer", "lee", "escribir", "escribe", "analizar", "analiza",
    "amar", "ama", "comer", "come", "vivir", "vive", "estudiar",
    "estudia", "trabajar", "trabaja", "funcionar", "funciona", "corre",
    "corren", "habla", "hablan", "llega", "llegan", "quiere", "quieren",
    "conoce", "conocen", "usa", "usan", "tiene", "tienen"
};
const std::unordered_set<std::string> PAST_VERBS = {
    "fui", "fuiste", "fue", "fuimos", "fueron", "era", "eras", "éramos",
    "eran", "estuve", "estuviste", "estuvo", "estuvimos", "estuvieron",
    "estaba", "estabas", "estábamos", "estaban", "había", "hubo", "hice",
    "hiciste", "hizo", "hicimos", "hicieron", "tuve", "tuviste", "tuvo",
    "tuvimos", "tuvieron", "pude", "pudiste", "pudo", "pudimos", "pudieron",
    "dije", "dijiste", "dijo", "dijimos", "dijeron", "vi", "viste", "vio",
    "vimos", "vieron", "leí", "leíste", "leyó", "leímos", "leyeron",
    "escribí", "escribiste", "escribió", "escribimos", "escribieron",
    "amé", "amaste", "amó", "amamos", "amaron", "comí", "comiste", "comió",
    "comimos", "comieron", "viví", "viviste", "vivió", "vivimos", "vivieron",
    "trabajé", "trabajaste", "trabajó", "trabajamos", "trabajaron",
    "llegué", "llegaste", "llegó", "llegamos", "llegaron", "hablé",
    "hablaste", "habló", "hablamos", "hablaron", "fui", "fueron"
};

std::string lowerAscii(std::string value) {
    for (char& character : value) {
        if (static_cast<unsigned char>(character) < 128) {
            character = static_cast<char>(
                std::tolower(static_cast<unsigned char>(character)));
        }
    }
    return value;
}

void replaceInsensitive(std::string& value, const std::string& search,
                        const std::string& replacement) {
    std::string lowered = lowerAscii(value);
    const std::string loweredSearch = lowerAscii(search);
    std::size_t position = lowered.find(loweredSearch);
    while (position != std::string::npos) {
        value.replace(position, search.size(), replacement);
        lowered.replace(position, search.size(), replacement);
        position = lowered.find(loweredSearch, position + replacement.size());
    }
}

void replaceWordInsensitive(std::string& value, const std::string& search,
                            const std::string& replacement) {
    std::string lowered = lowerAscii(value);
    const std::string loweredSearch = lowerAscii(search);
    std::size_t position = lowered.find(loweredSearch);
    while (position != std::string::npos) {
        const bool startsWord = position > 0 &&
            std::isalnum(static_cast<unsigned char>(lowered[position - 1])) != 0;
        const std::size_t end = position + search.size();
        const bool endsWord = end < lowered.size() &&
            std::isalnum(static_cast<unsigned char>(lowered[end])) != 0;
        if (!startsWord && !endsWord) {
            value.replace(position, search.size(), replacement);
            lowered.replace(position, search.size(), replacement);
            position = lowered.find(loweredSearch, position + replacement.size());
        } else {
            position = lowered.find(loweredSearch, end);
        }
    }
}

bool isPunctuation(char character) {
    return std::ispunct(static_cast<unsigned char>(character)) != 0 &&
           character != '-' && character != '\'';
}

std::vector<Token> tokenize(const std::string& input) {
    std::vector<Token> tokens;
    std::string current;
    auto flushWord = [&]() {
        if (!current.empty()) {
            tokens.push_back({current, lowerAscii(current), "", true});
            current.clear();
        }
    };

    for (char character : input) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            flushWord();
        } else if (character == ',' || character == '.') {
            flushWord();
        } else if (isPunctuation(character)) {
            flushWord();
            tokens.push_back({std::string(1, character), "", "Puntuación", false});
        } else {
            current += character;
        }
    }
    flushWord();
    return tokens;
}

bool looksLikeVerb(const std::string& word) {
    if (VERBS.count(word) != 0 || PAST_VERBS.count(word) != 0) {
        return true;
    }
    const std::vector<std::string> endings = {
        "ar", "er", "ir", "ando", "iendo", "ado", "ido", "aba", "ía", "arán",
        "erán", "irán", "aría", "ería", "iría", "é", "aste", "iste",
        "ó", "imos", "ieron", "aba", "abas", "ábamos", "aban"
    };
    for (const std::string& ending : endings) {
        if (word.size() > ending.size() &&
            word.compare(word.size() - ending.size(), ending.size(), ending) == 0) {
            return true;
        }
    }
    return false;
}

std::string colorFor(const std::string& category) {
    if (category == "Sujeto") return "\033[1;36m";
    if (category == "Preposición") return "\033[1;35m";
    if (category == "Nexo") return "\033[1;33m";
    if (category == "Verbo") return "\033[1;32m";
    if (category == "Artículo") return "\033[1;34m";
    if (category == "Pronombre") return "\033[1;31m";
    if (category == "Adverbio") return "\033[1;35m";
    return "\033[1;37m";
}

void classify(std::vector<Token>& tokens) {
    bool verbSeen = false;
    bool subjectStarted = false;
    for (Token& token : tokens) {
        if (!token.wordToken) continue;

        const std::string& word = token.word;
        if (PREPOSITIONS.count(word) != 0) {
            token.category = "Preposición";
        } else if (CONNECTORS.count(word) != 0) {
            token.category = "Nexo";
        } else if (ARTICLES.count(word) != 0) {
            token.category = "Artículo";
        } else if (PRONOUNS.count(word) != 0) {
            token.category = "Pronombre";
        } else if (ADVERBS.count(word) != 0) {
            token.category = "Adverbio";
        } else if (looksLikeVerb(word)) {
            token.category = PAST_VERBS.count(word) != 0 ? "Verbo (pasado)" : "Verbo";
            verbSeen = true;
        } else {
            token.category = "Palabra";
        }

        // Regla práctica: el sintagma anterior al primer verbo se toma como sujeto.
        if (!verbSeen && token.category != "Preposición" &&
            token.category != "Nexo" && token.category != "Adverbio") {
            token.category = "Sujeto";
            subjectStarted = true;
        } else if (subjectStarted && !verbSeen &&
                   (token.category == "Preposición" || token.category == "Nexo")) {
            subjectStarted = false;
        }
    }
}

        struct Formula {
            enum class Kind { Proposition, And, Or, Implication, Biconditional, Neither };
            Kind kind;
            std::string name;
            std::shared_ptr<Formula> left;
            std::shared_ptr<Formula> right;
        };

        struct FormulaParser {
            std::vector<std::string> tokens;
            std::size_t position = 0;
            std::string error;

            explicit FormulaParser(const std::string& input) {
                std::string normalized = input;
                replaceInsensitive(normalized, "(AND)", " AND ");
                replaceInsensitive(normalized, "(OR)", " OR ");
                replaceInsensitive(normalized, "(->)", " -> ");
                replaceInsensitive(normalized, "(<->)", " <-> ");
                replaceInsensitive(normalized, ",", " ");
                replaceInsensitive(normalized, ".", " ");
                replaceInsensitive(normalized, "por lo tanto", " -> ");
                replaceInsensitive(normalized, "entonces", " -> ");
                replaceInsensitive(normalized, "dado que", " <-> ");
                replaceInsensitive(normalized, "ya que", " <-> ");
                replaceWordInsensitive(normalized, "ni", " NI ");
                replaceWordInsensitive(normalized, "and", " AND ");
                replaceWordInsensitive(normalized, "or", " OR ");
                replaceWordInsensitive(normalized, "y", " AND ");
                replaceWordInsensitive(normalized, "o", " OR ");
                const std::vector<std::string> parenthesizedOperators = {
                    "(AND)", "(OR)", "(->)", "(<->)"
                };
                for (const std::string& operatorText : parenthesizedOperators) {
                    std::size_t found = normalized.find(operatorText);
                    while (found != std::string::npos) {
                        normalized.replace(found, operatorText.size(), " " + operatorText.substr(1, operatorText.size() - 2) + " ");
                        found = normalized.find(operatorText, found + 1);
                    }
                }
                for (std::size_t index = 0; index < normalized.size();) {
                    if (std::isspace(static_cast<unsigned char>(normalized[index]))) {
                        ++index;
                    } else if (normalized.compare(index, 3, "<->") == 0) {
                        tokens.push_back("<->");
                        index += 3;
                    } else if (normalized.compare(index, 2, "->") == 0) {
                        tokens.push_back("->");
                        index += 2;
                    } else if (normalized[index] == '(' || normalized[index] == ')') {
                        tokens.emplace_back(1, normalized[index++]);
                    } else {
                        std::size_t start = index;
                        while (index < normalized.size() &&
                               std::isalnum(static_cast<unsigned char>(normalized[index]))) {
                            ++index;
                        }
                        if (start == index) {
                            error = "carácter no reconocido";
                            return;
                        }
                        tokens.push_back(normalized.substr(start, index - start));
                    }
                }
                for (std::string& token : tokens) {
                    if (token == "and" || token == "AND") token = "AND";
                    if (token == "or" || token == "OR") token = "OR";
                    if (token == "ni" || token == "NI") token = "NI";
                }
            }

            std::shared_ptr<Formula> parse() {
                auto result = parseBiconditional();
                if (!result && error.empty()) error = "fórmula incompleta";
                if (error.empty() && position != tokens.size()) error = "símbolo inesperado: " + tokens[position];
                return result;
            }

            std::shared_ptr<Formula> parseBiconditional() {
                auto left = parseImplication();
                if (left && take("<->")) {
                    auto right = parseBiconditional();
                    if (!right) return fail("falta la conclusión del bicondicional");
                    return make(Formula::Kind::Biconditional, left, right);
                }
                if (left && take("NI")) {
                    auto right = parseBiconditional();
                    if (!right) return fail("falta la segunda proposición de NI");
                    return make(Formula::Kind::Neither, left, right);
                }
                return left;
            }

            std::shared_ptr<Formula> parseImplication() {
                auto left = parseOr();
                if (left && take("->")) {
                    auto right = parseImplication();
                    if (!right) return fail("falta la conclusión de la implicación");
                    return make(Formula::Kind::Implication, left, right);
                }
                return left;
            }

            std::shared_ptr<Formula> parseOr() {
                auto result = parseAnd();
                while (result && take("OR")) {
                    auto right = parseAnd();
                    if (!right) return fail("falta un operando para OR");
                    result = make(Formula::Kind::Or, result, right);
                }
                return result;
            }

            std::shared_ptr<Formula> parseAnd() {
                auto result = parsePrimary();
                while (result && take("AND")) {
                    auto right = parsePrimary();
                    if (!right) return fail("falta un operando para AND");
                    result = make(Formula::Kind::And, result, right);
                }
                return result;
            }

            std::shared_ptr<Formula> parsePrimary() {
                if (take("(")) {
                    auto result = parseBiconditional();
                    if (!take(")")) return fail("falta ')'");
                    return result;
                }
                if (position < tokens.size() && tokens[position] != "AND" &&
                    tokens[position] != "OR" && tokens[position] != "->" &&
                    tokens[position] != "<->" && tokens[position] != "NI" &&
                    tokens[position] != ")") {
                    auto result = std::make_shared<Formula>();
                    result->kind = Formula::Kind::Proposition;
                    result->name = tokens[position++];
                    while (position < tokens.size() && tokens[position] != "AND" &&
                           tokens[position] != "OR" && tokens[position] != "->" &&
                           tokens[position] != "<->" && tokens[position] != "NI" &&
                           tokens[position] != ")") {
                        result->name += "_" + tokens[position++];
                    }
                    return result;
                }
                return nullptr;
            }

            bool take(const std::string& token) {
                if (position < tokens.size() && tokens[position] == token) {
                    ++position;
                    return true;
                }
                return false;
            }

            std::shared_ptr<Formula> make(Formula::Kind kind,
                                          const std::shared_ptr<Formula>& left,
                                          const std::shared_ptr<Formula>& right) {
                auto result = std::make_shared<Formula>();
                result->kind = kind;
                result->left = left;
                result->right = right;
                return result;
            }

            std::shared_ptr<Formula> fail(const std::string& message) {
                if (error.empty()) error = message;
                return nullptr;
            }
        };

        bool evaluate(const std::shared_ptr<Formula>& formula,
                      const std::unordered_set<std::string>& trueValues) {
            if (formula->kind == Formula::Kind::Proposition) {
                return trueValues.count(formula->name) != 0;
            }
            bool left = evaluate(formula->left, trueValues);
            bool right = evaluate(formula->right, trueValues);
            if (formula->kind == Formula::Kind::And) return left && right;
            if (formula->kind == Formula::Kind::Or) return left || right;
            if (formula->kind == Formula::Kind::Implication) return !left || right;
            if (formula->kind == Formula::Kind::Neither) return !left && !right;
            return left == right;
        }

        void collectPropositions(const std::shared_ptr<Formula>& formula,
                                 std::vector<std::string>& propositions) {
            if (formula->kind == Formula::Kind::Proposition) {
                if (std::find(propositions.begin(), propositions.end(), formula->name) ==
                    propositions.end()) {
                    propositions.push_back(formula->name);
                }
                return;
            }
            collectPropositions(formula->left, propositions);
            collectPropositions(formula->right, propositions);
        }

        std::string kindName(Formula::Kind kind) {
            if (kind == Formula::Kind::And) return "AND: ambos eventos ocurren";
            if (kind == Formula::Kind::Or) return "OR: al menos un evento ocurre";
            if (kind == Formula::Kind::Implication) return "->: falso antecedente o verdadera conclusión";
            if (kind == Formula::Kind::Neither) return "ni: ambos sucesos son falsos";
            return "<->: ambos sucesos tienen el mismo valor";
        }

        void evaluateFormula(const std::string& input) {
            FormulaParser parser(input);
            auto formula = parser.parse();
            if (!formula) {
                std::cout << "Error semántico: " << parser.error << "\n";
                return;
            }
            std::vector<std::string> propositions;
            collectPropositions(formula, propositions);
            if (propositions.size() > 10) {
                std::cout << "Error semántico: se permiten como máximo 10 proposiciones.\n";
                return;
            }
            std::cout << "\nEvaluación semántica: " << input << "\n";
            std::cout << "Regla: " << kindName(formula->kind) << "\n";
            for (const std::string& proposition : propositions) std::cout << proposition << "\t";
            std::cout << "Resultado\n";
            const unsigned long combinations = 1UL << propositions.size();
            bool tautology = true;
            bool contradiction = true;
            for (unsigned long row = 0; row < combinations; ++row) {
                std::unordered_set<std::string> trueValues;
                for (std::size_t index = 0; index < propositions.size(); ++index) {
                    bool value = (row & (1UL << (propositions.size() - index - 1))) != 0;
                    if (value) trueValues.insert(propositions[index]);
                    std::cout << (value ? "V" : "F") << "\t";
                }
                bool result = evaluate(formula, trueValues);
                std::cout << (result ? "V" : "F") << "\n";
                tautology = tautology && result;
                contradiction = contradiction && !result;
            }
            //std::cout << "Conclusión: " << (tautology ? "válida (tautología)" :
             //   contradiction ? "inválida y contradictoria" : "contingente") << "\n";
        }

void printMarked(const std::vector<Token>& tokens, bool colors) {
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        const Token& token = tokens[index];
        if (!token.wordToken || !colors) {
            std::cout << token.text;
        } else {
            std::cout << colorFor(token.category) << token.text << RESET;
        }
        if (index + 1 < tokens.size() &&
            (token.wordToken || !tokens[index + 1].wordToken) &&
            tokens[index + 1].wordToken) {
            std::cout << ' ';
        }
    }
    std::cout << "\n";
}

void printTree(const std::vector<Token>& tokens) {
    std::cout << "\nÁrbol semántico simplificado\n";
    std::cout << "Oración\n";
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (!tokens[index].wordToken) continue;
        std::cout << "├── " << tokens[index].category << ": " << tokens[index].text << "\n";
    }
}

void enableColors() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (GetConsoleMode(output, &mode) != 0) {
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

bool isFormulaExpression(const std::string& input) {
    const std::string normalized = lowerAscii(input);
    return normalized.find("and") != std::string::npos ||
           normalized.find("or") != std::string::npos ||
           normalized.find(" y ") != std::string::npos ||
           normalized.find(" o ") != std::string::npos ||
           normalized.find("entonces") != std::string::npos ||
           normalized.find("por lo tanto") != std::string::npos ||
           normalized.find("dado que") != std::string::npos ||
           normalized.find("ya que") != std::string::npos ||
           normalized.find(" ni ") != std::string::npos ||
           input.find("->") != std::string::npos ||
           input.find("<->") != std::string::npos;
}
}  // namespace

int main(int argc, char* argv[]) {
    bool colors = true;
    bool tree = false;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--sin-color") colors = false;
        else if (argument == "--arbol") tree = true;
        else if (argument == "--ayuda" || argument == "-h") {
            std::cout << "Uso: lector [--sin-color] [--arbol]\n";
            return 0;
        }
    }

    enableColors();
    std::cout << "Procesador de texto en español (escriba una oración; Ctrl+Z para salir)\n";
    std::cout << "Leyenda: sujeto | preposición | nexo | verbo | artículo | pronombre | adverbio\n\n";

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input.empty()) continue;
        if (input[0] == '#') continue;
        if (isFormulaExpression(input)) {
            evaluateFormula(input);
            continue;
        }
        std::vector<Token> tokens = tokenize(input);
        classify(tokens);
        printMarked(tokens, colors);
        if (tree) printTree(tokens);
    }
    return 0;
}