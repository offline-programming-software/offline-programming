#pragma once
#include <QObject>
#include <zmq.hpp>
#include <memory>
#include <QString>

class ZmqWorker : public QObject
{
    Q_OBJECT
public:
    explicit ZmqWorker(QObject* parent = nullptr);
    ~ZmqWorker();

public slots:
    void startServer(const QString& address);
    void stopServer();

signals:
    void messageReceived(const QString& msg);
    void serverStarted(bool success, const QString& addr, const QString& errorMsg = "");

private:
    std::unique_ptr<zmq::context_t> m_zmqContext;
    std::unique_ptr<zmq::socket_t> m_zmqRepSocket;
    bool m_running;
};
