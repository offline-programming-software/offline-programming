#include <QDebug>
#include "zmqWorker.h"

ZmqWorker::ZmqWorker(QObject* parent)
    : QObject(parent), m_running(false)
{
}

ZmqWorker::~ZmqWorker()
{
    stopServer();
}

void ZmqWorker::startServer(const QString& address)
{
    try {
        if (!m_zmqContext) {
            m_zmqContext = std::make_unique<zmq::context_t>(1);
        }
        if (!m_zmqRepSocket) {
            m_zmqRepSocket = std::make_unique<zmq::socket_t>(*m_zmqContext, ZMQ_REP);
            // 可以设置超时时间，防止 recv 一直死锁导致线程无法安全退出
            int timeout = 1000; // 1000 ms
            m_zmqRepSocket->set(zmq::sockopt::rcvtimeo, timeout);
        }

        m_zmqRepSocket->bind(address.toStdString());
        m_running = true;
        emit serverStarted(true, address);

        // 阻塞监听循环
        while (m_running) {
            zmq::message_t request;
            try {
                // 使用 recv(请求) 如果超时会返回 nullopt (zmq.hpp 新版行为) 或抛出 EAGAIN
                if (m_zmqRepSocket->recv(request, zmq::recv_flags::none)) {
                    QString msg = QString::fromStdString(std::string(static_cast<char*>(request.data()), request.size()));
                    emit messageReceived(msg);

                    // 必须回应，否则 REP socket 无法继续接收下一次
                    std::string replyStr = "OK";
                    zmq::message_t reply(replyStr.size());
                    memcpy(reply.data(), replyStr.c_str(), replyStr.size());
                    m_zmqRepSocket->send(reply, zmq::send_flags::none);
                }
            }
            catch (const zmq::error_t& e) {
                if (e.num() != EAGAIN) { // EAGAIN 说明是超时，不是出错
                    qDebug() << "Receive error:" << e.what();
                }
            }
        }
    }
    catch (const zmq::error_t& e) {
        emit serverStarted(false, address, QString(e.what()));
    }
}

void ZmqWorker::stopServer()
{
    m_running = false;
    if (m_zmqRepSocket) {
        m_zmqRepSocket->close();
        m_zmqRepSocket.reset();
    }
    if (m_zmqContext) {
        m_zmqContext->close();
        m_zmqContext.reset();
    }
}