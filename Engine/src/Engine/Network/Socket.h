#pragma once

#include <zmq.hpp>

#include <optional>
#include <string>

namespace Engine
{
	/// Which role a Socket plays. Request/Reply are ZeroMQ's synchronous REQ/REP pattern
	/// (Milestone 2 Section 2-4). Publish/Subscribe are ZeroMQ's PUB/SUB pattern
	/// (Milestone 2 Section 5: peer-to-peer player-state broadcast) — fire-and-forget,
	/// no request/reply semantics, one publisher fanning out to many subscribers.
	/// ROUTER/DEALER are deliberately out of scope for this transport (forbidden by the
	/// assignment for both uses).
	enum class SocketRole
	{
		Request,    ///< ZMQ_REQ: connects to a peer, sends a request, blocks for its reply.
		Reply,      ///< ZMQ_REP: binds an endpoint, blocks for a request, then sends a reply.
		Publish,    ///< ZMQ_PUB: binds an endpoint, broadcasts messages to every connected subscriber.
		Subscribe   ///< ZMQ_SUB: connects to one or more publishers, receives whatever they broadcast.
	};

	// Thin RAII wrapper over one ZeroMQ context + socket (ENGINEERING_SPEC.md §6: RAII
	// for every resource). Owns both for their entire lifetime; destruction closes the
	// socket and terminates the context. Deliberately curated to the two synchronous
	// REQ/REP roles plus the two PUB/SUB roles — never ROUTER/DEALER.
	//
	// Every operation throws zmq::error_t on genuine ZeroMQ failure — cppzmq does this
	// itself, so failures are never silently swallowed here. The one exception is
	// Receive()'s own defensive "no message received" check, which throws
	// std::runtime_error instead, since that specific path isn't a ZeroMQ-reported
	// error. TryReceive() is the one deliberate non-throwing case: it returns
	// std::nullopt for the expected "no message available yet" outcome, and still
	// throws for any other error.
	//
	// Not thread-safe: a single Socket must be used from one thread only. This matches
	// cppzmq's own contract (a zmq::context_t may be shared across threads; a
	// zmq::socket_t may not) and is a deliberate limitation for this transport, not a
	// guarantee this type makes for any future multithreaded use.
	class Socket
	{
	public:
		/// Constructs a ZeroMQ context and a socket of the type matching `role`, together.
		explicit Socket(SocketRole role);

		Socket(const Socket&) = delete;
		Socket& operator=(const Socket&) = delete;

		/// Reply or Publish roles only: starts listening for connections at `endpoint`
		/// (e.g. "tcp://127.0.0.1:5555" for a concrete address, or "tcp://*:5555" to
		/// bind every interface).
		void Bind(const std::string& endpoint);

		/// Request or Subscribe roles only: connects to a peer already bound at
		/// `endpoint` (e.g. "tcp://127.0.0.1:5555"). A Subscribe socket may call this
		/// repeatedly over its lifetime to connect to multiple publishers — unlike
		/// Request, there is no "connected once" assumption for Subscribe.
		void Connect(const std::string& endpoint);

		/// Subscribe role only: severs one specific connection previously established
		/// via Connect(), leaving any other connections on this same socket untouched.
		/// Also valid for Request, for symmetry with Connect() — this does not change
		/// Request's own send/receive discipline, which remains exactly as before.
		void Disconnect(const std::string& endpoint);

		/// Subscribe role only: registers interest in messages whose payload starts with
		/// `topic`. An empty topic subscribes to everything a connected publisher sends
		/// — the only form this project uses; no topic-prefix scheme is introduced. A
		/// freshly-constructed Subscribe socket receives nothing until this is called at
		/// least once.
		void Subscribe(const std::string& topic);

		/// Sends `message` as a single ZeroMQ frame. Valid for Request/Reply (as part of
		/// their alternating discipline) and for Publish (fire-and-forget broadcast to
		/// every currently-connected subscriber).
		void Send(const std::string& message);

		/// Blocks until a single ZeroMQ frame arrives, returned as a string. No timeout.
		std::string Receive();

		/// Performs one non-blocking receive attempt (ZMQ_DONTWAIT). Returns the message
		/// if one was already fully available, or std::nullopt if none was available
		/// right now — this is the expected, non-error outcome of calling this when
		/// nothing has arrived yet, so it is reported as std::nullopt rather than an
		/// exception. A genuine ZeroMQ error still throws zmq::error_t, exactly like
		/// every other method on this class. Never blocks, never spins internally:
		/// exactly one non-blocking attempt per call. Intended primarily for Subscribe,
		/// but is not role-restricted since the underlying operation is identical for
		/// any receiving role.
		std::optional<std::string> TryReceive();

	private:
		SocketRole m_Role;
		zmq::context_t m_Context;
		zmq::socket_t m_Socket;
	};
}
