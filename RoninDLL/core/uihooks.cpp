#include "core/uihooks.h"

AUTOHOOK_INIT()

ON_DLL_LOAD_CLIENT("ui(11).dll", UIHooks, (CModule module))
{
	AUTOHOOK_DISPATCH()

	module.Offset(0x6AA50).Offset(0x7A).Patch("12");
	module.Offset(0x6AA50).Offset(0x8C).Patch("12");

	module.Offset(0x6AA50).Offset(0x12F).Patch("90 90");
}
