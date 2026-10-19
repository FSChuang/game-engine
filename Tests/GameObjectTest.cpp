// Focused unit tests for Engine::GameObject (ENGINEERING_SPEC.md §10). Pure logic, no SDL
// window. Covers identity/Transform (ported from the old EntityTest.cpp) plus the
// Milestone 3 compositional-property behavior: HasRender/HasVelocity/HasNetworkId are
// each independently false until their matching Set* is called.

#include "Engine/GameObject/GameObject.h"

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
	// Arrange
	Engine::Vector2 initialPosition{ 10.0f, 20.0f };
	Engine::Vector2 size{ 32.0f, 64.0f };
	Engine::GameObject object(1, Engine::ObjectType::Generic, initialPosition, size);

	// Assert: identity.
	Check(object.GetId() == 1, "GameObject_Id_MatchesConstructorArgument");
	Check(object.GetType() == Engine::ObjectType::Generic, "GameObject_Type_MatchesConstructorArgument");

	// Assert: required Transform data.
	Check(object.GetPosition().X == 10.0f && object.GetPosition().Y == 20.0f,
	      "GameObject_InitialPosition_MatchesConstructorArgument");
	Check(object.GetSize().X == 32.0f && object.GetSize().Y == 64.0f, "GameObject_Size_MatchesConstructorArgument");

	// Act
	object.SetPosition({ 5.0f, 7.0f });

	// Assert
	Check(object.GetPosition().X == 5.0f && object.GetPosition().Y == 7.0f, "SetPosition_UpdatesPosition");

	// Act
	object.Move({ 3.0f, -2.0f });

	// Assert
	Check(object.GetPosition().X == 8.0f && object.GetPosition().Y == 5.0f, "Move_AddsDeltaToCurrentPosition");

	// Assert
	Check(object.GetSize().X == 32.0f && object.GetSize().Y == 64.0f, "Move_DoesNotChangeSize");

	// Assert: every optional property starts absent — a freshly-constructed object is
	// hidden, static, and unnetworked until told otherwise.
	Check(!object.HasRender(), "GameObject_InitiallyHasNoRenderProperty");
	Check(!object.HasVelocity(), "GameObject_InitiallyHasNoVelocityProperty");
	Check(!object.HasNetworkId(), "GameObject_InitiallyHasNoNetworkIdProperty");

	// Act / Assert: SetRender attaches the render property.
	object.SetRender({ 255, 0, 0, 255 });
	Check(object.HasRender(), "SetRender_AttachesRenderProperty");
	Check(object.GetColor().R == 255 && object.GetColor().G == 0 && object.GetColor().B == 0 &&
	          object.GetColor().A == 255,
	      "SetRender_StoresGivenColor");

	// Act / Assert: SetVelocity attaches the velocity property.
	object.SetVelocity({ 1.0f, 2.0f });
	Check(object.HasVelocity(), "SetVelocity_AttachesVelocityProperty");
	Check(object.GetVelocity().X == 1.0f && object.GetVelocity().Y == 2.0f, "SetVelocity_StoresGivenVelocity");

	// Act / Assert: SetNetworkId attaches the network-identity property.
	object.SetNetworkId(42);
	Check(object.HasNetworkId(), "SetNetworkId_AttachesNetworkIdProperty");
	Check(object.GetNetworkId() == 42, "SetNetworkId_StoresGivenId");

	// Arrange / Act / Assert: a second, otherwise-identical object with no Set* calls
	// never acquires render/velocity/network state as a side effect of the first object's
	// calls above — each GameObject's optional properties are independent.
	Engine::GameObject hiddenObject(2, Engine::ObjectType::SpawnPoint, { 0.0f, 0.0f }, { 1.0f, 1.0f });
	Check(!hiddenObject.HasRender() && !hiddenObject.HasVelocity() && !hiddenObject.HasNetworkId(),
	      "GameObject_PropertiesAreIndependentPerInstance");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
