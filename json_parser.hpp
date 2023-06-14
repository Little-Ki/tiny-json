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
		parser(const std::string& doc) : m_doc(doc) { m_doc.append("    "); }

		bool parse() {
			if (!try_parse()) {
				std::cout << "Parse json failed at line:" << m_line + 1 << ", col:" << m_col + 1 << '\n';
			}
			return true;
		}

		json::value document() {
			return m_json;
		}

		bool valid() {
			return m_json.type() != TYPE::INVALID;
		}

	private:
		bool try_parse() {
			m_pos = 0;
			m_line = 0;
			m_col = 0;
			if (!parse_value(m_json)) {
				return false;
			}
			return true;
		}

		bool parse_value(json::value& out) {
			SKIP_CHECK;
			if (peek() == '[') { 
					out = json::value(json::TYPE::ARRAY);
					if (!parse_array(out)) return false;
			}
			else if (peek() == '{') { 
					out = json::value(json::TYPE::DIRCTORY);
					if (!parse_object(out)) return false;
			}
			else if (peek() == '\"') { 
					std::string str;
					if (!parse_string(str)) return false;
					out = str;
			}
			else if (
				(peek() >= '0' && peek() <= '9') || 
				(peek() == '-')
			){ 
					std::string str;
					bool f, e;
					if (!parse_number(str, f, e)) return false;
					if (f) 
						out = std::stod(str);
					else 
						out = std::stoi(str);
			}
			else if (peek() == 't' || peek() == 'f') {
				bool val = false;
				if (!parse_boolean(val)) return false;
				out = val;
			}
			else if (peek() == 'n') {
				if (!parse_null()) return false;
				out.set_null();
			}
			return true;
		}

		bool parse_object(json::value& out) {
			if (peek_eat() != '{') return false;
			out = value::dic_t();

			while(!eof() && peek() != '}') {
				SKIP_CHECK;
				if (peek() != '\"') return false;
				std::string key;
				auto succ = parse_string(key);
				if (!succ) return false;
				SKIP_CHECK;
				if (peek_eat() != ':') return false;
				SKIP_CHECK;
				json::value val;
				if (!parse_value(val)) {
					return false;
				}
				out[key] = val;
				SKIP_CHECK;
				if (peek() == ',') {
					eat();
				}
			}
			if (eof()) return false;
			if (peek() != '}') { return false; }
			eat();
			return true;
		}

		bool parse_array(json::value& out) {
			if (peek_eat() != '[') return false;
			out = value::arr_t();

			while(!eof() && peek() != ']') {
				SKIP_CHECK;
				json::value val;
				if (!parse_value(val)) {
					return false;
				}
				out += val;
				SKIP_CHECK;
				if (peek() == ',') {
					eat();
				}
			}
			if (eof()) return false;
			if (peek() != ']') return false;
			eat();
			return true;
		}

		bool parse_string(std::string& out) {
			size_t len = 0;
			auto c = peek_eat();
			if (c != '\"') {
				return false;
			}
			c = peek();
			while(c != '\"' && !eof()) {
				if (c == '\\') {
					eat();
					char eat = peek_eat();
					if (eat == '/')	out.push_back('/');
					if (eat == '\\') out.push_back('\\');
					if (eat == 'b')	out.push_back('\b');
					if (eat == 'f')	out.push_back('\f');
					if (eat == 't')	out.push_back('\t');
					if (eat == 'n')	out.push_back('\n');
					if (eat == 'r')	out.push_back('\r');
					if (eat == '"')	out.push_back('\"');
					if (eat == 'u') {
						char h0 = peek_eat();
						char h1 = peek_eat();
						char h2 = peek_eat();
						char h3 = peek_eat();
						if (!is_hex(h0) || !is_hex(h1) || !is_hex(h2) || !is_hex(h3)) {
							return false;
						}
						h0 = hex2dec(h0);
						h1 = hex2dec(h1);
						h2 = hex2dec(h2);
						h3 = hex2dec(h3);
						auto unicode = (h0 << 12) | (h1 << 8)	| (h2 << 4) | h3;
						out.append(unicode_utf8(unicode));
					};
					c = peek();
				} else {
					auto size = utf8_len();
					while(size--) {
						out.push_back(peek_eat());
					}
				}
				c = peek();
			}
			if (peek() != '\"') return false;
			eat();
			return true;
		}

		bool parse_number(std::string& out, bool& f, bool& e) {
			auto c = peek_eat();
			if (c == '-') {
				if (peek() >= '0' && peek() <= '9') {
					out.push_back(c);
					return parse_number(out, f, e);
				} return false;
			}

			bool valid = false;
			size_t d_at = 0, e_at = 0;

			if (!(c >= '0' && c <= '9')) { return false; }
			if (c == '0' && peek() != '.') { return false; }
			out.push_back(c);

			c = peek();
			while ((c >= '0' && c <= '9') || c == '.') {
				out.push_back(c);
				if (c == '.') {
					valid = false;
					if (d_at != 0) return false; 
					else d_at = m_pos;
				}
				if (c >= '0' && c <= '9') valid = true;
				eat();
				c = peek();
			};
			
			if (c == 'e') {
				out.push_back(c);
				eat();
				c = peek();
				valid = false;
				e_at = m_pos;

				if (c == '-') {
					out.push_back(c);
					eat();
					c = peek();
					if (!(c >= '0' && c <= '9')) return false;
				}

				while (c >= '0' && c <= '9') {
					valid = true;
					out.push_back(c);
					eat();
					c = peek();
				}
			}

			f = d_at != 0;
			e = e_at != 0;
			return valid && ((d_at != 0 && e_at != 0) ? (e_at > d_at && e_at - d_at > 1) : true);
		}

		bool parse_boolean(bool& out) {
			auto c = peek_eat();
			if (c != 't' && c != 'f') return false;
			if (c == 't') {
				if (peek_eat() != 'r') return false;
				if (peek_eat() != 'u') return false;
				if (peek_eat() != 'e') return false;
				out = true;
			}
			if (c == 'f') {
				if (peek_eat() != 'a') return false;
				if (peek_eat() != 'l') return false;
				if (peek_eat() != 's') return false;
				if (peek_eat() != 'e') return false;
				out = false;
			}
			return true;
		}

		bool parse_null() {
			auto c = peek_eat();
			if (c != 'n') return false;
			if (peek_eat() != 'u') return false;
			if (peek_eat() != 'l') return false;
			if (peek_eat() != 'l') return false;
			return true;
		}

		void skip_space() {
			auto c = peek();
			while(
				!eof() && 
				(c == ' ' || c == '\n' || c == '\t' || c == '\r')
			) {
				if (c == '\n') {
					m_col = 0;
					m_line += 1;
				}
				eat();
				c = peek();
			}
		}

		bool eof() { return m_pos >= m_doc.size(); }

		char peek() { return m_doc[m_pos]; }

		char peek_eat() { auto c = peek(); eat(); return c; }

		void eat() { m_pos += 1; m_col += 1; }

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

		std::string unicode_utf8(wchar_t u) {
			if (u <= 0x7F) {
				return { (char) (u & 0x7F) };
			} 
			else if (u <= 0x7FF) {
				return { (char) (((u >> 6) & 0x1F) | 0xC0), (char) ((u & 0x3F) | 0x80) };
			}
			else if (u <= 0xFFFF) {
				return { (char) (((u >> 12) & 0x0F) | 0xE0), (char) (((u >> 6) & 0x3F) | 0x80), (char) ((u & 0x3F) | 0x80) };
			}
			return {};
		}

	public:
		std::string m_doc;

		size_t m_pos { 0 };

		size_t m_line { 0 }, m_col { 0 };

		json::value m_json;

	};
}
