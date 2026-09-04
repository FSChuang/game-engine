#include "Core.h"

// Translation-unit anchor so the Engine static library has something to compile
// while systems are still being built out. Remove once real .cpp files exist.
namespace Engine
{
	namespace Detail
	{
		const int LinkAnchor = 0;
	}
}
