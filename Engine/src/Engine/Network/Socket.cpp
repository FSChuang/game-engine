#include "Engine/Network/Socket.h"

#include "Engine/Core/Core.h"

#include <stdexcept>

namespace Engine
{
	namespace
	{
		zmq::socket_type ToZmqSocketType(SocketRole role)
		{
			return role == SocketRole::Request ? zmq::socket_type::req : zmq::socket_type::rep;
		}
	}

	Socket::Socket(SocketRole role)
		: m_Role(role), m_Context(1), m_Socket(m_Context, ToZmqSocketType(role))
	{
	}

	void Socket::Bind(const std::string& endpoint)
	{
		ENGINE_ASSERT(m_Role == SocketRole::Reply, "Socket::Bind is only valid for SocketRole::Reply");
		m_Socket.bind(endpoint);
	}

	void Socket::Connect(const std::string& endpoint)
	{
		ENGINE_ASSERT(m_Role == SocketRole::Request, "Socket::Connect is only valid for SocketRole::Request");
		m_Socket.connect(endpoint);
	}

	void Socket::Send(const std::string& message)
	{
		m_Socket.send(zmq::buffer(message), zmq::send_flags::none);
	}

	std::string Socket::Receive()
	{
		zmq::message_t message;
		if (!m_Socket.recv(message, zmq::recv_flags::none))
		{
			throw std::runtime_error("Socket::Receive: no message received");
		}
		return std::string(static_cast<const char*>(message.data()), message.size());
	}
}
