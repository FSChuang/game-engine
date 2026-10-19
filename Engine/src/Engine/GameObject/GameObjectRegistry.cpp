#include "Engine/GameObject/GameObjectRegistry.h"

namespace Engine
{
	GameObject& GameObjectRegistry::Create(ObjectType type, Vector2 position, Vector2 size)
	{
		ObjectId id = m_NextId++;
		auto [it, inserted] = m_Objects.emplace(id, GameObject(id, type, position, size));
		return it->second;
	}

	bool GameObjectRegistry::Remove(ObjectId id)
	{
		return m_Objects.erase(id) > 0;
	}

	bool GameObjectRegistry::RemoveByNetworkId(std::uint32_t networkId)
	{
		GameObject* object = FindByNetworkId(networkId);
		if (object == nullptr)
		{
			return false;
		}

		return Remove(object->GetId());
	}

	GameObject* GameObjectRegistry::Find(ObjectId id)
	{
		auto it = m_Objects.find(id);
		return it == m_Objects.end() ? nullptr : &it->second;
	}

	const GameObject* GameObjectRegistry::Find(ObjectId id) const
	{
		auto it = m_Objects.find(id);
		return it == m_Objects.end() ? nullptr : &it->second;
	}

	GameObject* GameObjectRegistry::FindByNetworkId(std::uint32_t networkId)
	{
		for (auto& [id, object] : m_Objects)
		{
			if (object.HasNetworkId() && object.GetNetworkId() == networkId)
			{
				return &object;
			}
		}
		return nullptr;
	}

	const GameObject* GameObjectRegistry::FindByNetworkId(std::uint32_t networkId) const
	{
		for (const auto& [id, object] : m_Objects)
		{
			if (object.HasNetworkId() && object.GetNetworkId() == networkId)
			{
				return &object;
			}
		}
		return nullptr;
	}

	std::size_t GameObjectRegistry::Count() const
	{
		return m_Objects.size();
	}

	std::unordered_map<ObjectId, GameObject>::iterator GameObjectRegistry::begin()
	{
		return m_Objects.begin();
	}

	std::unordered_map<ObjectId, GameObject>::iterator GameObjectRegistry::end()
	{
		return m_Objects.end();
	}

	std::unordered_map<ObjectId, GameObject>::const_iterator GameObjectRegistry::begin() const
	{
		return m_Objects.begin();
	}

	std::unordered_map<ObjectId, GameObject>::const_iterator GameObjectRegistry::end() const
	{
		return m_Objects.end();
	}
}
