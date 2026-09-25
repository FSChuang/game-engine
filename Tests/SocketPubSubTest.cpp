// Focused, single-process, deterministic tests for Engine::Socket's PUB/SUB roles
// (Milestone 2 Section 5 engine checkpoint). Unlike the PING/PONG REQ/REP smoke test
// (NetworkSmoke/SmokeServer.cpp + SmokeClient.cpp), a PUB and a SUB can both live in
// one process/thread with no request/reply coordination needed between them, so this
// is CTest-registered rather than manually invoked.
//
// PUB/SUB has a well-known "slow joiner" behavior: a freshly-connected SUB's
// subscription handshake takes a small, variable amount of time, and any message a PUB
// sends before that completes is silently dropped for that subscriber (no retroactive
// delivery). This is handled here with a bounded retry loop that keeps publishing while
// polling TryReceive() — never a single Send() followed by an assumption of delivery,
// and never an arbitrary long sleep as the correctness mechanism.

#include "Engine/Network/Socket.h"

#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>

namespace
{
	constexpr const char* PubSubEndpoint = "tcp://127.0.0.1:5570";
	constexpr const char* UnusedEndpoint = "tcp://127.0.0.1:5571"; // nothing ever publishes here
	constexpr int MaxAttempts = 200;
	constexpr int RetryIntervalMs = 5;

	int g_Failures = 0;

	void Check(bool condition, const char* name)
	{
		if (condition)
		{
			std::printf("[PASS] %s\n", name);
		}
		else
		{
			std::printf("[FAIL] %s\n", name);
			++g_Failures;
		}
	}
}

int main()
{
	// A. PUB/SUB basic delivery, tolerant of the slow-joiner window.
	{
		Engine::Socket pub(Engine::SocketRole::Publish);
		pub.Bind(PubSubEndpoint);

		Engine::Socket sub(Engine::SocketRole::Subscribe);
		sub.Connect(PubSubEndpoint);
		sub.Subscribe(""); // empty topic: subscribe to everything this publisher sends

		std::optional<std::string> received;
		for (int attempt = 0; attempt < MaxAttempts && !received.has_value(); ++attempt)
		{
			// Keep publishing every attempt — any individual send may be lost to the
			// slow-joiner window, but once the subscription handshake completes, the
			// next one gets through.
			pub.Send("PING");
			received = sub.TryReceive();
			if (!received.has_value())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(RetryIntervalMs));
			}
		}

		Check(received.has_value() && *received == "PING",
		      "PubSub_BasicDelivery_TryReceiveEventuallyObtainsMessage");
	}

	// B. TryReceive with nothing available returns std::nullopt promptly — never
	// blocks, never spins internally (one non-blocking attempt per call).
	{
		Engine::Socket sub(Engine::SocketRole::Subscribe);
		sub.Connect(UnusedEndpoint);
		sub.Subscribe("");

		auto start = std::chrono::steady_clock::now();
		std::optional<std::string> received = sub.TryReceive();
		auto elapsed = std::chrono::steady_clock::now() - start;

		Check(!received.has_value(), "PubSub_TryReceive_NoMessageAvailable_ReturnsNullopt");
		Check(elapsed < std::chrono::milliseconds(50), "PubSub_TryReceive_NoMessageAvailable_ReturnsPromptly");
	}

	// D. Disconnect is valid and leaves the Socket in a usable state. Deliberately not
	// asserting anything about message delivery around the disconnect itself (that
	// would be a flaky, timing-dependent assertion) — only that the call succeeds
	// without throwing/crashing, and that a subsequent TryReceive() still works.
	{
		Engine::Socket sub(Engine::SocketRole::Subscribe);
		sub.Connect(PubSubEndpoint);
		sub.Subscribe("");
		sub.Disconnect(PubSubEndpoint);

		std::optional<std::string> received = sub.TryReceive();
		Check(!received.has_value(), "PubSub_Disconnect_SocketRemainsUsableAfterward");
	}

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
