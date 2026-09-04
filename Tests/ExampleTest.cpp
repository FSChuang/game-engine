// Minimal example test demonstrating the F.I.R.S.T. pattern (ENGINEERING_SPEC.md §10).
// It only exercises Core plumbing so it's a template, not a graded engine system.
// Replace/extend with real tests for physics, collision, input, scaling.

#include "Engine/Core/Core.h"

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
	// Arrange / Act
	Engine::Scope<int> value = Engine::CreateScope<int>(42);

	// Assert (one logical assertion per test)
	Check(value != nullptr, "CreateScope_ReturnsNonNull");
	Check(*value == 42, "CreateScope_HoldsGivenValue");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
