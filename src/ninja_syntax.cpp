// Ported from ninja_syntax.py from the Ninja repository, Copyright 2011 Google Inc. (Apache-2.0)
// This file is also licensed under the Apache-2.0 license
// Copyright 2025 Ty Qualters

#include "ninja_syntax.hpp"

/**
* Notes:
*     1) To break down a std::variant, you must visit it with std::visit.
*     2) std::format is generally broken.
*/
void NinjaWriter::newline() {
    m_buf << '\n';
}

void NinjaWriter::comment(const std::string& text) {
    for (const auto lines = wrapText(text, m_width - 2, false); const auto& line : lines) {
        m_buf << "# " << line << '\n';
    }
}

void NinjaWriter::variable(std::string key, std::optional<std::variant<bool, int, float, std::string, std::vector<std::string>>> value, const int indent) {

    if(!value.has_value()) {
      return;
    }

    std::visit([&]<typename T0>(T0&& arg) {
        using T = std::decay_t<T0>;

        if constexpr(std::is_same_v<T, bool>) {
          // Handle bool
          _line(std::format("{} = {}", key, (arg ? "true" : "false")), indent);
        } else if constexpr(std::is_same_v<T, int>) {
          // Handle int
          std::string _value = std::to_string(arg);
          _line(std::format("{} = {}", key, _value), indent);
        } else if constexpr(std::is_same_v<T, float>) {
          // Handle float
          std::string _value = std::to_string(arg);
          _line(std::format("{} = {}", key, _value), indent);
        } else if constexpr(std::is_same_v<T, std::string>) {
          // Handle string
          std::string& _value = arg;
          _line(std::format("{} = {}", key, _value), indent);
        } else if constexpr(std::is_same_v<T, std::vector<std::string>>) {
          // Handle vector<string>
          removeEmptyStrings(arg);
          std::string _value = joinStrings(arg, " ");
          _line(std::format("{} = {}", key, (std::string)_value), indent);
        } else {
          static_assert(false, "non-exhaustive visitor");
        }
    }, value.value());

}
void NinjaWriter::pool(std::string name, int depth) {
    _line(std::format("pool {}", name));
    variable("depth", depth, 1);
}

void NinjaWriter::rule(std::string name, std::string command, std::optional<std::string> description, std::optional<std::string> depfile, bool generator, std::optional<std::string> pool, bool restat, std::optional<std::string> rspfile, std::optional<std::string> rspfile_content, std::optional<std::variant<std::string, std::vector<std::string>>> deps) {

  _line(std::format("rule {}", name));
  variable("command", command, 1);
  if(description.has_value()) {
    variable("description", description.value(), 1);
  }
  if(depfile.has_value()) {
    variable("depfile", depfile.value(), 1);
  }
  if(generator) {
    variable("generator", "1", 1);
  }
  if(pool.has_value()) {
    variable("pool", pool.value(), 1);
  }
  if(restat) {
    variable("restat", "1", 1);
  }
  if(rspfile.has_value()) {
    variable("rspfile", rspfile.value(), 1);
  }
  if(rspfile_content.has_value()) {
    variable("rspfile_content", rspfile_content.value(), 1);
  }
  if(deps.has_value()) {
      std::visit([&]<typename T0>(T0&& arg) {
          using T = std::decay_t<T0>;
          
          if constexpr(std::is_same_v<T, std::string>) {
              // Handle string
              variable("deps", arg, 1);
          } else if constexpr(std::is_same_v<T, std::vector<std::string>>) {
              // Handle vector<string>
              variable("deps", arg, 1);
          } else {
              static_assert(false, "non-exhaustive visitor");
          }
      }, deps.value());
  }
}

std::vector<std::string> NinjaWriter::build(std::variant<std::string, std::vector<std::string>> outputs, std::string rule, std::optional<std::variant<std::string, std::vector<std::string>>> inputs, std::optional<std::variant<std::string, std::vector<std::string>>> implicit, std::optional<std::variant<std::string, std::vector<std::string>>> order_only, std::optional<std::variant<std::pair<std::string, std::optional<std::variant<std::string, std::vector<std::string>>>>, std::unordered_map<std::string, std::optional<std::variant<std::string, std::vector<std::string>>>>>> variables, std::optional<std::variant<std::string, std::vector<std::string>>> implicit_outputs, std::optional<std::string> pool, std::optional<std::string> dynDep) {
    std::vector<std::string> _outputs = as_list(outputs);
    // use _outputs from here on

    std::vector<std::string> out_outputs = _outputs;
    std::for_each(out_outputs.begin(), out_outputs.end(), [&](auto& out) {
        out = escape_path(out);
    });

    std::vector<std::string> all_inputs = as_list(inputs);
    std::for_each(all_inputs.begin(), all_inputs.end(), [&](auto& in) {
        in = escape_path(in);
    });

    if (implicit.has_value()) {
        std::vector<std::string> _implicit = as_list(implicit.value());
        all_inputs.push_back("|");
        std::for_each(_implicit.begin(), _implicit.end(), [&](auto& imp) {
            imp = escape_path(imp);
            all_inputs.push_back(imp);
        });
    }
    if (order_only.has_value()) {
        std::vector<std::string> _order_only = as_list(order_only.value());
        all_inputs.push_back("||");
        std::for_each(_order_only.begin(), _order_only.end(), [&](auto& oo) {
            oo = escape_path(oo);
            all_inputs.push_back(oo);
        });
    }
    if (implicit_outputs.has_value()) {
        std::vector<std::string> _implicit_outputs = as_list(implicit_outputs.value());
        all_inputs.push_back("|");
        std::for_each(_implicit_outputs.begin(), _implicit_outputs.end(), [&](auto& impo) {
            impo = escape_path(impo);
            all_inputs.push_back(impo);
        });
    }

    std::vector<std::string> _1 = all_inputs;
    _1.emplace(_1.begin(), rule);
    _line(std::format("build {}: {}", joinStrings(out_outputs, " "), joinStrings(_1, " ")));

    if (pool.has_value()) {
        _line(std::format("  pool = {}", pool.value()));
    }

    if (dynDep.has_value()) {
        _line(std::format("  dyndep = {}", dynDep.value()));
    }

    if (variables.has_value()) {
        std::visit([&]<typename T0>(T0&& arg) {
            using T = std::decay_t<T0>;
            if constexpr(std::is_same_v<T, std::unordered_map<std::string, std::optional<std::variant<std::string, std::vector<std::string>>>>>) {
                // Dict[str, Optional[Union[str, List[str]]]]
                for (const auto& [key, value] : arg) {
                    if (value.has_value()) {

                        // variants need to be explicitly handled
                        std::visit([&]<typename T1>(T1&& arg) {
                          using U = std::decay_t<T1>;

                          if constexpr(std::is_same_v<U, std::string>) {
                              // Handle string
                              variable(key, arg, 1);
                          } else if constexpr(std::is_same_v<U, std::vector<std::string>>) {
                              // Handle vector<string>
                              variable(key, arg, 1);
                          } else {
                              static_assert(false, "non-exhaustive visitor");
                          }
                      }, value.value());
                    } else variable(key, std::nullopt, 1);
                }
            } else if constexpr(std::is_same_v<T, std::pair<std::string, std::optional<std::variant<std::string, std::vector<std::string>>>>>) {
                // List[Tuple[str, Optional[Union[str, List[str]]]]]
                const auto& [key, value] = arg;

                // variants need to be explicitly handled
                std::visit([&]<typename T1>(T1&& arg) {
                  using U = std::decay_t<T1>;

                  if constexpr(std::is_same_v<U, std::string>) {
                      // Handle string
                      variable(key, arg, 1);
                  } else if constexpr(std::is_same_v<U, std::vector<std::string>>) {
                      // Handle vector<string>
                      variable(key, arg, 1);
                  } else {
                      static_assert(false, "non-exhaustive visitor");
                  }
              }, value.value());
            }
        }, variables.value());
    }

    return _outputs;
}

void NinjaWriter::include(std::string path) {
    _line(std::format("include {}", path));
}


void NinjaWriter::subninja(std::string path) {
    _line(std::format("subninja {}", path));
}

void NinjaWriter::_default(std::variant<std::string, std::vector<std::string>> paths) {
    _line(std::format("default {}", joinStrings(as_list(paths), " ")));
}

int NinjaWriter::_count_dollars_before_index(const std::string &s, const int i) {
    int dollar_count = 0;
    int dollar_index = i - 1;
    while (dollar_index > 0 && s[dollar_index] == '$') {
        ++dollar_count;
        --dollar_index;
    }
    return dollar_count;
}

void NinjaWriter::_line(std::string text, const int indent) {
    auto leadingSpace = std::string(indent, ' ');
    while (leadingSpace.size() + text.size() > m_width) {
        const auto available_space = m_width - leadingSpace.size() - std::string(" $").size();
        auto space = available_space;
        while (true) {
            space = text.rfind(" ", 0, space);
            if (space < 0 || _count_dollars_before_index(text, space) % 2 == 0) {
                break;
            }
        }

        if (space < 0) {
            space = available_space - 1;
            while (true) {
                space = text.find(" ", space + 1);
                if (space < 0 || _count_dollars_before_index(text, space) % 2 == 0) {
                    break;
                }
            }
        }

        if (space < 0) {
            break;
        }

        m_buf << leadingSpace << text.substr(0, space) << " $\n";
        text = text.substr(space + 1);
        leadingSpace = std::string(indent + 2, ' ');
    }
    m_buf << leadingSpace << text << "\n";
}

std::vector<std::string> NinjaWriter::as_list(std::optional<std::variant<std::string, std::vector<std::string>>> input) {
    std::vector<std::string> result;
    if (!input.has_value()) {
        return result;
    }
    std::visit([&]<typename T0>(T0&& arg) {
        using T = std::decay_t<T0>;
        if constexpr(std::is_same_v<T, std::string>) {
            result.push_back(arg);
        } else if constexpr(std::is_same_v<T, std::vector<std::string>>) {
            result.swap(arg);
        } else {
            static_assert(false, "non-exhaustive visitor");
        }
    }, input.value());
    return result;
}

std::string NinjaWriter::escape(std::string str) {
    for (const char c : str) {
        if (c == '\n') throw std::runtime_error("Ninja syntax does not allow newlines");
    }

    str.replace(str.begin(), str.end(), "$", "$$");
    return str;
}

// This function was AI-generated
std::string NinjaWriter::expand(std::string str, std::unordered_map<std::string, std::string> vars, std::unordered_map<std::string, std::string> local_vars) {
    std::string& input = str; // I didn't feel like renaming
    std::regex var_regex(R"(\$(\$|\w*))"); // Matches "$var" or "$$"
    std::string result;
    size_t last_pos = 0;

    // Iterate over matches using std::sregex_iterator
    auto end = std::sregex_iterator();
    for (std::sregex_iterator it(input.begin(), input.end(), var_regex); it != end; ++it) {
        const std::smatch& match = *it;
        std::string var = match[1]; // Capture group 1: variable name or literal "$"

        // Add text before the match
        result += input.substr(last_pos, match.position() - last_pos);

        // Replace based on matched group
        if (var == "$") {
            result += "$"; // Literal "$"
        } else {
            auto local_it = local_vars.find(var);
            if (local_it != local_vars.end()) {
                result += local_it->second; // Use local variable if it exists
            } else {
                auto global_it = vars.find(var);
                if (global_it != vars.end()) {
                    result += global_it->second; // Use global variable if it exists
                } else {
                    result += ""; // Default to empty string
                }
            }
        }

        // Update the last position to the end of the match
        last_pos = match.position() + match.length();
    }

    // Append any remaining part of the string after the last match
    result += input.substr(last_pos);

    return result;
}



std::string NinjaWriter::string() const {
    return m_buf.str();
}

void NinjaWriter::reset() {
    m_buf.clear();
    m_buf.str("");
}

std::string NinjaWriter::escape_path(std::string path) {
    replaceAll(path, "$ ", "$$ ");
    replaceAll(path, " ", "$ ");
    replaceAll(path, ":", "$:");
    return path;
}

void NinjaWriter::removeEmptyStrings(std::vector<std::string>& strings) {
    std::vector<std::string> nonEmptyStrings(strings.size());
    std::copy_if(strings.begin(), strings.end(), nonEmptyStrings.begin(), [](const auto& str) { return !str.empty() && str != " "; });
    strings = nonEmptyStrings;
}

// Michael Mrozek @ https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
void NinjaWriter::replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

// This function was AI-generated
std::vector<std::string> NinjaWriter::wrapText(const std::string& text, std::size_t width, bool breakLongWords) {
    std::vector<std::string> result;
    std::istringstream stream(text);
    std::string word;
    std::string currentLine;

    while (stream >> word) {
        // If the current word won't fit in the current line
        if (currentLine.size() + word.size() + 1 > width) {
            if (!currentLine.empty()) {
                result.push_back(currentLine);
                currentLine.clear();
            }
        }

        // Handle long words if `breakLongWords` is enabled
        if (word.size() > width) {
            if (breakLongWords) {
                for (std::size_t start = 0; start < word.size(); start += width) {
                    result.push_back(word.substr(start, width));
                }
                continue; // Skip adding this word to the current line
            }
        }

        // Add the current word to the line
        if (!currentLine.empty()) {
            currentLine += " ";
        }
        currentLine += word;
    }

    // Add any remaining content to the result
    if (!currentLine.empty()) {
        result.push_back(currentLine);
    }

    return result;
}

// This function was AI-generated
std::string NinjaWriter::joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return ""; // If the vector is empty, return an empty string

    std::ostringstream joined;
    for (std::size_t i = 0; i < strings.size(); ++i) {
        joined << strings[i];
        if (i != strings.size() - 1) { // Add the delimiter for all but the last element
            joined << delimiter;
        }
    }
    return joined.str();
}