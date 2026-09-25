#include "Engine/Network/Socket.h"

#include "Engine/Core/Core.h"

#include <stdexcept>

namespace Engine
{
	namespace
	{
		zmq::socket_type ToZmqSocketType(SocketRole role)
		{
			switch (role)
			{
				case SocketRole::Request:
					return zmq::socket_type::req;
				case SocketRole::Reply:
					return zmq::socket_type::rep;
				case SocketRole::Publish:
					return zmq::socket_type::pub;
				case SocketRole::Subscribe:
					return zmq::socket_type::sub;
			}
			// Unreachable: every SocketRole enumerator is handled above. Kept as an
			// explicit fallback (rather than a default: case) so adding a future
			// SocketRole without updating this switch is a compiler warning, not a
			// silent fallthrough.
			return zmq::socket_type::req;
		}
	}

	Socket::Socket(SocketRole role)
		: m_Role(role), m_Context(1), m_Socket(m_Context, ToZmqSocketType(role))
	{
	}

	void Socket::Bind(const std::string& endpoint)
	{
		ENGINE_ASSERT(m_Role == SocketRole::Reply || m_Role == SocketRole::Publish,
		              "Socket::Bind is only valid for SocketRole::Reply or SocketRole::Publish");
		m_Socket.bind(endpoint);
	}

	void Socket::Connect(const std::string& endpoint)
	{
		ENGINE_ASSERT(m_Role == SocketRole::Request || m_Role == SocketRole::Subscribe,
		              "Socket::Connect is only valid for SocketRole::Request or SocketRole::Subscribe");
		m_Socket.connect(endpoint);
	}

	void Socket::Disconnect(const std::string& endpoint)
	{
		m_Socket.disconnect(endpoint);
	}

	void Socket::Subscribe(const std::string& topic)
	{
		ENGINE_ASSERT(m_Role == SocketRole::Subscribe, "Socket::Subscribe is only valid for SocketRole::Subscribe");
		m_Socket.set(zmq::sockopt::subscribe, topic);
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

	std::optional<std::string> Socket::TryReceive()
	{
		zmq::message_t message;
		// cppzmq's recv() returns an empty result on EAGAIN (the expected "nothing
		// available yet" outcome under ZMQ_DONTWAIT) and throws zmq::error_t for any
		// other failure — exactly the std::nullopt-vs-throw split this method promises.
		if (!m_Socket.recv(message, zmq::recv_flags::dontwait))
		{
			return std::nullopt;
		}
		return std::string(static_cast<const char*>(message.data()), message.size());
	}
}
