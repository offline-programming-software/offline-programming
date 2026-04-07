#pragma once
#include <QString>

class Client {
public:
	Client() : isConnected(false), isLocal(false) {};
	~Client() {};

	// 增加访问属性的公共接口
	QString name() const { return m_name; }
	QString ip() const { return m_ip; }
	QString port() const { return m_port; }
	bool getIsConnected() const { return isConnected; }
	bool getIsLocal() const { return isLocal; }

	// 增加设置属性的公共接口
	void setName(const QString& name) { m_name = name; }
	void setIp(const QString& ip) { m_ip = ip; }
	void setPort(const QString& port) { m_port = port; }
	void setIsConnected(bool connected) { isConnected = connected; }
	void setIsLocal(bool local) { isLocal = local; }

	void connect();

private:
	QString m_name;
	QString m_ip;
	QString m_port;
	bool isConnected;
	bool isLocal;
};