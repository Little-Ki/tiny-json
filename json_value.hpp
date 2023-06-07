#pragma once
#include <string>
#include <any>
#include <unordered_map>
#include <vector>

namespace json {

  template <typename T>
  constexpr size_t hash_type()
  {
      size_t result{};
      #ifdef _MSC_VER 
        #define F __FUNCSIG__
      #else 
				#define F __PRETTY_FUNCTION__
      #endif
      for (const auto &c : F)
          (result ^= c) <<= 1;
      return result;
  };

  enum class VALTYPE {
    UNKNOWN,
    STRING,
    BOOLEAN,
    NUMBER,
    OBJECT,
    ARRAY,
    NULLVAL
  };

  class value {
  public:
    using dic_t = std::unordered_map<std::string, json::value>;
    using arr_t = std::vector<json::value>;

		value() {}

    template<typename T>
    value(T val) {
      m_hash = hash_type<T>();
			clear();
      set<T>(val);
    }

		~value() { clear(); }

    template<typename T>
    void set(T val) {
      /**/ if constexpr (std::is_same_v<T, std::string>){
				m_type = VALTYPE::STRING;
			} 
      else if constexpr (std::is_same_v<T, bool>){
        m_type = VALTYPE::BOOLEAN;
      } 
      else if constexpr (std::is_arithmetic_v<T>){
        m_type = VALTYPE::NUMBER;
      } 
			else if constexpr (std::is_same_v<T, std::initializer_list<json::value>>){
        m_type = VALTYPE::ARRAY;
      }
			else if constexpr (std::is_same_v<T, arr_t>){
        m_type = VALTYPE::ARRAY;
      }
			else if constexpr (std::is_same_v<T, dic_t>){
				m_type = VALTYPE::OBJECT;
			}
			else {
				m_type = VALTYPE::UNKNOWN;
			}
      if (m_type != VALTYPE::UNKNOWN) {
        m_val = val;
      }
    }

    void set(const char* str) {
      set(std::string(str));
    }

		void clear() {
			if (m_type == VALTYPE::ARRAY) {
				(std::any_cast<arr_t&>(m_val)).clear();
			}
			if (m_type == VALTYPE::OBJECT) {
				(std::any_cast<dic_t&>(m_val)).clear();
			}
		}

    template<typename T>
    T as(const T& _default = T{}) {
			static_assert(
				std::is_same_v<T, std::string> ||
				std::is_arithmetic_v<T> || 
				std::is_same_v<T, bool>
			);
      if (m_type == VALTYPE::UNKNOWN || m_type == VALTYPE::NULLVAL) {
        return _default;
      }
      /**/ if constexpr (std::is_same_v<T, std::string>) {
        if (m_type != VALTYPE::STRING) 
					return _default;
      }
      else if constexpr (std::is_same_v<T, bool>) {
        if (m_type != VALTYPE::BOOLEAN) return _default;
      }
      else if constexpr (std::is_arithmetic_v<T>) {
        if (m_type != VALTYPE::NUMBER) return _default;
				if (m_hash == hash_type<typename std::remove_cv_t<float>>()) {
					return static_cast<T>(std::any_cast<float>(m_val));
				}
				if (m_hash == hash_type<typename std::remove_cv_t<double>>()) {
					return static_cast<T>(std::any_cast<double>(m_val));
				}
				if (m_hash == hash_type<typename std::remove_cv_t<int>>()) {
					return static_cast<T>(std::any_cast<int>(m_val));
				}
      }
      return std::any_cast<T>(m_val);
    }

    template<typename T>
    void operator=(const T& val) {
			static_assert(
				std::is_arithmetic_v<T> || 
				std::is_same_v<T, bool>
			);
      
    }

		void append(const value& val) {
			if (m_type == VALTYPE::ARRAY) {
				auto& vec = std::any_cast<std::vector<json::value>&>(m_val);
				vec.push_back(val);
			}
		}

		void set(const std::string& key, const value& val) {
			if (m_type == VALTYPE::OBJECT) {
				auto& map = std::any_cast<dic_t&>(m_val);
				map[key] = val;
			}
		}

		void set(const std::string& key, const char* val) {
			if (m_type == VALTYPE::OBJECT) {
				auto& map = std::any_cast<dic_t&>(m_val);
				map[key] = json::value(std::string(val));
			}
		}

		value& operator[](const std::string& key) {
			if (m_type == VALTYPE::OBJECT) {
				auto& map = std::any_cast<dic_t&>(m_val);
				if (map.find(key) == map.end()) {
					return *this;
				}
				return map[key];
			}
			return *this;
		}

		value& operator[](const size_t index) {
			if (m_type == VALTYPE::ARRAY) {
				auto& vec = std::any_cast<arr_t&>(m_val);
				if (index < vec.size()) 
					return vec[index];
				return *this;
			}
			return *this;
		}

		bool is_null() { return m_type == VALTYPE::NULLVAL; }

		VALTYPE type() { return m_type; }

  private:
    std::any m_val;
    
    VALTYPE m_type { VALTYPE::UNKNOWN };

    size_t m_hash { 0 };
  };

}