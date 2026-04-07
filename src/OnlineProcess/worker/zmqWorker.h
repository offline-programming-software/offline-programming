#pragma once
#include <QObject>
#include <zmq.hpp>
#include <memory>
#include <QString>

class ZmqWorker : public QObject
{
    Q_OBJECT
public:
    explicit ZmqWorker(zmq::context_t& ctx);
    ~ZmqWorker();

public slots:

	void on_connectToServer(const QString& ip, const QString& port);
 
signals:
    void connectResult(bool isSuccess);

private:
    zmq::context_t& m_zmqContext;
    bool m_running;
};
