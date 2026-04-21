#ifndef PARSEJSON_H
#define PARSEJSON_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class parseJSON {
public:
	// 构造函数：加载JSON文件
	explicit parseJSON(const std::string& filePath);

	// 根据多个键值对查询对象（AND条件）
	std::vector<json> findObjectsByMultipleKeys(const std::map<std::string, json>& keyValuePairs) const;

	// 根据多个键值对查询对象（OR条件）
	std::vector<json> findObjectsByMultipleKeysOr(const std::map<std::string, json>& keyValuePairs) const;

	// 根据复合条件查询（使用自定义谓词函数）
	template<typename Predicate>
	std::vector<json> findObjectsByCondition(const Predicate& pred) const;

	// 根据多个键名获取所有匹配的对象（不关心值，只关心键是否存在）
	std::vector<json> findObjectsByKeys(const std::vector<std::string>& keys) const;

	// 组合查询：先按键值对过滤，再应用额外条件
	template<typename Predicate>
	std::vector<json> findObjectsByMultipleKeysWithCondition(
		const std::map<std::string, json>& keyValuePairs,
		const Predicate& additionalCondition) const;

	// 检查对象是否包含所有指定键
	bool hasAllKeys(const json& obj, const std::vector<std::string>& keys) const;

	// 检查对象是否包含任意指定键
	bool hasAnyKey(const json& obj, const std::vector<std::string>& keys) const;

	// 获取原始JSON对象（只读）
	const json& getJson() const { return m_jsonData; }

	// 获取指定键的所有值（深度遍历）
	template<typename T>
	std::vector<T> getAllValuesForKey(const std::string& key) const;

private:
	json m_jsonData;

	// 递归查找所有匹配键的值
	template<typename T>
	void findAllValuesForKey(const json& j, const std::string& key, std::vector<T>& results) const;

	// 递归查找所有包含指定键的对象
	void findAllObjectsWithKeys(const json& j, const std::vector<std::string>& keys, std::vector<json>& results) const;
};

// 模板函数实现（必须在头文件中）
template<typename T>
std::vector<T> parseJSON::getAllValuesForKey(const std::string& key) const {
	std::vector<T> results;
	findAllValuesForKey<T>(m_jsonData, key, results);
	return results;
}

template<typename T>
void parseJSON::findAllValuesForKey(const json& j, const std::string& key, std::vector<T>& results) const {
	if (j.is_object()) {
		for (auto it = j.begin(); it != j.end(); ++it) {
			if (it.key() == key) {
				try {
					results.push_back(it.value().get<T>());
				}
				catch (const std::exception&) {
					// 跳过类型不匹配的值
				}
			}
			if (it.value().is_structured()) {
				findAllValuesForKey<T>(it.value(), key, results);
			}
		}
	}
	else if (j.is_array()) {
		for (const auto& element : j) {
			if (element.is_structured()) {
				findAllValuesForKey<T>(element, key, results);
			}
		}
	}
}

template<typename Predicate>
std::vector<json> parseJSON::findObjectsByCondition(const Predicate& pred) const {
	std::vector<json> results;

	if (m_jsonData.is_array()) {
		for (const auto& item : m_jsonData) {
			if (item.is_object() && pred(item)) {
				results.push_back(item);
			}
		}
	}
	else if (m_jsonData.is_object()) {
		if (pred(m_jsonData)) {
			results.push_back(m_jsonData);
		}
	}

	return results;
}

template<typename Predicate>
std::vector<json> parseJSON::findObjectsByMultipleKeysWithCondition(
	const std::map<std::string, json>& keyValuePairs,
	const Predicate& additionalCondition) const {

	// 首先根据键值对筛选
	std::vector<json> candidates = findObjectsByMultipleKeys(keyValuePairs);

	// 然后应用额外条件
	std::vector<json> results;
	for (const auto& obj : candidates) {
		if (additionalCondition(obj)) {
			results.push_back(obj);
		}
	}

	return results;
}

#endif // parseJSON_H

