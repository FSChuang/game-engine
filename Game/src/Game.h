#pragma once

#include "Engine/Entity/Entity.h"
#include "Engine/Physics/PhysicsSystem.h"

namespace Engine
{
	class InputManager;
	class Renderer;
}

// Minimal playable vertical slice for Spare Parts (拼裝求生): a static platform, a
// controllable player affected by gravity, and a patrolling enemy. All game-specific state and
// rules live here, never in Engine/ (ENGINEERING_SPEC.md §0, §5).
class Game
{
public:
	Game();

	void Update(Engine::InputManager& input, float deltaTime);
	void Render(Engine::Renderer& renderer);

private:
	void UpdatePlayerMovement(Engine::InputManager& input);
	void UpdateEnemyPatrol(float deltaTime);
	void ResolvePlatformCollision();
	void ResolveEnemyCollision();

	Engine::Entity m_Platform;
	Engine::Entity m_Player;
	Engine::Entity m_Enemy;
	Engine::PhysicsSystem m_Physics;
	float m_EnemyPatrolDirection;
};
