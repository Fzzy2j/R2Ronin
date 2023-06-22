#include "engine/hoststate.h"
#include "masterserver/masterserver.h"
#include "server/auth/serverauthentication.h"
#include "server/serverpresence.h"
#include "shared/playlist.h"
#include "core/tier0.h"
#include "engine/r2engine.h"
#include "squirrel/squirrel.h"
#include "plugins/plugins.h"
#include "plugins/pluginbackend.h"
#include "ckf/inputhooks.h"

AUTOHOOK_INIT()

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	CHostState* g_pHostState;
} // namespace R2

std::string sLastMode;

VAR_AT(engine.dll + 0x13FA6070, ConVar*, Cvar_hostport);
FUNCTION_AT(engine.dll + 0x1232C0, void, __fastcall, _Cmd_Exec_f, (const CCommand& arg, bool bOnlyIfExists, bool bUseWhitelists));

void ServerStartingOrChangingMap()
{
	ConVar* Cvar_mp_gamemode = g_pCVar->FindVar("mp_gamemode");

	// directly call _Cmd_Exec_f to avoid weirdness with ; being in mp_gamemode potentially
	// if we ran exec {mp_gamemode} and mp_gamemode contained semicolons, this could be used to execute more commands
	char* commandBuf[1040]; // assumedly this is the size of CCommand since we don't have an actual constructor
	memset(commandBuf, 0, sizeof(commandBuf));
	CCommand tempCommand = *(CCommand*)&commandBuf;
	if (sLastMode.length() &&
		CCommand__Tokenize(
			tempCommand, fmt::format("exec server/cleanup_gamemode_{}", sLastMode).c_str(), R2::cmd_source_t::kCommandSrcCode))
		_Cmd_Exec_f(tempCommand, false, false);

	memset(commandBuf, 0, sizeof(commandBuf));
	if (CCommand__Tokenize(
			tempCommand,
			fmt::format("exec server/setup_gamemode_{}", sLastMode = Cvar_mp_gamemode->GetString()).c_str(),
			R2::cmd_source_t::kCommandSrcCode))
	{
		_Cmd_Exec_f(tempCommand, false, false);
	}

	Cbuf_Execute(); // exec everything right now

	// net_data_block_enabled is required for sp, force it if we're on an sp map
	// sucks for security but just how it be
	if (!strncmp(g_pHostState->m_levelName, "sp_", 3))
	{
		g_pCVar->FindVar("net_data_block_enabled")->SetValue(true);
		g_pServerAuthentication->m_bStartingLocalSPGame = true;
	}
	else
		g_pServerAuthentication->m_bStartingLocalSPGame = false;
}

// clang-format off
AUTOHOOK(CHostState__State_NewGame, engine.dll + 0x16E7D0,
void, __fastcall, (CHostState* self))
// clang-format on
{
	spdlog::info("HostState: NewGame");

	Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec autoexec_ns_server", cmd_source_t::kCommandSrcCode);
	Cbuf_Execute();

	// need to do this to ensure we don't go to private match
	if (g_pServerAuthentication->m_bNeedLocalAuthForNewgame)
		SetCurrentPlaylist("tdm");

	ServerStartingOrChangingMap();

	double dStartTime = Tier0::Plat_FloatTime();
	CHostState__State_NewGame(self);
	spdlog::info("loading took {}s", Tier0::Plat_FloatTime() - dStartTime);

	// setup server presence
	g_pServerPresence->CreatePresence();
	g_pServerPresence->SetMap(g_pHostState->m_levelName, true);
	g_pServerPresence->SetPlaylist(GetCurrentPlaylistName());
	g_pServerPresence->SetPort(Cvar_hostport->GetInt());

	g_pServerAuthentication->m_bNeedLocalAuthForNewgame = false;
}

// clang-format off
AUTOHOOK(CHostState__State_LoadGame, engine.dll + 0x16E730,
void, __fastcall, (CHostState* self))
// clang-format on
{
	// singleplayer server starting
	// useless in 99% of cases but without it things could potentially break very much

	spdlog::info("HostState: LoadGame");

	Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec autoexec_ns_server", cmd_source_t::kCommandSrcCode);
	Cbuf_Execute();

	// this is normally done in ServerStartingOrChangingMap(), but seemingly the map name isn't set at this point
	g_pCVar->FindVar("net_data_block_enabled")->SetValue(true);
	g_pServerAuthentication->m_bStartingLocalSPGame = true;

	double dStartTime = Tier0::Plat_FloatTime();
	CHostState__State_LoadGame(self);
	spdlog::info("loading took {}s", Tier0::Plat_FloatTime() - dStartTime);

	// no server presence, can't do it because no map name in hoststate
	// and also not super important for sp saves really

	g_pServerAuthentication->m_bNeedLocalAuthForNewgame = false;
}

// clang-format off
AUTOHOOK(CHostState__State_ChangeLevelMP, engine.dll + 0x16E520,
void, __fastcall, (CHostState* self))
// clang-format on
{
	spdlog::info("HostState: ChangeLevelMP");

	ServerStartingOrChangingMap();

	double dStartTime = Tier0::Plat_FloatTime();
	CHostState__State_ChangeLevelMP(self);
	spdlog::info("loading took {}s", Tier0::Plat_FloatTime() - dStartTime);

	g_pServerPresence->SetMap(g_pHostState->m_levelName);
}

// clang-format off
AUTOHOOK(CHostState__State_GameShutdown, engine.dll + 0x16E640,
void, __fastcall, (CHostState* self))
// clang-format on
{
	spdlog::info("HostState: GameShutdown");

	g_pServerPresence->DestroyPresence();

	CHostState__State_GameShutdown(self);

	// run gamemode cleanup cfg now instead of when we start next map
	if (sLastMode.length())
	{
		char* commandBuf[1040]; // assumedly this is the size of CCommand since we don't have an actual constructor
		memset(commandBuf, 0, sizeof(commandBuf));
		CCommand tempCommand = *(CCommand*)&commandBuf;
		if (CCommand__Tokenize(
				tempCommand, fmt::format("exec server/cleanup_gamemode_{}", sLastMode).c_str(), R2::cmd_source_t::kCommandSrcCode))
		{
			_Cmd_Exec_f(tempCommand, false, false);
			Cbuf_Execute();
		}

		sLastMode.clear();
	}
}

// clang-format off
AUTOHOOK(CHostState__FrameUpdate, engine.dll + 0x16DB00,
void, __fastcall, (CHostState* self, double flCurrentTime, float flFrameTime))
// clang-format on
{
	CHostState__FrameUpdate(self, flCurrentTime, flFrameTime);

	try
	{
		framesSinceJump++;

		if (superglideCrouchFrame == 0)
		{
			hookedPostEvent(
				crouchPressHolder.a,
				crouchPressHolder.nType,
				crouchPressHolder.nTick,
				crouchPressHolder.scanCode,
				crouchPressHolder.virtualCode,
				crouchPressHolder.data3);
		}
		if (superglideCrouchFrame >= 0)
			superglideCrouchFrame--;

		// if (SRMM_GetSetting(SRMM_TAS_MODE)) TASFrameHook();

		if (jumpPressHolder.waitingToSend)
		{
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);
			long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
			long sinceJump = real - jumpPressHolder.timestamp;
			long sinceCrouch = real - crouchPressHolder.timestamp;

			if (sinceJump > CROUCHKICK_BUFFERING)
			{
				jumpPressHolder.waitingToSend = false;
				hookedPostEvent(
					jumpPressHolder.a,
					jumpPressHolder.nType,
					jumpPressHolder.nTick,
					jumpPressHolder.scanCode,
					jumpPressHolder.virtualCode,
					jumpPressHolder.data3);

				long long e = sinceCrouch - sinceJump;
				framesSinceJump = 0;

				if (e < 100)
				{
					NS::log::FZZY->info(("not crouchkick: " + std::to_string(e) + "ms JUMP IS EARLY").c_str());
				}
			}
		}
		if (jumpReleaseHolder.waitingToSend)
		{
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);
			long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
			long sinceJump = real - jumpReleaseHolder.timestamp;

			if (sinceJump > CROUCHKICK_BUFFERING)
			{
				jumpReleaseHolder.waitingToSend = false;
				hookedPostEvent(
					jumpReleaseHolder.a,
					jumpReleaseHolder.nType,
					jumpReleaseHolder.nTick,
					jumpReleaseHolder.scanCode,
					jumpReleaseHolder.virtualCode,
					jumpReleaseHolder.data3);
			}
		}

		if (crouchPressHolder.waitingToSend)
		{
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);
			long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
			long sinceCrouch = real - crouchPressHolder.timestamp;
			long sinceJump = real - jumpPressHolder.timestamp;

			if (sinceCrouch > CROUCHKICK_BUFFERING)
			{
				crouchPressHolder.waitingToSend = false;
				hookedPostEvent(
					crouchPressHolder.a,
					crouchPressHolder.nType,
					crouchPressHolder.nTick,
					crouchPressHolder.scanCode,
					crouchPressHolder.virtualCode,
					crouchPressHolder.data3);

				long long e = sinceJump - sinceCrouch;

				if (e < 100)
				{
					NS::log::FZZY->info(("not crouchkick: " + std::to_string(e) + "ms CROUCH IS EARLY").c_str());
				}
			}
		}
		if (crouchReleaseHolder.waitingToSend)
		{
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);
			long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
			long sinceCrouch = real - crouchReleaseHolder.timestamp;

			if (sinceCrouch > CROUCHKICK_BUFFERING)
			{
				crouchReleaseHolder.waitingToSend = false;
				hookedPostEvent(
					crouchReleaseHolder.a,
					crouchReleaseHolder.nType,
					crouchReleaseHolder.nTick,
					crouchReleaseHolder.scanCode,
					crouchReleaseHolder.virtualCode,
					crouchReleaseHolder.data3);
			}
		}
	}
	catch (...)
	{
	}

	if (*R2::g_pServerState == R2::server_state_t::ss_active)
	{
		// update server presence
		g_pServerPresence->RunFrame(flCurrentTime);
	}

	// Run Squirrel message buffer
	if (g_pSquirrel<ScriptContext::UI>->m_pSQVM != nullptr && g_pSquirrel<ScriptContext::UI>->m_pSQVM->sqvm != nullptr)
		g_pSquirrel<ScriptContext::UI>->ProcessMessageBuffer();

	if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM != nullptr && g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM->sqvm != nullptr)
		g_pSquirrel<ScriptContext::CLIENT>->ProcessMessageBuffer();

	if (g_pSquirrel<ScriptContext::SERVER>->m_pSQVM != nullptr && g_pSquirrel<ScriptContext::SERVER>->m_pSQVM->sqvm != nullptr)
		g_pSquirrel<ScriptContext::SERVER>->ProcessMessageBuffer();

	g_pGameStatePresence->RunFrame();
}

ON_DLL_LOAD_RELIESON("engine.dll", HostState, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH()

	g_pHostState = module.Offset(0x7CF180).As<CHostState*>();
}
