#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <codecvt> 
#include <locale> 

#include "json_value.hpp"

namespace json {

  class parser {
  private:
  
    #define SKIP_CHECK { skip_space(); if (eof()) return false; }
  public:
    parser(const std::string& doc) : m_doc(doc) {}

    bool parse() {
      auto succ = parse_t();
      if (!succ) {
        size_t line = 0, col = 0;
        size_t old_pos = m_pos;
        for(auto i = 0; i < old_pos;) {
          if (m_doc[i] == '\n') {
            line++;
            col = 0;
          } else col++;
          i += utf8_len();
          m_pos = i;
        }
        m_pos = old_pos;
        std::cout << "Parse json failed at line:" << line << ", col:" << col << '\n';
      }
      return succ;
    }

    json::value document() {
      return m_result;
    }

    bool valid() {
      return m_result.type() != VALTYPE::UNKNOWN;
    }

  private:
    
    bool parse_t() {
      
      m_pos = 0;
      SKIP_CHECK;

      /**/ if (peek() == '[') { 
          m_result = json::value(value::arr_t());
          if (!parse_array(m_result)) return false;
      }
      else if (peek() == '{') { 
          m_result = json::value(value::dic_t());
          if (!parse_object(m_result)) return false;
      }
      else if (peek() == '\"') { 
          std::string str;
          if (!parse_string(str)) return false;
          m_result = json::value(str);
      }
      else if (peek() >= '0' && peek() <= '9'){ 
          std::string str;
          bool floating;
          if (!parse_number(str, floating)) return false;
          m_result = floating ? json::value(std::stod(str)) : json::value(std::stoi(str));
      }
      else if (peek() == 't' || peek() == 'f') {
        bool result = false;
        if (!parse_boolean(result)) return false;
        m_result = json::value(result);
      }
      else if (peek() == 'n') {
        if (!parse_null()) return false;
        m_result = json::value();
      }
      return true;
    }

    bool parse_object(json::value& out) {
      if (peek_next() != '{') return false;
      while(!eof() && peek() != '}') {
        SKIP_CHECK;
        if (peek() != '\"') return false;

        std::string key;
        auto succ = parse_string(key);
        if (!succ) return false;

        SKIP_CHECK;
        if (peek_next() != ':') return false;
        SKIP_CHECK;
        
        /**/ if (peek() == '{') {
          auto tmp = json::value(value::dic_t());
          if (!parse_object(tmp)) return false;
          out.set(key, tmp);
        }
        else if (peek() == '[') {
          auto tmp = json::value(value::arr_t());
          if (!parse_array(tmp)) return false;
          out.set(key, tmp);
        }
        else if (peek() == '\"') {
          std::string str;
          if (!parse_string(str)) return false;
          out.set(key, json::value(str));
        }
        else if (peek() >= '0' && peek() <= '9') {
          std::string str;
          bool floating;
          if (!parse_number(str, floating)) return false;
          out.set(key, floating ? json::value(std::stod(str)) : json::value(std::stoi(str)));
        }
        else if (peek() == 't' || peek() == 'f') {
          bool result = false;
          if (!parse_boolean(result)) return false;
          out.set(key, json::value(result));
        }
        else if (peek() == 'n') {
          if (!parse_null()) return false;
          out.set(key, json::value());
        }

        SKIP_CHECK;
        if (peek() == ',') {
          next();
        }
      }
      if (eof()) return false;
      if (peek() != '}') { return false; }
      next();
      return true;
    }

    bool parse_array(json::value& out) {
      if (peek_next() != '[') return false;
      while(!eof() && peek() != ']') {
        SKIP_CHECK;
        /**/ if (peek() == '{') {
          auto tmp = json::value(value::dic_t());
          if (!parse_object(tmp)) return false;
          out.append(tmp);
        }
        else if (peek() == '[') {
          auto tmp = json::value(value::arr_t());
          if (!parse_array(tmp)) return false;
          out.append(tmp);
        }
        else if (peek() == '\"') {
          std::string str;
          if (!parse_string(str)) return false;
          out.append(json::value(str));
        }
        else if (peek() >= '0' && peek() <= '9') {
          std::string str;
          bool floating;
          if (!parse_number(str, floating)) return false;
          out.append(floating ? json::value(std::stod(str)) : json::value(std::stoi(str)));
        }
        else if (peek() == 't' || peek() == 'f') {
          bool result = false;
          if (!parse_boolean(result)) return false;
          out.append(json::value(result));
        }
        else if (peek() == 'n') {
          if (!parse_null()) return false;
          out.append(json::value());
        }
        SKIP_CHECK;
        if (peek() == ',') {
          next();
        }
      }
      if (eof()) return false;
      if (peek() != ']') return false;
      next();
      return true;
    }

    bool parse_string(std::string& out) {
      size_t len = 0;
      auto c = peek_next();
      if (c != '\"') {
        return false;
      }
      c = peek();
      while(c != '\"' && !eof()) {
        if (c == '\\') {
          next();
          char next = peek_next();
          if (next == '/')  out.push_back('/');
          if (next == '\\') out.push_back('\\');
          if (next == 'b')  out.push_back('\b');
          if (next == 'f')  out.push_back('\f');
          if (next == 't')  out.push_back('\t');
          if (next == 'n')  out.push_back('\n');
          if (next == 'r')  out.push_back('\r');
          if (next == '"')  out.push_back('\"');
          if (next == 'u') {
            unsigned char h0 = peek_next();
            unsigned char h1 = peek_next();
            unsigned char h2 = peek_next();
            unsigned char h3 = peek_next();
            if (!is_hex(h0) || !is_hex(h1) || !is_hex(h2) || !is_hex(h3)) {
              return false;
            }
            // just ignore
          };
          c = peek();
        } else {
          auto size = utf8_len();
          while(size--) {
            out.push_back(peek_next());
          }
        }
        c = peek();
      }
      if (peek() != '\"') return false;
      next();
      return true;
    }

    bool parse_number(std::string& out, bool& floating) {
      size_t dot = 0, e = 0;
      auto c = peek();
      bool valid = false;
      if (!(c >= '0' && c <= '9')) { return false; }
      while ((c >= '0' && c <= '9') || c == '.' || c == 'e') {
        out.push_back(c);
        if (c == '.') {
          valid = false;
          if (dot != 0) {
            break;
          }
          else dot = m_pos;
        }
        if (c == 'e') {
          valid = false;
          if (e != 0)  {
            break;
          }
          else e = m_pos;
        }
        if (c >= '0' && c <= '9') {
          valid = true;
        }
        next();
        c = peek();
      };
      floating = dot != 0;
      return valid && ((dot != 0 && e != 0) ? (e > dot && e - dot > 1) : true);
    }

    bool parse_boolean(bool& out) {
      auto c = peek_next();
      if (c != 't' && c != 'f') return false;
      if (c == 't') {
        if (peek_next() != 'r') return false;
        if (peek_next() != 'u') return false;
        if (peek_next() != 'e') return false;
        out = true;
      }
      if (c == 'f') {
        if (peek_next() != 'a') return false;
        if (peek_next() != 'l') return false;
        if (peek_next() != 's') return false;
        if (peek_next() != 'e') return false;
        out = false;
      }
      return true;
    }

    bool parse_null() {
      auto c = peek_next();
      if (c != 'n') return false;
      if (peek_next() != 'u') return false;
      if (peek_next() != 'l') return false;
      if (peek_next() != 'l') return false;
      return true;
    }

    void skip_space() {
      auto c = peek();
      while(
        !eof() && 
        (c == ' ' || c == '\n' || c == '\t' || c == '\r')
      ) {
        next();
        c = peek();
      }
    }

    bool eof() { return m_pos == m_doc.size(); }

    char peek() { return m_doc[m_pos]; }

    char peek_next() { auto c = peek(); next(); return c; }

    void next() { m_pos += 1; }

    size_t utf8_len() {
      auto d = (const unsigned char*)m_doc.data();
      auto c = d[m_pos];
      /**/ if ((c & 0xF8) == 0xF0) return 4U;
      else if ((c & 0xF0) == 0xE0) return 3U; 
      else if ((c & 0xE0) == 0xC0) return 2U;
      else if ((c & 0x80) == 0x00) return 1U;
      return 1U;
    }
    
    uint8_t hex2dec(char val) {
      if (val >= '0' && val <= '9') return val - '0';
      if (val >= 'a' && val <= 'f') return val - 'a' + 10;
      if (val >= 'A' && val <= 'F') return val - 'A' + 10;
      return 0;
    }

    bool is_hex(char val) {
      return 
      (val >= '0' && val <= '9') || 
      (val >= 'a' && val <= 'f') ||
      (val >= 'A' && val <= 'F') ;
    }

  public:
    std::string m_doc;

    size_t m_pos = 0;

    json::value m_result;
  };
}
