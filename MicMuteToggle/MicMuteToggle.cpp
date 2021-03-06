// MicMuteToggle.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "windows.h"
#include "mmdeviceapi.h"
#include "mmsystem.h"
#include "endpointvolume.h"
#include "MicMuteToggle.h"

// Configuration

namespace {
	const auto MUTE_KEY = VK_F15;
	const auto UNMUTE_KEY = VK_F16;
	const auto PTT_PTM_MOUSE_BUTTON = XBUTTON2;
	const unsigned int PTT_RELEASE_DELAY_MS = 250;
};

// Global Variables:
HINSTANCE hInst;
HANDLE hSingleInstanceMutex;
HHOOK hMouseHook;
HHOOK hKeyboardHook;
IAudioEndpointVolume* micVolume;
UINT_PTR muteTimer = 0;

// Forward declarations of functions included in this code module:
BOOL                InitInstance(HINSTANCE, int);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MICMUTETOGGLE));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	ReleaseMutex(hSingleInstanceMutex);

	return (int)msg.wParam;
}

enum class MuteBehavior {
	TOGGLE = 0,
	MUTE = 1,
	UNMUTE = 2,
};

void SetMute(MuteBehavior SetTo, bool IsButtonUp) {
	IMMDeviceEnumerator* de;
	CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&de
	);

	IMMDevice* micDevicePtr;
	de->GetDefaultAudioEndpoint(EDataFlow::eCapture, ERole::eCommunications, &micDevicePtr);

	micDevicePtr->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&micVolume);
	BOOL wasMuted;
	micVolume->GetMute(&wasMuted);
	if (wasMuted && SetTo == MuteBehavior::MUTE) {
		return;
	}
	if (!wasMuted && SetTo == MuteBehavior::UNMUTE) {
		return;
	}

	const auto muteWav = MAKEINTRESOURCE(IDR_MUTE);
	const auto unmuteWav = MAKEINTRESOURCE(IDR_UNMUTE);
	const auto feedbackWav = wasMuted ? unmuteWav : muteWav;

	if (!wasMuted) {
		if (PTT_RELEASE_DELAY_MS > 0 && IsButtonUp) {
			// Push to talk released. Everyone releases the button once they've started the last syllable,
			// but that's not understandable if you actually cut the microphone there. Wait a little longer.
			muteTimer = SetTimer(
				nullptr,
				0,
				PTT_RELEASE_DELAY_MS,
				[](HWND hWnd, UINT, UINT_PTR idTimer, DWORD) {
				muteTimer = 0;
				KillTimer(hWnd, idTimer);
				micVolume->SetMute(TRUE, nullptr);
				PlaySound(MAKEINTRESOURCE(IDR_MUTE), hInst, SND_ASYNC | SND_RESOURCE);
			}
			);
			return;
		}
		micVolume->SetMute(TRUE, nullptr);
	}
	else {
		micVolume->SetMute(FALSE, nullptr);
	}
	PlaySound(feedbackWav, hInst, SND_ASYNC | SND_RESOURCE);
}

LRESULT CALLBACK GlobalMouseHook(
	int code,
	WPARAM wParam,
	LPARAM lParam
) {
	if (code != 0 || lParam == 0 || (wParam != WM_XBUTTONUP && wParam != WM_XBUTTONDOWN)) {
		return CallNextHookEx(0, code, wParam, lParam);
	}

	MSLLHOOKSTRUCT* event = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
	if ((event->mouseData >> 16) != PTT_PTM_MOUSE_BUTTON) {
		return CallNextHookEx(0, code, wParam, lParam);
	}

	if (muteTimer != 0) {
		// PTT stays alive for a little while after the button is released, as people generally
		// release it at the start of the syllable, so it clips.
		//
		// If we have an active timer, then the user released PTT, but hit it again before the timer
		// was hit, so we need to unmute, not re-mute
		KillTimer(nullptr, muteTimer);
		muteTimer = 0;
		return S_FALSE;
	}
	SetMute(MuteBehavior::TOGGLE, wParam == WM_XBUTTONUP);

	return S_FALSE;
}

LRESULT CALLBACK GlobalKeyboardHook(
	int code,
	WPARAM wParam,
	LPARAM lParam
) {
	if (code != 0 || lParam == 0 || (wParam != WM_KEYDOWN && wParam != WM_KEYUP)) {
		return CallNextHookEx(0, code, wParam, lParam);
	}

	KBDLLHOOKSTRUCT* event = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
	if (event->vkCode == MUTE_KEY) {
		if (wParam == WM_KEYDOWN) {
			SetMute(MuteBehavior::MUTE, /* button up = */ false);
		}
		return S_FALSE;
	}
	if (event->vkCode == UNMUTE_KEY) {
		if (wParam == WM_KEYDOWN) {
			SetMute(MuteBehavior::UNMUTE, /* button up = */ false);
		}
		return S_FALSE;
	}

	return CallNextHookEx(0, code, wParam, lParam);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	CoInitialize(NULL); // Initalize COM

	hSingleInstanceMutex = CreateMutex(nullptr, FALSE, L"Global\\com.fredemmott.micmutetoggle");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return FALSE;
	}

	hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, GlobalMouseHook, nullptr, 0); // Hook low-level mouse events, e.g. button presses
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, GlobalKeyboardHook, nullptr, 0); // Same for keyboard
	return TRUE;
}
