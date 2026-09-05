#include "Game.h"

#include "Engine/Collision/Collision.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Renderer/Renderer.h"

#include <SDL3/SDL_scancode.h>

namespace
{
	// Gameplay tunables (ENGINEERING_SPEC.md §9: configurable data stays at the top level,
	// never buried as unexplained magic numbers).
	constexpr float Gravity = 980.0f;             // px/s^2 downward acceleration
	constexpr float PlayerSpeed = 300.0f;         // px/s horizontal movement speed

	constexpr Engine::Vector2 PlayerSpawnPosition{ 700.0f, 200.0f };
	constexpr Engine::Vector2 PlayerSize{ 50.0f, 50.0f };
	constexpr Engine::Color PlayerColor{ 0, 128, 255, 255 };

	constexpr Engine::Vector2 PlatformPosition{ 400.0f, 900.0f };
	constexpr Engine::Vector2 PlatformSize{ 1200.0f, 40.0f };
	constexpr Engine::Color PlatformColor{ 120, 120, 120, 255 };

	constexpr float EnemyPatrolY = 850.0f;
	constexpr float EnemyPatrolLeftBound = 450.0f;
	constexpr float EnemyPatrolRightBound = 1500.0f;
	constexpr float EnemyPatrolSpeed = 150.0f;    // px/s
	constexpr Engine::Vector2 EnemySize{ 50.0f, 50.0f };
	constexpr Engine::Color EnemyColor{ 200, 40, 40, 255 };
}

Game::Game()
	: m_Platform(PlatformPosition, PlatformSize, PlatformColor),
	  m_Player(PlayerSpawnPosition, PlayerSize, PlayerColor),
	  m_Enemy({ EnemyPatrolLeftBound, EnemyPatrolY }, EnemySize, EnemyColor),
	  m_Physics(Gravity),
	  m_EnemyPatrolDirection(1.0f)
{
}

void Game::Update(Engine::InputManager& input, float deltaTime)
{
	UpdatePlayerMovement(input);
	m_Physics.Update(m_Player, deltaTime);
	ResolvePlatformCollision();

	UpdateEnemyPatrol(deltaTime);
	ResolveEnemyCollision();
}

void Game::Render(Engine::Renderer& renderer)
{
	renderer.DrawEntity(m_Platform);
	renderer.DrawEntity(m_Player);
	renderer.DrawEntity(m_Enemy);
}

void Game::UpdatePlayerMovement(Engine::InputManager& input)
{
	Engine::Vector2 velocity = m_Player.GetVelocity();

	bool movingLeft = input.IsKeyPressed(SDL_SCANCODE_A) || input.IsKeyPressed(SDL_SCANCODE_LEFT);
	bool movingRight = input.IsKeyPressed(SDL_SCANCODE_D) || input.IsKeyPressed(SDL_SCANCODE_RIGHT);

	if (movingLeft && !movingRight)
	{
		velocity.X = -PlayerSpeed;
	}
	else if (movingRight && !movingLeft)
	{
		velocity.X = PlayerSpeed;
	}
	else
	{
		velocity.X = 0.0f;
	}

	m_Player.SetVelocity(velocity);
}

void Game::UpdateEnemyPatrol(float deltaTime)
{
	Engine::Vector2 velocity{ m_EnemyPatrolDirection * EnemyPatrolSpeed, 0.0f };
	m_Enemy.SetVelocity(velocity);
	m_Enemy.Move({ velocity.X * deltaTime, 0.0f });

	float enemyX = m_Enemy.GetPosition().X;
	if (enemyX <= EnemyPatrolLeftBound)
	{
		m_EnemyPatrolDirection = 1.0f;
	}
	else if (enemyX >= EnemyPatrolRightBound)
	{
		m_EnemyPatrolDirection = -1.0f;
	}
}

void Game::ResolvePlatformCollision()
{
	if (!Engine::IsColliding(m_Player, m_Platform))
	{
		return;
	}

	Engine::Vector2 velocity = m_Player.GetVelocity();
	if (velocity.Y <= 0.0f)
	{
		return;
	}

	Engine::Vector2 position = m_Player.GetPosition();
	position.Y = m_Platform.GetPosition().Y - m_Player.GetSize().Y;
	m_Player.SetPosition(position);

	velocity.Y = 0.0f;
	m_Player.SetVelocity(velocity);
}

void Game::ResolveEnemyCollision()
{
	if (!Engine::IsColliding(m_Player, m_Enemy))
	{
		return;
	}

	m_Player.SetPosition(PlayerSpawnPosition);
	m_Player.SetVelocity({ 0.0f, 0.0f });
}
