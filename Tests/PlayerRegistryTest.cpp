// Focused unit tests for Engine::PlayerRegistry (ENGINEERING_SPEC.md §10). Pure logic,
// no sockets, no SDL, no threads.

#include "Engine/Network/PlayerRegistry.h"

#include <cstdio>

static int g_Failures = 0;

static void Check(bool condition, const char* name)
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

static bool SnapshotContainsId(const std::vector<Engine::PlayerState>& snapshot, Engine::PlayerId id)
{
	for (const Engine::PlayerState& state : snapshot)
	{
		if (state.Id == id)
		{
			return true;
		}
	}
	return false;
}

int main()
{
	// Arrange / Act / Assert: a fresh registry starts empty.
	{
		Engine::PlayerRegistry registry;
		Check(registry.PlayerCount() == 0, "PlayerRegistry_Fresh_StartsWithZeroCount");
		Check(registry.Snapshot().empty(), "PlayerRegistry_Fresh_SnapshotIsEmpty");
	}

	// Act / Assert: the first assigned ID is valid and, given the documented starting
	// counter, is exactly 1.
	{
		Engine::PlayerRegistry registry;
		std::optional<Engine::PlayerId> firstId = registry.AssignPlayer();
		Check(firstId.has_value(), "AssignPlayer_First_Succeeds");
		Check(firstId.has_value() && *firstId == 1, "AssignPlayer_First_IsOne");
	}

	// Act / Assert: three players can join, and assigned IDs are unique.
	{
		Engine::PlayerRegistry registry;
		std::optional<Engine::PlayerId> a = registry.AssignPlayer();
		std::optional<Engine::PlayerId> b = registry.AssignPlayer();
		std::optional<Engine::PlayerId> c = registry.AssignPlayer();

		Check(a.has_value() && b.has_value() && c.has_value(), "AssignPlayer_ThreePlayers_AllSucceed");
		Check(*a != *b && *b != *c && *a != *c, "AssignPlayer_ThreePlayers_AllUnique");
		Check(registry.PlayerCount() == 3, "AssignPlayer_ThreePlayers_CountIsThree");
	}

	// Act / Assert: updating a known player's state is reflected in the snapshot.
	{
		Engine::PlayerRegistry registry;
		Engine::PlayerId id = *registry.AssignPlayer();

		bool updated = registry.UpdatePlayer(Engine::PlayerState{ id, 10.0f, 20.0f, 1.0f, 2.0f });
		Check(updated, "UpdatePlayer_KnownPlayer_ReturnsTrue");

		std::vector<Engine::PlayerState> snapshot = registry.Snapshot();
		bool found = false;
		for (const Engine::PlayerState& state : snapshot)
		{
			if (state.Id == id)
			{
				found = state.PositionX == 10.0f && state.PositionY == 20.0f && state.VelocityX == 1.0f &&
				        state.VelocityY == 2.0f;
			}
		}
		Check(found, "UpdatePlayer_KnownPlayer_UpdatedStateAppearsInSnapshot");
	}

	// Act / Assert: removing a known player drops it from the snapshot and count.
	{
		Engine::PlayerRegistry registry;
		Engine::PlayerId id = *registry.AssignPlayer();
		registry.AssignPlayer(); // a second player stays behind

		bool removed = registry.RemovePlayer(id);
		Check(removed, "RemovePlayer_KnownPlayer_ReturnsTrue");
		Check(!SnapshotContainsId(registry.Snapshot(), id), "RemovePlayer_KnownPlayer_AbsentFromSnapshot");
		Check(registry.PlayerCount() == 1, "RemovePlayer_KnownPlayer_CountDecrements");
	}

	// Act / Assert: player count changes correctly across assign/remove sequence.
	{
		Engine::PlayerRegistry registry;
		Check(registry.PlayerCount() == 0, "PlayerCount_BeforeAnyAssign_IsZero");

		Engine::PlayerId a = *registry.AssignPlayer();
		Check(registry.PlayerCount() == 1, "PlayerCount_AfterOneAssign_IsOne");

		Engine::PlayerId b = *registry.AssignPlayer();
		Check(registry.PlayerCount() == 2, "PlayerCount_AfterTwoAssigns_IsTwo");

		registry.RemovePlayer(a);
		Check(registry.PlayerCount() == 1, "PlayerCount_AfterOneRemoval_IsOne");

		registry.RemovePlayer(b);
		Check(registry.PlayerCount() == 0, "PlayerCount_AfterAllRemoved_IsZero");
	}

	// Act / Assert: MaxPlayers is enforced — the (MaxPlayers + 1)th assignment fails,
	// and freeing a slot allows assignment to succeed again.
	{
		Engine::PlayerRegistry registry;
		for (std::size_t i = 0; i < Engine::MaxPlayers; ++i)
		{
			Check(registry.AssignPlayer().has_value(), "AssignPlayer_UpToMaxPlayers_Succeeds");
		}

		std::optional<Engine::PlayerId> overflow = registry.AssignPlayer();
		Check(!overflow.has_value(), "AssignPlayer_BeyondMaxPlayers_Rejected");

		registry.RemovePlayer(1); // free one slot
		Check(registry.AssignPlayer().has_value(), "AssignPlayer_AfterFreeingSlot_SucceedsAgain");
	}

	// Act / Assert: player IDs are never reused, even after the player is removed.
	{
		Engine::PlayerRegistry registry;
		Engine::PlayerId first = *registry.AssignPlayer();
		registry.RemovePlayer(first);
		Engine::PlayerId second = *registry.AssignPlayer();

		Check(second != first, "AssignPlayer_AfterRemoval_DoesNotReuseId");
	}

	// Act / Assert: updating or removing an unknown player is a documented no-op.
	{
		Engine::PlayerRegistry registry;
		Engine::PlayerId knownId = *registry.AssignPlayer();
		constexpr Engine::PlayerId UnknownId = 9999;

		bool updateResult = registry.UpdatePlayer(Engine::PlayerState{ UnknownId, 1.0f, 1.0f, 1.0f, 1.0f });
		Check(!updateResult, "UpdatePlayer_UnknownPlayer_ReturnsFalse");
		Check(registry.PlayerCount() == 1, "UpdatePlayer_UnknownPlayer_DoesNotInsertNewEntry");

		bool removeResult = registry.RemovePlayer(UnknownId);
		Check(!removeResult, "RemovePlayer_UnknownPlayer_ReturnsFalse");
		Check(registry.PlayerCount() == 1, "RemovePlayer_UnknownPlayer_DoesNotChangeCount");
		Check(SnapshotContainsId(registry.Snapshot(), knownId), "RemovePlayer_UnknownPlayer_KnownPlayerStillPresent");
	}

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
