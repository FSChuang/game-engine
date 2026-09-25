#pragma once

#include <zmq.hpp>

#include <string>

namespace Engine
{
	// Which half of ZeroMQ's synchronous REQ/REP pattern a Socket plays. ROUTER/DEALER
	// are deliberately out of scope for this transport (Milestone 2 Section 4 forbids them).
	enum class SocketRole
	{
		Request,  // ZMQ_REQ: connects to a peer, sends a request, blocks for its reply.
		Reply     // ZMQ_REP: binds an endpoint, blocks for a request, then sends a reply.
	};

	// Thin RAII wrapper over one ZeroMQ context + socket, restricted to the synchronous
	// REQ/REP pattern (ENGINEERING_SPEC.md §6: RAII for every resource). Owns both for
	// their entire lifetime; destruction closes the socket and terminates the context.
	//
	// Every operation throws zmq::error_t on failure — cppzmq does this itself, so
	// failures are never silently swallowed here.
	//
	// Not thread-safe: a single Socket must be used from one thread only. This matches
	// cppzmq's own contract (a zmq::context_t may be shared across threads; a
	// zmq::socket_t may not) and is a deliberate limitation for this first transport
	// slice, not a guarantee this type makes for any future multithreaded use.
	class Socket
	{
	public:
		explicit Socket(SocketRole role);

		Socket(const Socket&) = delete;
		Socket& operator=(const Socket&) = delete;

		// Reply role only: starts listening for connections at `endpoint`
		// (e.g. "tcp://127.0.0.1:5555").
		void Bind(const std::string& endpoint);

		// Request role only: connects to a peer already bound at `endpoint`
		// (e.g. "tcp://127.0.0.1:5555").
		void Connect(const std::string& endpoint);

		// Sends `message` as a single ZeroMQ frame.
		void Send(const std::string& message);

		// Blocks until a single ZeroMQ frame arrives, returned as a string.
		std::string Receive();

	private:
		SocketRole m_Role;
		zmq::context_t m_Context;
		zmq::socket_t m_Socket;
	};
}
