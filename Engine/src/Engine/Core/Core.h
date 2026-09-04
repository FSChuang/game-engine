#pragma once

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <utility>

// Foundational, game-agnostic plumbing shared across the whole engine.
// This file holds conventions from ENGINEERING_SPEC.md (§6 ownership, §8 assert),
// NOT any gameplay or engine-system logic.

namespace Engine
{
	// --- Ownership aliases (ENGINEERING_SPEC.md §6) ---
	// Use Scope<T> for unique ownership, Ref<T> for shared ownership.
	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T, typename... Args>
	constexpr Ref<T> CreateRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	// --- Basic 2D spatial value (ENGINEERING_SPEC.md §9: value objects over bare primitives) ---
	// Used for both position and size. Floating-point to match SDL3's float-based rendering APIs.
	struct Vector2
	{
		float X;
		float Y;
	};
}

// --- Cross-platform assertion (ENGINEERING_SPEC.md §8) ---
// Portable on purpose: no MSVC-only __debugbreak(). Compiles out in release.
#ifdef NDEBUG
	#define ENGINE_ASSERT(condition, message) ((void)0)
#else
	#define ENGINE_ASSERT(condition, message)                                          \
		do                                                                             \
		{                                                                              \
			if (!(condition))                                                          \
			{                                                                          \
				std::fprintf(stderr, "Assertion failed: %s\n  at %s:%d\n", (message),  \
				             __FILE__, __LINE__);                                       \
				std::abort();                                                          \
			}                                                                          \
		} while (false)
#endif
