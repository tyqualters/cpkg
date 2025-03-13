#ifndef NINJA_SYNTAX_HPP
#define NINJA_SYNTAX_HPP

#include <variant>
#include <string>
#include <vector>
#include <sstream>
#include <optional>
#include <unordered_map>
#include <format>
#include <regex>
#include <ranges>

class NinjaWriter {
public:

    NinjaWriter() = default;
    NinjaWriter(const int width) : m_width(width) {}

    void newline();

    void comment(const std::string& text);

    void variable(
        std::string key,
        std::optional<std::variant<bool, int, float, std::string, std::vector<std::string>>> value,
        int indent = 0
        );

    void pool(std::string name, int depth);

    void rule(
        std::string name,
        std::string command,
        std::optional<std::string> description = std::nullopt,
        std::optional<std::string> depfile = std::nullopt,
        bool generator = false,
        std::optional<std::string> pool = std::nullopt,
        bool restat = false,
        std::optional<std::string> rspfile = std::nullopt,
        std::optional<std::string> rspfile_content = std::nullopt,
        std::optional<std::variant<std::string, std::vector<std::string>>> deps = std::nullopt
        );

    std::vector<std::string> build(
        std::variant<std::string, std::vector<std::string>> outputs,
        std::string rule,
        std::optional<std::variant<std::string, std::vector<std::string>>> inputs = std::nullopt,
        std::optional<std::variant<std::string, std::vector<std::string>>> implicit = std::nullopt,
        std::optional<std::variant<std::string, std::vector<std::string>>> order_only = std::nullopt,
        std::optional<std::variant<std::pair<std::string, std::optional<std::variant<std::string, std::vector<std::string>>>>, std::unordered_map<std::string, std::optional<std::variant<std::string, std::vector<std::string>>>>>> variables = std::nullopt,
        std::optional<std::variant<std::string, std::vector<std::string>>> implicit_outputs = std::nullopt,
        std::optional<std::string> pool = std::nullopt,
        std::optional<std::string> dynDep = std::nullopt
        ); // TODO

    void include(std::string path); // TODO

    void subninja(std::string path); // TODO

    void _default(std::variant<std::string, std::vector<std::string>> paths); // TODO

    static int _count_dollars_before_index(const std::string &s, int i);

    void _line(std::string text, int indent = 0);

    std::vector<std::string> as_list(
        std::optional<std::variant<std::string, std::vector<std::string>>> input
        );

    static std::string escape(std::string str);

    // This function was AI-generated
    std::string expand(
        std::string str,
        std::unordered_map<std::string, std::string> vars,
        std::unordered_map<std::string, std::string> local_vars
        );

    std::string string() const;

    void reset();

private:
    std::stringstream m_buf;
    int m_width = 78;

    static std::string escape_path(std::string path);

    static void removeEmptyStrings(std::vector<std::string>& strings);

    // Michael Mrozek @ https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
    static void replaceAll(std::string& str, const std::string& from, const std::string& to);

    // This function was AI-generated
    static std::vector<std::string> wrapText(const std::string& text, std::size_t width, bool breakLongWords = false);

    // This function was AI-generated
    static std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter = " ");
};

#endif //NINJA_SYNTAX_HPP
