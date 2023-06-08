# TinyJson
A very simple json parser;

**NOTE:** Require utf-8 input format.

**NOTE:** I don't know how to solve the '\uXXXX' unicode in string, so I just ignore it.

**usage** 
```cpp
void demo() {
  auto json_text = R"(
    { 
      "string": "hello",
      "int": 100,
      "double": 1.00e1,
      "null": null,
      "ok": true,
      "no": false,
      "array": [
        {
          "inner": "world"
        },
        123.456
      ]
    }
  )";
  json::parser jp(json_text);
  auto success = jp.parse();
  if (success) {
    auto doc = jp.document();
    std::cout << doc["string"].as<std::string>() << '\n';
    std::cout << doc["int"].as<int>() << '\n';
    std::cout << doc["double"].as<double>() << '\n';
    std::cout << doc["null"].is_null() << '\n';
    std::cout << doc["ok"].as<bool>() << '\n';
    std::cout << doc["no"].as<bool>() << '\n';
    std::cout << doc["array"][0]["inner"].as<std::string>() << '\n';
    std::cout << doc["array"][1].as<double>() << '\n';
  }
}
```
