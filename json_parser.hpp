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

	enum class TYPE {
		INVALID,
		STRING,
		BOOLEAN,
		FLOAT,
		INTEGER,
		DOUBLE,
		DIRCTORY,
		ARRAY,
		NULLVAL
	};
	
	class value {
	public:
		using dic_t = std::unordered_map<std::string, json::value>;
		using arr_t = std::vector<json::value>;

	public:
		value() = default;

		value(json::TYPE type) : m_type(type) {
			if (type == json::TYPE::STRING) {
				m_val	 = std::string("");
				m_hash	= hash_type<std::string>();
			}
			if (type == json::TYPE::BOOLEAN) {
				m_val	 = false;
				m_hash	= hash_type<bool>();
			}
			if (type == json::TYPE::FLOAT) {
				m_val	 = 0.0f;
				m_hash	= hash_type<float>();
			}
			if (type == json::TYPE::INTEGER) {
				m_val	 = 0;
				m_hash	= hash_type<int>();
			}
			if (type == json::TYPE::DOUBLE) {
				m_val	 = 0.0;
				m_hash	= hash_type<double>();
			}
			if (type == json::TYPE::DIRCTORY) {
				m_val	 = dic_t();
				m_hash	= hash_type<dic_t>();
			}
			if (type == json::TYPE::ARRAY) {
				m_val	 = arr_t();
				m_hash	= hash_type<arr_t>();
			}
		}

		template<typename T>
		value(const T& val) {
			operator=<T>(val);
		}

		~value() { clear(); }

		template<typename T>
		void operator=(const T& val) {
			auto __set = [&](json::TYPE t) {
				m_type = t;
				m_val = val;
				m_hash = hash_type<T>();
			};
			/**/ if constexpr (std::is_same_v<T, std::string>) {
				__set(json::TYPE::STRING);
			}
			else if constexpr (std::is_same_v<T, bool>) {
				__set(json::TYPE::BOOLEAN);
			}
			else if constexpr (std::is_same_v<T, int>) {
				__set(json::TYPE::INTEGER);
			}
			else if constexpr (std::is_same_v<T, double>) {
				__set(json::TYPE::DOUBLE);
			}
			else if constexpr (std::is_same_v<T, float>) {
				__set(json::TYPE::FLOAT);
			}
			else if constexpr (std::is_same_v<T, dic_t>) {
				__set(json::TYPE::DIRCTORY);
			}
			else if constexpr (std::is_same_v<T, arr_t>) {
				__set(json::TYPE::ARRAY);
			}
		} 

		void operator=(const std::initializer_list<std::pair<std::string, json::value>>& val) { 
			clear();
			m_type = json::TYPE::DIRCTORY;
			m_val = dic_t();
			m_hash = hash_type<dic_t>();
			auto& map = std::any_cast<dic_t&>(m_val);
			for(auto& p : val) {
				map[p.first] = p.second;
			}
		}

		void operator=(const std::initializer_list<json::value>& val) { 
			clear();
			m_type = json::TYPE::ARRAY;
			m_val = dic_t();
			m_hash = hash_type<dic_t>();
			auto& arr = std::any_cast<arr_t&>(m_val);
			for(auto& p : val) {
				arr.push_back(p);
			}
		}

		template<typename T>
		void operator=(const char* val) {
				m_type = json::TYPE::STRING;
				m_val = std::string(val);
				m_hash = hash_type<std::string>();
		}

		template<typename T>
		void operator+=(const T& val) {
			append(val);
		}

		template<typename T>
		void operator+=(const char* val) {
			append(std::string(val));
		}

		void clear() {
			if (m_type == json::TYPE::ARRAY) {
				(std::any_cast<arr_t&>(m_val)).clear();
			}
			if (m_type == json::TYPE::DIRCTORY) {
				(std::any_cast<dic_t&>(m_val)).clear();
			}
		}

		template<typename T>
		T as(const T& defaults = T{}) {
			if constexpr (std::is_same_v<T, std::string>) {
				if (m_type != json::TYPE::STRING) return defaults;
				return std::any_cast<T>(m_val);
			}
			else if constexpr (std::is_same_v<T, bool>) {
				if (m_type != json::TYPE::BOOLEAN) return defaults;
				return std::any_cast<T>(m_val);
			}
			else if constexpr (std::is_arithmetic_v<T>) {
				if (
					m_type != json::TYPE::INTEGER && 
					m_type != json::TYPE::FLOAT &&
					m_type != json::TYPE::DOUBLE
				) return defaults;
				if (m_hash == hash_type<float>()) {
					return static_cast<T>(std::any_cast<float>(m_val));
				}
				if (m_hash == hash_type<double>()) {
					return static_cast<T>(std::any_cast<double>(m_val));
				}
				if (m_hash == hash_type<int>()) {
					return static_cast<T>(std::any_cast<int>(m_val));
				}
			}
			return defaults;
		}

		void append(const value& val) {
			if (m_type == json::TYPE::ARRAY) {
				auto& vec = std::any_cast<std::vector<json::value>&>(m_val);
				vec.push_back(val);
			}
		}

		value& operator[](const std::string& key) {
			if (m_type == json::TYPE::DIRCTORY) {
				auto& map = std::any_cast<dic_t&>(m_val);
				if (map.find(key) == map.end()) 
					map.emplace(key, value(json::TYPE::INVALID));
				return map[key];
			}
			return *this;
		}

		value& operator[](const size_t index) {
			if (m_type == json::TYPE::ARRAY) {
				auto& vec = std::any_cast<arr_t&>(m_val);
				if (index < vec.size()) return vec[index];
				return *this;
			}
			return *this;
		}

		bool is_null() { return m_type == json::TYPE::NULLVAL; }

		void set_null() { m_val.reset(); m_type = json::TYPE::NULLVAL; }

		bool undefined() { return m_type == json::TYPE::INVALID; }
		
		json::TYPE type() { return m_type; }

	private:
		std::any m_val;
		
		json::TYPE m_type { json::TYPE::INVALID };

		size_t m_hash { 0 };

	};

}
