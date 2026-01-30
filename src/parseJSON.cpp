#include "parseJSON.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

parseJSON::parseJSON(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("Cannot open JSON file: " + filePath);
	}

	try {
		file >> m_jsonData;
	}
	catch (const json::parse_error& e) {
		throw std::runtime_error("Invalid JSON format in file: " + filePath + " - " + std::string(e.what()));
	}
}

std::vector<json> parseJSON::findObjectsByMultipleKeys(
	const std::map<std::string, json>& keyValuePairs) const {

	std::vector<json> results;

	if (m_jsonData.is_array()) {
		for (const auto& item : m_jsonData) {
			if (item.is_object()) {
				bool matches = true;

				for (const auto& pair : keyValuePairs) {
					const std::string& key = pair.first;
					const json& expectedValue = pair.second;

					if (!item.contains(key) || item[key] != expectedValue) {
						matches = false;
						break;
					}
				}

				if (matches) {
					results.push_back(item);
				}
			}
		}
	}
	else if (m_jsonData.is_object()) {
		bool matches = true;

		for (const auto& pair : keyValuePairs) {
			const std::string& key = pair.first;
			const json& expectedValue = pair.second;

			if (!m_jsonData.contains(key) || m_jsonData[key] != expectedValue) {
				matches = false;
				break;
			}
		}

		if (matches) {
			results.push_back(m_jsonData);
		}
	}

	return results;
}

std::vector<json> parseJSON::findObjectsByMultipleKeysOr(
	const std::map<std::string, json>& keyValuePairs) const {

	std::vector<json> results;

	if (m_jsonData.is_array()) {
		for (const auto& item : m_jsonData) {
			if (item.is_object()) {
				bool matches = false;

				for (const auto& pair : keyValuePairs) {
					const std::string& key = pair.first;
					const json& expectedValue = pair.second;

					if (item.contains(key) && item[key] == expectedValue) {
						matches = true;
						break;
					}
				}

				if (matches) {
					results.push_back(item);
				}
			}
		}
	}
	else if (m_jsonData.is_object()) {
		bool matches = false;

		for (const auto& pair : keyValuePairs) {
			const std::string& key = pair.first;
			const json& expectedValue = pair.second;

			if (m_jsonData.contains(key) && m_jsonData[key] == expectedValue) {
				matches = true;
				break;
			}
		}

		if (matches) {
			results.push_back(m_jsonData);
		}
	}

	return results;
}

std::vector<json> parseJSON::findObjectsByKeys(
	const std::vector<std::string>& keys) const {

	std::vector<json> results;
	findAllObjectsWithKeys(m_jsonData, keys, results);
	return results;
}

void parseJSON::findAllObjectsWithKeys(
	const json& j, const std::vector<std::string>& keys, std::vector<json>& results) const {

	if (j.is_object()) {
		if (hasAllKeys(j, keys)) {
			results.push_back(j);
		}

		for (auto it = j.begin(); it != j.end(); ++it) {
			if (it.value().is_structured()) {
				findAllObjectsWithKeys(it.value(), keys, results);
			}
		}
	}
	else if (j.is_array()) {
		for (const auto& element : j) {
			if (element.is_structured()) {
				findAllObjectsWithKeys(element, keys, results);
			}
		}
	}
}

bool parseJSON::hasAllKeys(const json& obj, const std::vector<std::string>& keys) const {
	if (!obj.is_object()) {
		return false;
	}

	for (const auto& key : keys) {
		if (!obj.contains(key)) {
			return false;
		}
	}
	return true;
}

bool parseJSON::hasAnyKey(const json& obj, const std::vector<std::string>& keys) const {
	if (!obj.is_object()) {
		return false;
	}

	for (const auto& key : keys) {
		if (obj.contains(key)) {
			return true;
		}
	}
	return false;
}