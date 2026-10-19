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

	// C. SetHighWaterMark: a burst of messages sent faster than the subscriber drains
	// them, with the DEFAULT high-water-mark (1000), loses some once the queue fills —
	// demonstrating the real problem a high-rate PUB/SUB exchange (e.g. the CSC 581
	// performance benchmark) hits without this. Raising the high-water-mark past the
	// burst size on BOTH ends (set before Bind()/Connect(), matching the documented
	// contract) delivers every message instead.
	{
		constexpr const char* DefaultHwmEndpoint = "tcp://127.0.0.1:5572";
		constexpr const char* RaisedHwmEndpoint = "tcp://127.0.0.1:5573";
		constexpr int BurstSize = 5000; // well above the library default of 1000

		// Keeps draining until `targetCount` matching ("MSG") messages are seen or
		// `timeoutMs` elapses, rather than a single fixed sleep-then-drain —
		// merely-delayed-but-not-dropped messages (the raised-high-water-mark case) still
		// need time to actually arrive over the loopback connection; a dropped message
		// (the default case) never arrives no matter how long this waits, so the count
		// simply plateaus below target instead. Filters by exact content (only "MSG"
		// counts) rather than assuming every stray leftover "WARMUP" from the handshake
		// above has already arrived by the time draining starts — a straggler warmup
		// message arriving mid-burst would otherwise inflate the count by one.
		auto drainWithRetry = [](Engine::Socket& sub, int targetCount, int timeoutMs) -> int
		{
			int count = 0;
			int elapsedMs = 0;
			while (elapsedMs < timeoutMs)
			{
				std::optional<std::string> message;
				while ((message = sub.TryReceive()).has_value())
				{
					if (*message == "MSG")
					{
						++count;
					}
				}
				if (count >= targetCount)
				{
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(RetryIntervalMs));
				elapsedMs += RetryIntervalMs;
			}
			return count;
		};

		Engine::Socket defaultPub(Engine::SocketRole::Publish);
		defaultPub.Bind(DefaultHwmEndpoint);
		Engine::Socket defaultSub(Engine::SocketRole::Subscribe);
		defaultSub.Connect(DefaultHwmEndpoint);
		defaultSub.Subscribe("");

		// Slow-joiner settle, same bounded-retry idea as test A, but here we need the
		// subscription fully established BEFORE the burst (a message lost to slow-joiner
		// would be indistinguishable from one lost to the high-water-mark).
		std::optional<std::string> warmup;
		for (int attempt = 0; attempt < MaxAttempts && !warmup.has_value(); ++attempt)
		{
			defaultPub.Send("WARMUP");
			warmup = defaultSub.TryReceive();
			if (!warmup.has_value())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(RetryIntervalMs));
			}
		}

		for (int i = 0; i < BurstSize; ++i)
		{
			defaultPub.Send("MSG");
		}
		int defaultReceivedCount = drainWithRetry(defaultSub, BurstSize, 500);
		Check(defaultReceivedCount < BurstSize,
		      "DefaultHighWaterMark_BurstFasterThanDrain_LosesSomeMessages");

		Engine::Socket raisedPub(Engine::SocketRole::Publish);
		raisedPub.SetHighWaterMark(BurstSize * 2);
		raisedPub.Bind(RaisedHwmEndpoint);
		Engine::Socket raisedSub(Engine::SocketRole::Subscribe);
		raisedSub.SetHighWaterMark(BurstSize * 2);
		raisedSub.Connect(RaisedHwmEndpoint);
		raisedSub.Subscribe("");

		std::optional<std::string> raisedWarmup;
		for (int attempt = 0; attempt < MaxAttempts && !raisedWarmup.has_value(); ++attempt)
		{
			raisedPub.Send("WARMUP");
			raisedWarmup = raisedSub.TryReceive();
			if (!raisedWarmup.has_value())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(RetryIntervalMs));
			}
		}

		for (int i = 0; i < BurstSize; ++i)
		{
			raisedPub.Send("MSG");
		}
		int raisedReceivedCount = drainWithRetry(raisedSub, BurstSize, 3000);
		Check(raisedReceivedCount == BurstSize,
		      "RaisedHighWaterMark_SameBurst_DeliversEveryMessage");
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
