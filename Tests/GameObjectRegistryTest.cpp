// Focused unit tests for Engine::GameObjectRegistry (Milestone 3). Pure logic, no SDL window.

#include "Engine/GameObject/GameObjectRegistry.h"

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

int main()
{
	Engine::GameObjectRegistry registry;

	// Act / Assert: Create assigns distinct, never-reused ids.
	Engine::GameObject& first = registry.Create(Engine::ObjectType::Platform, { 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::GameObject& second = registry.Create(Engine::ObjectType::Player, { 1.0f, 1.0f }, { 5.0f, 5.0f });
	Check(first.GetId() != second.GetId(), "Create_AssignsDistinctIds");
	Check(registry.Count() == 2, "Create_IncreasesCount");

	// Act / Assert: Find resolves a live id and reflects live state (same underlying object).
	Engine::ObjectId firstId = first.GetId();
	first.SetPosition({ 99.0f, 99.0f });
	Engine::GameObject* found = registry.Find(firstId);
	Check(found != nullptr && found->GetPosition().X == 99.0f, "Find_ReturnsTheSameLiveObject");

	// Assert: Find returns nullptr for an id that was never assigned.
	Check(registry.Find(9999) == nullptr, "Find_UnknownId_ReturnsNullptr");

	// Act / Assert: network-id lookup resolves by network identity, not registry id.
	second.SetNetworkId(7);
	Engine::GameObject* byNetworkId = registry.FindByNetworkId(7);
	Check(byNetworkId != nullptr && byNetworkId->GetId() == second.GetId(), "FindByNetworkId_ResolvesMatchingObject");
	Check(registry.FindByNetworkId(8) == nullptr, "FindByNetworkId_UnknownNetworkId_ReturnsNullptr");

	// Act / Assert: two distinct network ids never collide with one another.
	Engine::GameObject& third = registry.Create(Engine::ObjectType::Player, { 2.0f, 2.0f }, { 5.0f, 5.0f });
	third.SetNetworkId(8);
	Check(registry.FindByNetworkId(7)->GetId() == second.GetId() && registry.FindByNetworkId(8)->GetId() == third.GetId(),
	      "FindByNetworkId_DistinctNetworkIds_DoNotOverwriteOneAnother");

	// Act / Assert: Remove deletes exactly the requested object.
	Check(registry.Remove(firstId), "Remove_ExistingId_ReturnsTrue");
	Check(registry.Find(firstId) == nullptr, "Remove_ExistingId_ObjectNoLongerFound");
	Check(registry.Count() == 2, "Remove_DecreasesCount");
	Check(!registry.Remove(firstId), "Remove_AlreadyRemovedId_ReturnsFalse");

	// Act / Assert: RemoveByNetworkId resolves and deletes by network identity.
	Check(registry.RemoveByNetworkId(7), "RemoveByNetworkId_ExistingNetworkId_ReturnsTrue");
	Check(registry.FindByNetworkId(7) == nullptr, "RemoveByNetworkId_ObjectNoLongerFound");
	Check(!registry.RemoveByNetworkId(7), "RemoveByNetworkId_AlreadyRemoved_ReturnsFalse");

	// Assert: the untouched third object survives every operation above.
	Check(registry.FindByNetworkId(8) != nullptr, "RemoveByNetworkId_DoesNotAffectOtherObjects");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
