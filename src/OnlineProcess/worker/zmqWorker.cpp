#include "OnlineProcess/worker/zmqWorker.h"

ZmqWorker::ZmqWorker(zmq::context_t& ctx)
	:m_zmqContext(ctx)
{
}

ZmqWorker::~ZmqWorker()
{
}

void ZmqWorker::on_connectToServer(const QString& ip, const QString& port)
{
	zmq::socket_t m_socket(m_zmqContext, zmq::socket_type::req);
	if (ip == "...")
	{
		m_socket.connect(QString("tcp://localhost:%1").arg(port).toStdString());
	}
	else
	{
	m_socket.connect(QString("tcp://%1:%2").arg(ip).arg(port).toStdString());
	}
	std::string ping = "HELLO";
	m_socket.send(zmq::buffer(ping), zmq::send_flags::none);

	// 3. 等待响应 (建议设置超时，防止服务器没开导致 UI 线程/Worker 永久挂起)
	// 设置 3000ms 超时
	m_socket.set(zmq::sockopt::rcvtimeo, 3000);
	// 3. 等待等待服务端响应
	zmq::message_t reply;
	auto res = m_socket.recv(reply, zmq::recv_flags::none);

	if (res.has_value()) {
		// 收到了回复，说明双向通信建立成功
		// std::string replyStr = reply.to_string(); // C++14 可转为字符串
		emit connectResult(true);
	}
	else {
		// 接收超时，说明服务端没响应
		emit connectResult(false);
	}
}



