#include <fmt/core.h>

#include <cassert>
#include <cctype>
#include <fstream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace sjson {

enum class JSONToken {
  NULL_,
  BOOL,
  INTEGER,
  NUMBER,
  STRING,
  LEFT_SQUARE_BRACKET,   // [
  RIGHT_SQUARE_BRACKET,  // ]
  LEFT_CURLY_BRACKET,    // {
  RIGHT_CURLY_BRACKET,   // }
  COMMA,                 // ,
  COLON,                 // :
};

class JSONObject {
 public:
  enum class JSONType {
    NULL_,
    BOOL,
    INTEGER,  // int
    NUMBER,   // float
    STRING,   // string
    ARRAY,    // array
    DICT      // dictionary
  };
  using Array = std::vector<JSONObject>;
  using Dict = std::map<std::string, JSONObject>;

  JSONObject() : type(JSONType::NULL_), value("null") {}
  explicit JSONObject(bool v) : type(JSONType::BOOL), value(v) {}
  explicit JSONObject(int v) : type(JSONType::INTEGER), value(v) {}
  explicit JSONObject(float v) : type(JSONType::NUMBER), value(v) {}
  explicit JSONObject(const std::string& v) : type(JSONType::STRING), value(v) {}
  explicit JSONObject(const Array& v) : type(JSONType::ARRAY), value(v) {}
  explicit JSONObject(const Dict& v) : type(JSONType::DICT), value(v) {}

  JSONType type;
  std::variant<bool, int, float, std::string, Array, Dict> value;

  std::string to_string(int depth = 0) {
    std::string out;

    auto add = [&](const std::string& s) { 
      out += std::string(depth, ' ') + s;
    };
    
    switch (type) {
      case JSONType::NULL_:
        return "null";
        break;
      case JSONType::BOOL:
        try {
          return std::get<bool>(value) ? "true" : "false";
        } catch (std::exception& e) {
          fmt::println("Error: {} BOOL", e.what());
        }
        break;
      case JSONType::INTEGER:
        try {
          return std::to_string(std::get<int>(value));
        } catch (std::exception& e) {
          fmt::println("Error: {} INTEGER", e.what());
        }
        break;
      case JSONType::NUMBER:
        try {
          return std::to_string(std::get<float>(value));
        } catch (std::exception& e) {
          fmt::println("Error: {} NUMBER", e.what());
        }
        break;
      case JSONType::STRING:
        try {
          return "\"" + std::get<std::string>(value) + "\"";
        } catch (std::exception& e) {
          fmt::println("Error: {} STRING", e.what());
        }
        break;
      case JSONType::ARRAY:
        try {
          out += "[\n";
          auto& array = std::get<Array>(value);
          for (int i = 0; i < array.size(); i++) {
            add(array[i].to_string(depth + 2));
            if (i < array.size() - 1) {
              out += ",";
            }
            out += "\n";
          }
          if (depth > 2) {
            out += std::string(depth - 2, ' ');
          }
          out += "]";
        } catch (std::exception& e) {
          fmt::println("Error: {} ARRAY", e.what());
        }
        break;
      case JSONType::DICT:
        try {
          out += "{\n";
          auto& dict = std::get<Dict>(value);
          for (auto it = dict.begin(); it != dict.end(); it++) {
            add("\"" + it->first + "\": " + it->second.to_string(depth + 2));
            if (std::next(it) != dict.end()) {
              out += ",";
            }
            out += "\n";
          }
          if (depth > 2) {
            out += std::string(depth - 2, ' ');
          }
          out += "}";
        } catch (std::exception& e) {
          fmt::println("Error: {} DICT", e.what());
        }
        break;
    }
    return out;
  }
};

class JSONScanner {
 public:
  JSONScanner() {}

  void read(const std::string& filename) {
    file.open(filename);
    if (!file.is_open()) {
      throw std::runtime_error("Invalid file");
    }

    try {
      scan();
    } catch (std::exception& e) {
      fmt::println("Error: {}", e.what());
    }
  }

  void scan() {
    std::string line;
    for (int line_index = 1; std::getline(file, line); line_index++) {
      // fmt::print("line [{}]: {}\n", line_index, line);

      // find comments
      auto comment = line.find("//");
      if (comment != std::string::npos) {
        line = line.substr(0, comment);
      }

      for (int i = 0; i < line.length(); i++) {
        if (std::isspace(line[i])) {
          continue;
        }
        if (line[i] == '{') {
          tokens.push_back({JSONToken::LEFT_CURLY_BRACKET, "{"});
        } else if (line[i] == '}') {
          tokens.push_back({JSONToken::RIGHT_CURLY_BRACKET, "}"});
        } else if (line[i] == '[') {
          tokens.push_back({JSONToken::LEFT_SQUARE_BRACKET, "["});
        } else if (line[i] == ']') {
          tokens.push_back({JSONToken::RIGHT_SQUARE_BRACKET, "]"});
        } else if (line[i] == ',') {
          tokens.push_back({JSONToken::COMMA, ","});
        } else if (line[i] == ':') {
          tokens.push_back({JSONToken::COLON, ":"});
        } else if (line[i] == '"') {
          try {
            scan_string(line, i);
          } catch (std::exception& e) {
            fmt::println("Error: {}", e.what());
          }
        } else if (line[i] == 't' || line[i] == 'f') {
          try {
            scan_bool(line, i);
          } catch (std::exception& e) {
            fmt::println("Error: {}", e.what());
          }
        } else if (line[i] == 'n') {
          try {
            scan_null(line, i);
          } catch (std::exception& e) {
            fmt::println("Error: {}", e.what());
          }
        } else if (std::isdigit(line[i]) || line[i] == '+' || line[i] == '-') {
          try {
            scan_number_integer(line, i);
          } catch (std::exception& e) {
            fmt::println("Error: {}", e.what());
          }
        } else {
          throw std::runtime_error("Invalid JSON: unexpected character");
        }
      }
    }
  }

  void scan_null(const std::string& s, int& i) {
    // null: null
    tokens.push_back({JSONToken::NULL_, "null"});
  }

  void scan_bool(const std::string& s, int& i) {
    // bool: true false
    if (s.substr(i, 4) == "true") {
      tokens.push_back({JSONToken::BOOL, "true"});
      i += 3;
    } else if (s.substr(i, 5) == "false") {
      tokens.push_back({JSONToken::BOOL, "false"});
      i += 4;
    } else {
      throw std::runtime_error("Invalid JSON: unexpected boolean format");
    }
  }

  void scan_string(const std::string& s, int& i) {
    // string: "..."
    int start = i;
    i++;
    for (; i < s.size(); i++) {
      if (s[i] == '"') {
        tokens.push_back({JSONToken::STRING, s.substr(start, i - start + 1)});
        return;
      }
    }
    throw std::runtime_error("Invalid JSON: unexpected string format");
  }

  void scan_number_integer(const std::string& s, int& i) {
    // number: 0.0 .0 0e+0 0.0e+0
    // integer: 0

    int dot_pos = -1;
    int e_pos = -1;
    int start = i;
    for (; i < s.size(); i++) {
      char c = s[i];
      if (std::isdigit(c)) {
        // do nothing
      } else if (c == '.') {
        if (dot_pos != -1 || e_pos != -1) {
          throw std::runtime_error("Invalid JSON: unexpected number format");
        }
        dot_pos = i;
      } else if (c == 'e' || c == 'E') {
        if (e_pos != -1) {
          throw std::runtime_error("Invalid JSON: unexpected number format");
        }
        e_pos = i;
      } else if (c == '+' || c == '-') {
        if (i > start && (s[i - 1] != 'e' && s[i - 1] != 'E')) {
          throw std::runtime_error("Invalid JSON: unexpected number format");
        }
      } else {
        break;
      }
    }

    if (dot_pos != -1 || e_pos != -1) {
      tokens.push_back({JSONToken::NUMBER, s.substr(start, i - start)});
    } else {
      tokens.push_back({JSONToken::INTEGER, s.substr(start, i - start)});
    }
  }

  std::vector<std::pair<JSONToken, std::string>> get_tokens() {
    // for (int i = 0; i < tokens.size(); i++) {
    //   fmt::print("token [{}]: {} {}\n", i, int(tokens[i].first),
    //              tokens[i].second);
    // }
    return tokens;
  }

 private:
  std::fstream file;
  std::vector<std::pair<JSONToken, std::string>> tokens;
};

class JSONParser {
 public:
  JSONParser(std::vector<std::pair<JSONToken, std::string>> tokens)
      : tokens(tokens) {}

  std::pair<JSONToken, std::string> get_next_token() {
    if (index >= tokens.size()) {
      fmt::println("Invalid JSON: unexpected end of tokens");
      return {JSONToken::NULL_, ""};
    }
    return tokens[index++];
  }

  std::pair<JSONToken, std::string> peek_next_token() {
    if (index >= tokens.size()) {
      fmt::println("Invalid JSON: unexpected end of tokens");
      return {JSONToken::NULL_, ""};
    }
    return tokens[index];
  }

  JSONObject parse() {
    auto [tokenType, token] = get_next_token();
    switch (tokenType) {
      case JSONToken::NULL_:
        return {};
      case JSONToken::BOOL:
        if (token == "true") {
          return JSONObject(true);
        } else {
          return JSONObject(false);
        }
      case JSONToken::STRING: {
        auto len = token.size();
        std::string str_value = token.substr(1, len - 2);
        // fmt::println("str_value: {} (from token {})", str_value, token);
        return JSONObject(str_value);
      }
      case JSONToken::LEFT_SQUARE_BRACKET:
        try {
          return parse_array();
        } catch (std::exception& e) {
          fmt::println("Error: {}", e.what());
        }
      case JSONToken::LEFT_CURLY_BRACKET:
        try {
          return parse_dict();
        } catch (std::exception& e) {
          fmt::println("Error: {}", e.what());
        }
      case JSONToken::INTEGER: {
        int int_value = std::stoi(token);
        // fmt::println("int_value: {} (from token {})", int_value, token);
        return JSONObject(int_value);
      }
      case JSONToken::NUMBER: {
        float float_value = std::stof(token);
        // fmt::println("float_value: {} (from token {})", float_value, token);
        return JSONObject(float_value);
      }
      default:
        throw std::runtime_error(
            "Invalid JSON: unexpected token, got " +
            fmt::format("token [{}] {}", int(tokenType), token));
    }
  }

  JSONObject parse_array() {
    std::vector<JSONObject> v;

    for (int i = 0; true; i++) {
      {
        auto [tokenType, token] = peek_next_token();
        if (tokenType == JSONToken::RIGHT_SQUARE_BRACKET) {
          get_next_token();
          return JSONObject(v);
        }
      }

      if (i > 0) {
        auto [tokenType, token] = get_next_token();
        if (tokenType != JSONToken::COMMA) {
          throw std::runtime_error(
              "Invalid JSON: expected `,` after value in array");
        }
      }

      auto value = parse();
      v.push_back(value);
    }

    throw std::runtime_error("Invalid JSON: expected `]`");
  }

  JSONObject parse_dict() {
    std::map<std::string, JSONObject> m;
    for (int i = 0; true; i++) {
      {
        auto [tokenType, token] = peek_next_token();
        if (tokenType == JSONToken::RIGHT_CURLY_BRACKET) {
          get_next_token();
          // fmt::println("m: {}", m.size());
          // fmt::println("content: {}", JSONObject(m).to_string());
          return JSONObject(m);
        }
      }

      if (i > 0) {
        auto [tokenType, token] = get_next_token();
        if (tokenType != JSONToken::COMMA) {
          throw std::runtime_error(
              "Invalid JSON: expected `,` after value in dictionary");
        }
      }
      
      std::string key;
      {
        auto strObj = parse();
        auto key_ptr = std::get_if<std::string>(&strObj.value);
        if (key_ptr == nullptr) {
          throw std::runtime_error(
              "Invalid JSON: expected string as key in dictionary");
        }
        key = *key_ptr;
      }

      {
        auto [tokenType, token] = get_next_token();
        if (tokenType != JSONToken::COLON) {
          throw std::runtime_error(
              "Invalid JSON: expected `:` after key in dictionary, got " +
              token);
        }
      }

      auto value = parse();
      m[key] = value;
    }

    throw std::runtime_error("Invalid JSON: expected `}`");
  }

 private:
  std::vector<std::pair<JSONToken, std::string>> tokens;
  int index = 0;
};

class JSON {
 public:
  JSON() : root(JSONObject()) {}

  void read_from_file(const std::string& filename) {
    JSONScanner scanner;
    try {
      scanner.read(filename);
    } catch (std::exception& e) {
      fmt::println("Error: {}", e.what());
    }

    auto tokens = scanner.get_tokens();
    // for (auto [tokenType, token] : tokens) {
    //   fmt::println("token: {} {}", int(tokenType), token);
    // }

    JSONParser parser(tokens);

    try {
      root = parser.parse();
    } catch (std::exception& e) {
      fmt::println("Error: {}", e.what());
    }
  }

  std::string to_string() {
    return root.to_string();
  }

  void write_to_file(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      fmt::println("Error: cannot open file");
    }
    file << root.to_string(2);
    file.close();
  }

private:
  JSONObject root;
};

}  // namespace sjson