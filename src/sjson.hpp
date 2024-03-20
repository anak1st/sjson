#include <fmt/core.h>

#include <cctype>
#include <cstddef>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>
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

enum class JSONType {
  NULL_,
  BOOL,
  INTEGER,  // int
  NUMBER,   // float
  STRING,   // string
  ARRAY,    // array
  DICT      // dictionary
};

class JSONObject {
 public:
  
  using Array = std::vector<std::shared_ptr<JSONObject>>;
  using Dict = std::map<std::string, std::shared_ptr<JSONObject>>;
  using ValueType = std::variant<bool, int, float, std::string, Array, Dict>;

  template <typename T>
  using good_type =
      std::disjunction<std::is_same<T, bool>, std::is_same<T, int>,
                       std::is_same<T, float>, std::is_same<T, std::string>,
                       std::is_same<T, Array>, std::is_same<T, Dict>>;

  JSONObject() : type(JSONType::NULL_) {}
  template <typename T>
    requires good_type<T>::value
  explicit JSONObject(T v) {
    set<T>(v);
  }

  ~JSONObject() {
    // fmt::println("destructor: !!!");
  }

  template <typename T>
    requires good_type<T>::value
  void set(T v) {
    if (std::is_same_v<T, bool>) {
      type = JSONType::BOOL;
    } else if (std::is_same_v<T, int>) {
      type = JSONType::INTEGER;
    } else if (std::is_same_v<T, float>) {
      type = JSONType::NUMBER;
    } else if (std::is_same_v<T, std::string>) {
      type = JSONType::STRING;
    } else if (std::is_same_v<T, Array>) {
      type = JSONType::ARRAY;
    } else if (std::is_same_v<T, Dict>) {
      type = JSONType::DICT;
    } else {
      fmt::println("Error: unknown type");
    }

    value = std::make_shared<ValueType>(std::move(v));

    // fmt::println("set: {}", to_string());
  }

  template <typename T>
    requires good_type<T>::value
  T* get() {
    auto ptr = std::get_if<T>(value.get());
    return ptr;
  }

  JSONType get_type() { return type; }

  std::string to_string(int depth = 0) {
    std::string out;

    auto add = [&](const std::string& s) {
      out += std::string(depth, ' ') + s;
    };

    switch (type) {
      case JSONType::NULL_:
        return "null";
      case JSONType::BOOL:
        return *get<bool>() ? "true" : "false";
      case JSONType::INTEGER:
        return std::to_string(*get<int>());
      case JSONType::NUMBER:
        return std::to_string(*get<float>());
      case JSONType::STRING:
        return "\"" + *get<std::string>() + "\"";
      case JSONType::ARRAY: {
        out += "[\n";
        auto array = get<Array>();
        for (int i = 0; i < array->size(); i++) {
          add(array->at(i)->to_string(depth + 2));
          if (i < array->size() - 1) {
            out += ",";
          }
          out += "\n";
        }
        if (depth > 2) {
          out += std::string(depth - 2, ' ');
        }
        out += "]";
        break;
      }
      case JSONType::DICT: {
        out += "{\n";
        auto dict = get<Dict>();
        for (auto it = dict->begin(); it != dict->end(); it++) {
          add("\"" + it->first + "\": " + it->second->to_string(depth + 2));
          if (std::next(it) != dict->end()) {
            out += ",";
          }
          out += "\n";
        }
        if (depth > 2) {
          out += std::string(depth - 2, ' ');
        }
        out += "}";
        break;
      }
      default: {
        fmt::println("Error: unknown type");
        break;
      }
    }
    return out;
  }
 
 private:
  JSONType type;
  std::shared_ptr<ValueType> value = nullptr;
};

using Array = JSONObject::Array;
using Dict = JSONObject::Dict;


class JSONScanner {
 public:
  JSONScanner() = default;

  void read(const std::string& filename) {
    file.open(filename);
    if (!file.is_open()) {
      fmt::println("JSON Scan Error: cannot open file");
      return;
    }
    scan();
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
          tokens.push_back(scan_string(line, i));
        } else if (line[i] == 't' || line[i] == 'f') {
          tokens.push_back(scan_bool(line, i));
        } else if (line[i] == 'n') {
          tokens.push_back(scan_null(line, i));
        } else if (std::isdigit(line[i]) || line[i] == '+' || line[i] == '-') {
          tokens.push_back(scan_number_integer(line, i));
        } else {
          fmt::println("JSON Scan Error: unexpected character {}", line[i]);
          return;
        }
      }
    }
  }

  std::pair<JSONToken, std::string> scan_null(const std::string& s, int& i) {
    // null: null
    if (s.substr(i, 4) == "null") {
      i += 3;
      return std::make_pair(JSONToken::NULL_, "null");
    }
    fmt::println("JSON Scan Error: unexpected null format");
    return std::make_pair(JSONToken::NULL_, "");
  }

  std::pair<JSONToken, std::string> scan_bool(const std::string& s, int& i) {
    // bool: true false
    if (s.substr(i, 4) == "true") {
      i += 3;
      return std::make_pair(JSONToken::BOOL, "true");
    }
    if (s.substr(i, 5) == "false") {
      i += 4;
      return std::make_pair(JSONToken::BOOL, "false");
    }
    fmt::println("JSON Scan Error: unexpected bool format");
    return std::make_pair(JSONToken::NULL_, "");
  }

  std::pair<JSONToken, std::string> scan_string(const std::string& s, int& i) {
    // string: "..."
    int start = i;
    i++;
    for (; i < s.size(); i++) {
      if (s[i] == '"') {
        return std::make_pair(JSONToken::STRING, s.substr(start, i - start + 1));
      }
    }
    fmt::println("JSON Scan Error: unexpected string format");
    return std::make_pair(JSONToken::NULL_, "");
  }

  std::pair<JSONToken, std::string> scan_number_integer(const std::string& s, int& i) {
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
          fmt::println("JSON Scan Error: unexpected number format");
          return std::make_pair(JSONToken::NULL_, "");
        }
        dot_pos = i;
      } else if (c == 'e' || c == 'E') {
        if (e_pos != -1) {
          fmt::println("JSON Scan Error: unexpected number format");
          return std::make_pair(JSONToken::NULL_, "");
        }
        e_pos = i;
      } else if (c == '+' || c == '-') {
        if (i > start && (s[i - 1] != 'e' && s[i - 1] != 'E')) {
          fmt::println("JSON Scan Error: unexpected number format");
          return std::make_pair(JSONToken::NULL_, "");
        }
      } else {
        break;
      }
    }

    if (dot_pos != -1 || e_pos != -1) {
      return std::make_pair(JSONToken::NUMBER, s.substr(start, i - start));
    } else {
      return std::make_pair(JSONToken::INTEGER, s.substr(start, i - start));
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
  explicit JSONParser(const std::vector<std::pair<JSONToken, std::string>> &tokens)
      : tokens(tokens) {}

  std::pair<JSONToken, std::string> get_next_token() {
    if (index >= tokens.size()) {
      fmt::println("JSON Parser Error: unexpected end of tokens");
      return {JSONToken::NULL_, ""};
    }
    return tokens[index++];
  }

  std::pair<JSONToken, std::string> peek_next_token() {
    if (index >= tokens.size()) {
      fmt::println("JSON Parser Error: unexpected end of tokens");
      return {JSONToken::NULL_, ""};
    }
    return tokens[index];
  }

  std::shared_ptr<JSONObject> parse() {
    auto [tokenType, token] = get_next_token();
    switch (tokenType) {
      case JSONToken::NULL_:
        return std::make_shared<JSONObject>();
      case JSONToken::BOOL:
        if (token == "true") {
          return std::make_shared<JSONObject>(true);
        } else {
          return std::make_shared<JSONObject>(false);
        }
      case JSONToken::STRING: {
        auto len = token.size();
        std::string str_value = token.substr(1, len - 2);
        // fmt::println("str_value: {} (from token {})", str_value, token);
        return std::make_shared<JSONObject>(str_value);
      }
      case JSONToken::LEFT_SQUARE_BRACKET:
        return parse_array();
      case JSONToken::LEFT_CURLY_BRACKET:
        return parse_dict();
      case JSONToken::INTEGER: {
        int int_value = std::stoi(token);
        // fmt::println("int_value: {} (from token {})", int_value, token);
        return std::make_shared<JSONObject>(int_value);
      }
      case JSONToken::NUMBER: {
        float float_value = std::stof(token);
        // fmt::println("float_value: {} (from token {})", float_value, token);
        return std::make_shared<JSONObject>(float_value);
      }
      default: {
        fmt::println("JSON Parser Error: unexpected token type {}", int(tokenType));
        return std::make_shared<JSONObject>();
      }
    }
  }

  std::shared_ptr<JSONObject> parse_array() {
    Array v;

    for (int i = 0; true; i++) {
      {
        auto [tokenType, token] = peek_next_token();
        if (tokenType == JSONToken::RIGHT_SQUARE_BRACKET) {
          get_next_token();
          return std::make_shared<JSONObject>(v);
        }
      }

      if (i > 0) {
        auto [tokenType, token] = get_next_token();
        if (tokenType != JSONToken::COMMA) {
          fmt::println("JSON Parser Error: expected `,` after value in array, got {}", token);
          return std::make_shared<JSONObject>();
        }
      }

      auto value = parse();
      v.push_back(value);
    }

    fmt::println("JSON Parser Error: expected `]`");
    return std::make_shared<JSONObject>();
  }

  std::shared_ptr<JSONObject> parse_dict() {
    Dict m;
    for (int i = 0; true; i++) {
      {
        auto [tokenType, token] = peek_next_token();
        if (tokenType == JSONToken::RIGHT_CURLY_BRACKET) {
          get_next_token();
          // fmt::println("m: {}", m.size());
          // fmt::println("content: {}", JSONObject(m).to_string());
          return std::make_shared<JSONObject>(m);
        }
      }

      if (i > 0) {
        auto [tokenType, token] = get_next_token();
        if (tokenType != JSONToken::COMMA) {
          fmt::println("JSON Parser Error: expected `,` after value in dictionary, got {}",
                       token);
          return std::make_shared<JSONObject>();
        }
      }

      std::string key;
      {
        auto strObj = parse();

        if (strObj->get_type() != JSONType::STRING) {
          fmt::println("JSON Parser Error: expected string as key in dictionary, got {}",
                       strObj->to_string());
          return std::make_shared<JSONObject>();
        }

        auto key_ptr = strObj->get<std::string>();
        key = *key_ptr;
      }

      {
        auto [tokenType, token] = get_next_token();
        if (tokenType != JSONToken::COLON) {
          fmt::println("JSON Parser Error: expected `:` after key in dictionary, got {}",
                       token);
          return std::make_shared<JSONObject>();
        }
      }

      auto value = parse();
      m[key] = value;
    }

    fmt::println("JSON Parser Error: expected `}}`");
    return std::make_shared<JSONObject>();
  }

 private:
  std::vector<std::pair<JSONToken, std::string>> tokens;
  int index = 0;
};

class JSON {
 public:
  JSON() : root(nullptr) {
    fmt::println("constructor a null JSON !!!");
    root = std::make_shared<JSONObject>(Dict());
  }

  JSON(std::shared_ptr<JSONObject> _root) : root(std::move(_root)) {}

  void read_from_file(const std::string& filename) {
    JSONScanner scanner;
    scanner.read(filename);

    auto tokens = scanner.get_tokens();
    // for (auto [tokenType, token] : tokens) {
    //   fmt::println("token: {} {}", int(tokenType), token);
    // }

    JSONParser parser(tokens);
    root = parser.parse();
  }

  std::string to_string() { return root->to_string(); }

  void write_to_file(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      fmt::println("JSON Error: cannot open file");
      return;
    }
    file << root->to_string(2);
    file.close();
  }

  JSON operator[](std::string_view key) {
    auto key_str = std::string(key);
    if (root->get_type() == JSONType::DICT) {
      auto dict = root->get<Dict>();
      if (dict->find(key_str) == dict->end()) {
        fmt::println("JSON Warning: key [{}] not found, creating new element", key);
        dict->insert({key_str, std::make_shared<JSONObject>()});
      }
      return {dict->at(key_str)};
    }
    fmt::println("JSON Error: root is not a dictionary");
    return {};
  }

  JSON operator[](size_t index) {
    if (root->get_type() == JSONType::ARRAY) {
      auto array = root->get<Array>();
      while (index >= array->size()) {
        fmt::println("JSON Warning: index [{}] out of range, creating new element", index);
        array->push_back(std::make_shared<JSONObject>());
      }
      return {array->at(index)};
    }
    fmt::println("JSON Error: root is not an array");
    return {};
  }

  template <typename T>
  JSON& operator=(T v) {
    fmt::println("operator= !!!");
    root->set<T>(v);
    fmt::println("root: {}", root->to_string());
    return *this;
  }

 // private:
  std::shared_ptr<JSONObject> root;
};

}  // namespace sjson