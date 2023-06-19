#include "dllmain.h"
#include "logging/logging.h"
#include "logging/crashhandler.h"
#include "core/memalloc.h"
#include "util/version.h"
#include "core/bindingshooks.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include <string.h>
#include <filesystem>

HMODULE _module;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	_module = hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

DWORD WINAPI Thread(HMODULE hModule)
{
	Sleep(7000);

	while (true)
	{
		Sleep(7000);

		findBinds();
	}
	return 0;
}

bool InitialiseRonin()
{
	static bool bInitialised = false;
	if (bInitialised)
		return false;

	bInitialised = true;

	//InitialiseRoninPrefix();

	// initialise the console if needed (-ronin needs this)
	InitialiseConsole();
	// initialise logging before most other things so that they can use spdlog and it have the proper formatting
	InitialiseLogging();
	InitialiseVersion();
	InitializeTF2Binds();

	CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Thread, _module, 0, nullptr));
	//CreateLogFiles();

	InitialiseCrashHandler();

	// Write launcher version to log
	spdlog::info("RoninLauncher version: {}", version);
	spdlog::info("Command line: {}", GetCommandLineA());
	//spdlog::info("Using profile: {}", GetRoninPrefix());

	InstallInitialHooks();

	//g_pServerPresence = new ServerPresenceManager();

	//g_pGameStatePresence = new GameStatePresence();
	//g_pPluginManager = new PluginManager();
	//g_pPluginManager->LoadPlugins();

	//InitialiseSquirrelManagers();

	// Fix some users' failure to connect to respawn datacenters
	//SetEnvironmentVariableA("OPENSSL_ia32cap", "~0x200000200000000");

	curl_global_init_mem(CURL_GLOBAL_DEFAULT, _malloc_base, _free_base, _realloc_base, _strdup_base, _calloc_base);

	// run callbacks for any libraries that are already loaded by now
	CallAllPendingDLLLoadCallbacks();

	return true;
}
