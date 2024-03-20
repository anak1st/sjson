#include <fmt/core.h>
#include <cassert>
#include <cstdint>

#include "sjson.hpp"

int main(int argc, char* argv[]) {
  fmt::println("Hello, world!");
  
  std::string file_name_in = "D:/GitHub/cpp/sjson/test/test.json";
  std::string file_name_out = "D:/GitHub/cpp/sjson/test/test_out.json";
  bool print_json = false;

  for (int i = 0; i < argc; ++i) {
    // fmt::print("argv[{}] = {}\n", i, argv[i]);
  
    if (argv[i] == std::string("--file") || argv[i] == std::string("-f")) {
      if (i + 1 < argc) {
        file_name_in = argv[i + 1];
      } else {
        fmt::println("No file provided");
        return -1;
      }
    }

    if (argv[i] == std::string("--print") || argv[i] == std::string("-p")) {
      print_json = true;
    }

    if (argv[i] == std::string("--output") || argv[i] == std::string("-o")) {
      if (i + 1 < argc) {
        file_name_out = argv[i + 1];
      } else {
        fmt::println("No output file provided");
      }
    }
  }

  sjson::JSON json;
  json.read_from_file(file_name_in);

  if (print_json) {
    fmt::println("JSON: {}", json.to_string());
  }

  fmt::println("JSON: {}", json[std::string("configurations")][0].to_string());

  json["config"] = std::string("new value");

  fmt::println("JSON after modify: {}", json["config"].to_string());

  json["config"] = 123;

  fmt::println("JSON after modify: {}", json["config"].to_string());

  json.write_to_file(file_name_out);

  return 0;
}