
#pragma once
#include<qstring.h>

class Device
{
public:
	// 设备类型枚举
	enum class DeviceType {
		Robot,      // 机械臂
		AGV,        // 自动导引车
		Measure     // 测.试/测量设备
	};

	// 构造函数
	Device(QString name = "", QString ip = "", QString port = "", DeviceType type = DeviceType::Robot)
		: m_deviceName(name),
		m_IP(ip),
		m_port(port),
		m_deviceType(type) {}

	// 设置函数
	void setPort(const QString &port) { m_port = port; }
	void setIP(const QString &ip) { m_IP = ip; }
	void setDeviceType(DeviceType type) { m_deviceType = type; }
	void setName(const QString &name) { m_deviceName = name; }

	// 获取函数
	QString getPort() const { return m_port; }
	QString getIP() const { return m_IP; }
	QString getName() const { return m_deviceName; }
	DeviceType getDeviceType() const { return m_deviceType; }

	// 设备类型转字符串（方便UI显示）
	QString getDeviceTypeString() const {
		switch (m_deviceType) {
		case DeviceType::Robot: return "机械臂";
		case DeviceType::AGV: return "AGV";
		case DeviceType::Measure: return "测量设备";
		default: return "未知";
		}
	}

private:
	QString m_IP;
	QString m_deviceName;
	QString m_port;
	DeviceType m_deviceType;
};
