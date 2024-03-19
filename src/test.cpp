#include <fmt/core.h>

#include "sjson.hpp"

int main(int argc, char* argv[]) {
  fmt::println("Hello, world!");
  sjson::JSON json;
  for (int i = 0; i < argc; ++i) {
    // fmt::print("argv[{}] = {}\n", i, argv[i]);
  
    if (argv[i] == std::string("--file") || argv[i] == std::string("-f")) {
      if (i + 1 < argc) {
        fmt::println("File provided: {}", argv[i + 1]);
        json.read_from_file(argv[i + 1]);
        fmt::println("OK");
      } else {
        fmt::println("No file provided");
      }
    }

    if (argv[i] == std::string("--print") || argv[i] == std::string("-p")) {
      fmt::println("Printing JSON");
      fmt::println("{}", json.to_string());
    }

    if (argv[i] == std::string("--output") || argv[i] == std::string("-o")) {
      if (i + 1 < argc) {
        fmt::println("Output file: {}", argv[i + 1]);
        json.write_to_file(argv[i + 1]);
      } else {
        fmt::println("No output file provided");
      }
    }
  }

  return 0;
}