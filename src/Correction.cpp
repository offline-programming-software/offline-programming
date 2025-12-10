#include "Correction.h"

Correction::Correction() = default;
Correction::~Correction() = default;

void to_json(nlohmann::json& j, const Correction& c)
{
	j = {
		{"name", c.m_name.toStdString()},
		{"interType", static_cast<int>(c.m_interType)},
		{"isPosCorrect", c.m_isPosCorrect},
		{"range", c.m_range},
		{"flagPoints", c.m_flagPoints},
		{"measurePoints", c.m_measurePoints},
		{"isApply", c.m_isApply}
	};
}

void from_json(const nlohmann::json& j, Correction& c)
{
	c.m_name = QString::fromStdString(j.at("name").get<std::string>());
	c.m_interType = static_cast<Correction::interpolationType>(j.at("interType").get<int>());
	c.m_isPosCorrect = j.at("isPosCorrect").get<bool>();
	c.m_range = j.at("range").get<std::array<double, 6>>();
	c.m_flagPoints = j.at("flagPoints").get<std::vector<double>>();
	c.m_measurePoints = j.at("measurePoints").get<std::vector<double>>();
	c.m_isApply = j.at("isApply").get<bool>();
}
