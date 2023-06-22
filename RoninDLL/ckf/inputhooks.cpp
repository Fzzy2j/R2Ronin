#include "ckf/inputhooks.h"
#include "ckf/bindingshooks.h"
#include "logging/sourceconsole.h"
#include <string>

InputHolder jumpPressHolder;
InputHolder jumpReleaseHolder;
InputHolder crouchPressHolder;
InputHolder crouchReleaseHolder;

POSTEVENT hookedPostEvent = nullptr;

int framesSinceJump = 0;
int superglideCrouchFrame = -1;

AUTOHOOK_INIT()

AUTOHOOK(
	PostEvent,
	inputsystem.dll + 0x7EC0,
	unsigned int,
	__fastcall,
	(uintptr_t a, InputEventType_t nType, int nTick, ButtonCode_t scanCode, ButtonCode_t virtualCode, int data3))
{
	hookedPostEvent = PostEvent;
	try
	{
		ButtonCode_t key = scanCode;
		// if (SRMM_GetSetting(SRMM_TAS_MODE) && TASProcessInput(a, nType, nTick, scanCode, virtualCode, data3))
		// return 0;
		if (nType == IE_ButtonPressed)
		{
			if ((key == jumpBinds[0] || key == jumpBinds[1]) && !jumpPressHolder.waitingToSend)
			{
				struct timespec ts;
				timespec_get(&ts, TIME_UTC);
				long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
				long sinceCrouch = real - crouchPressHolder.timestamp;
				if (crouchPressHolder.waitingToSend && sinceCrouch <= CROUCHKICK_BUFFERING)
				{
					crouchPressHolder.waitingToSend = false;
					// auto crouchElapsed = std::chrono::steady_clock::now() - crouchPressHolder.timestamp;
					// long long sinceCrouch = std::chrono::duration_cast<std::chrono::milliseconds>(crouchElapsed).count();
					// m_sourceConsole->Print(("crouchkick: " + to_string(sinceCrouch) + "ms CROUCH IS EARLY\n").c_str());
					NS::log::FZZY->info(("crouchkick: " + std::to_string(sinceCrouch) + "ms CROUCH IS EARLY").c_str());

					// playSound();
					PostEvent(
						crouchPressHolder.a,
						crouchPressHolder.nType,
						crouchPressHolder.nTick,
						crouchPressHolder.scanCode,
						crouchPressHolder.virtualCode,
						crouchPressHolder.data3);
				}
				else
				{
					jumpPressHolder.a = a;
					jumpPressHolder.nType = nType;
					jumpPressHolder.nTick = nTick;
					jumpPressHolder.scanCode = scanCode;
					jumpPressHolder.virtualCode = virtualCode;
					jumpPressHolder.data3 = data3;

					jumpPressHolder.waitingToSend = true;
					struct timespec ts;
					timespec_get(&ts, TIME_UTC);
					long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
					jumpPressHolder.timestamp = real;

					return 0;
				}
			}
			if ((key == crouchBinds[0] || key == crouchBinds[1]) && !crouchPressHolder.waitingToSend)
			{
				struct timespec ts;
				timespec_get(&ts, TIME_UTC);
				long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
				long sinceJump = real - jumpPressHolder.timestamp;
				if ((jumpPressHolder.waitingToSend && sinceJump > CROUCHKICK_BUFFERING) || framesSinceJump == 0)
				{
					if (framesSinceJump == 0)
						superglideCrouchFrame = 0;
					else
						superglideCrouchFrame = 1;
					// m_sourceConsole->Print(("superglide: " + to_string(framesSinceJump) + "\n").c_str());
					NS::log::FZZY->info(("superglide: " + std::to_string(framesSinceJump)).c_str());
					return 0;
				}
				if (jumpPressHolder.waitingToSend && sinceJump <= CROUCHKICK_BUFFERING)
				{
					jumpPressHolder.waitingToSend = false;

					// auto jumpElapsed = std::chrono::steady_clock::now() - jumpPressHolder.timestamp;
					// long long sinceJump = std::chrono::duration_cast<std::chrono::milliseconds>(jumpElapsed).count();
					// m_sourceConsole->Print(("crouchkick: " + to_string(sinceJump) + "ms JUMP IS EARLY\n").c_str());
					NS::log::FZZY->info(("crouchkick: " + std::to_string(sinceJump) + "ms JUMP IS EARLY").c_str());

					// playSound();
					PostEvent(
						jumpPressHolder.a,
						jumpPressHolder.nType,
						jumpPressHolder.nTick,
						jumpPressHolder.scanCode,
						jumpPressHolder.virtualCode,
						jumpPressHolder.data3);
				}
				else
				{
					crouchPressHolder.a = a;
					crouchPressHolder.nType = nType;
					crouchPressHolder.nTick = nTick;
					crouchPressHolder.scanCode = scanCode;
					crouchPressHolder.virtualCode = virtualCode;
					crouchPressHolder.data3 = data3;

					crouchPressHolder.waitingToSend = true;
					struct timespec ts;
					timespec_get(&ts, TIME_UTC);
					long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
					crouchPressHolder.timestamp = real;

					return 0;
				}
			}
		}
		if (nType == IE_ButtonReleased)
		{
			if ((key == jumpBinds[0] || key == jumpBinds[1]) && !jumpReleaseHolder.waitingToSend)
			{
				jumpReleaseHolder.a = a;
				jumpReleaseHolder.nType = nType;
				jumpReleaseHolder.nTick = nTick;
				jumpReleaseHolder.scanCode = scanCode;
				jumpReleaseHolder.virtualCode = virtualCode;
				jumpReleaseHolder.data3 = data3;

				jumpReleaseHolder.waitingToSend = true;
				struct timespec ts;
				timespec_get(&ts, TIME_UTC);
				long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
				jumpReleaseHolder.timestamp = real;

				return 0;
			}
			if ((key == crouchBinds[0] || key == crouchBinds[1]) && !crouchReleaseHolder.waitingToSend)
			{
				crouchReleaseHolder.a = a;
				crouchReleaseHolder.nType = nType;
				crouchReleaseHolder.nTick = nTick;
				crouchReleaseHolder.scanCode = scanCode;
				crouchReleaseHolder.virtualCode = virtualCode;
				crouchReleaseHolder.data3 = data3;

				crouchReleaseHolder.waitingToSend = true;
				struct timespec ts;
				timespec_get(&ts, TIME_UTC);
				long long real = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);
				crouchReleaseHolder.timestamp = real;

				return 0;
			}
		}
	}
	catch (...)
	{
	}
	uintptr_t moduleBase = (uintptr_t)GetModuleHandleW(L"inputsystem.dll");
	unsigned int toReturn = PostEvent(moduleBase + 0x69F40, nType, nTick, scanCode, virtualCode, data3);
	return toReturn;
}

ON_DLL_LOAD_CLIENT("inputsystem.dll", InputHooks, (CModule module))
{
	AUTOHOOK_DISPATCH()
}
