# simple json

一个简单的json解析器

## 用法

```cpp

#include "sjson.hpp"

int main() {
  sjson::JSON json;
  json.read_from_file("test.json");
  json.write_to_file("test_out.json");
  return 0;
}

```