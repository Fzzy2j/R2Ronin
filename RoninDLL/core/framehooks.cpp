#include "framehooks.h"
#include "core/inputhooks.h"
#include <string>

AUTOHOOK_INIT()

AUTOHOOK(CHostState__FrameUpdate, engine.dll + 0x16DB00, void, __fastcall, (CHostState * self, double flCurrentTime, float flFrameTime))
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

		if (SRMM_GetSetting(SRMM_CK_FIX))
		{
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
	}
	catch (...)
	{
	}
}

ON_DLL_LOAD_RELIESON("engine.dll", HostState, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH()
}
