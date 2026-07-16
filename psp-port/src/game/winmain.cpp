#include "pch.h"
#include "winmain.h"

#include "control.h"
#include "EmbeddedData.h"
#include "fullscrn.h"
#include "midi.h"
#include "options.h"
#include "pb.h"
#include "render.h"
#include "Sound.h"
#include "translations.h"

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include "psp_input.h"

PSP_MODULE_INFO("SpaceCadetPinball", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

constexpr const char* winmain::Version;

SDL_Window* winmain::MainWindow = nullptr;
SDL_Renderer* winmain::Renderer = nullptr;

int winmain::return_value = 0;
bool winmain::bQuit = false;
bool winmain::activated = false;
bool winmain::DispFrameRate = false;
bool winmain::DispGRhistory = false;
bool winmain::single_step = false;
bool winmain::has_focus = true;
int winmain::last_mouse_x;
int winmain::last_mouse_y;
int winmain::mouse_down;
bool winmain::no_time_loss = false;

bool winmain::restart = false;

std::vector<float> winmain::gfrDisplay{};
unsigned winmain::gfrOffset = 0;
float winmain::gfrWindow = 5.0f;
bool winmain::LaunchBallEnabled = true;
bool winmain::HighScoresEnabled = true;
bool winmain::DemoActive = false;
int winmain::MainMenuHeight = 0;
std::string winmain::FpsDetails, winmain::PrevSdlError;
unsigned winmain::PrevSdlErrorCount = 0;
double winmain::UpdateToFrameRatio;
winmain::DurationMs winmain::TargetFrameTime;
optionsStruct& winmain::Options = options::Options;
winmain::DurationMs winmain::SpinThreshold = DurationMs(0.005);
WelfordState winmain::SleepState{};
int winmain::CursorIdleCounter = 0;

static int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

static int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void psp_setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if(thid >= 0)
        sceKernelStartThread(thid, 0, 0);
}

int winmain::WinMain(LPCSTR lpCmdLine)
{
	psp_setup_callbacks();
	PspInput::Init();

	std::set_new_handler(memalloc_failure);

	printf("SpaceCadetPinball PSP v%s\n", Version);

	// SDL init
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
		SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
	{
		pb::ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2", SDL_GetError());
		return 1;
	}

	pb::quickFlag = strstr(lpCmdLine, "-quick") != nullptr;

	// SDL window - PSP 480x272 fullscreen
	SDL_Window* window = SDL_CreateWindow
	(
		pb::get_rc_string(Msg::STRING139),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		480, 272,
		SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN
	);
	MainWindow = window;
	if (!window)
	{
		pb::ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create window", SDL_GetError());
		return 1;
	}

	// Software renderer for PSP
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	Renderer = renderer;
	if (!renderer)
	{
		pb::ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create renderer", SDL_GetError());
		return 1;
	}
	SDL_RendererInfo rendererInfo{};
	if (!SDL_GetRendererInfo(renderer, &rendererInfo))
		printf("Using SDL renderer: %s\n", rendererInfo.name);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

	auto prefPath = SDL_GetPrefPath("", "SpaceCadetPinball");
	auto basePath = SDL_GetBasePath();

	// SDL mixer init
	bool mixOpened = false, noAudio = strstr(lpCmdLine, "-noaudio") != nullptr;
	if (!noAudio)
	{
		if ((Mix_Init(MIX_INIT_MID_Proxy) & MIX_INIT_MID_Proxy) == 0)
		{
			printf("Could not initialize SDL MIDI, music might not work.\nSDL Error: %s\n", SDL_GetError());
			SDL_ClearError();
		}
		if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) != 0)
		{
			printf("Could not open audio device, continuing without audio.\nSDL Error: %s\n", SDL_GetError());
			SDL_ClearError();
		}
		else
			mixOpened = true;
	}

	auto resetAllOptions = strstr(lpCmdLine, "-reset") != nullptr;
	do
	{
		restart = false;

		options::InitPrimary();
		if (resetAllOptions)
		{
			resetAllOptions = false;
			options::ResetAllOptions();
		}
		Options.FullScreen = true;
		Options.ShowMenu = false;
		Options.HideCursor = true;

		// Data search order: WD, executable path, user pref path, platform specific paths.
		std::vector<const char*> searchPaths
		{
			{
				"",
				basePath,
				prefPath
			}
		};
		searchPaths.insert(searchPaths.end(), std::begin(PlatformDataPaths), std::end(PlatformDataPaths));
		pb::SelectDatFile(searchPaths);

		options::InitSecondary();

		Sound::Init(mixOpened, Options.SoundChannels, Options.Sounds, Options.SoundVolume);
		if (!mixOpened)
			Options.Sounds = false;

		if (!midi::music_init(mixOpened, Options.MusicVolume))
			Options.Music = false;

		if (pb::init())
		{
			std::string message = "The .dat file is missing.\n"
				"Make sure that the game data is present in any of the following locations:\n";
			for (auto path : searchPaths)
			{
				if (path)
				{
					message = message + (path[0] ? path : "working directory") + "\n";
				}
			}
			pb::ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Could not load game data", message.c_str());
			return 1;
		}

		fullscrn::init();

		pb::reset_table();
		pb::firsttime_setup();

		SDL_ShowWindow(window);
		fullscrn::set_screen_mode(Options.FullScreen);

		if (strstr(lpCmdLine, "-demo"))
			pb::toggle_demo();
		else
			pb::replay_level(false);

		MainLoop();

		options::uninit();
		midi::music_shutdown();
		Sound::Close();
		pb::uninit();
	}
	while (restart);

	if (!noAudio)
	{
		if (mixOpened)
			Mix_CloseAudio();
		Mix_Quit();
	}

	SDL_free(basePath);
	SDL_free(prefPath);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return return_value;
}

void winmain::MainLoop()
{
	bQuit = false;
	unsigned updateCounter = 0, frameCounter = 0;
	auto frameStart = Clock::now();
	double UpdateToFrameCounter = 0;
	DurationMs sleepRemainder(0), frameDuration(TargetFrameTime);
	auto prevTime = frameStart;

	while (true)
	{
		if (DispFrameRate)
		{
			auto curTime = Clock::now();
			if (curTime - prevTime > DurationMs(1000))
			{
				char buf[60];
				auto elapsedSec = DurationMs(curTime - prevTime).count() * 0.001;
				snprintf(buf, sizeof buf, "UPS=%02.02f FPS=%02.02f",
				         updateCounter / elapsedSec, frameCounter / elapsedSec);
				FpsDetails = buf;
				frameCounter = updateCounter = 0;
				prevTime = curTime;
			}
		}

		// PSP controller input
		{
			SceCtrlData pad;
			sceCtrlReadBufferPositive(&pad, 1);

			static unsigned int prevButtons = 0;
			unsigned int pressed = pad.Buttons & ~prevButtons;
			unsigned int released = ~pad.Buttons & prevButtons;

			// Left Flipper: L-trigger
			if (pressed & PSP_CTRL_LTRIGGER)
				pb::InputDown({InputTypes::GameController, 0});
			if (released & PSP_CTRL_LTRIGGER)
				pb::InputUp({InputTypes::GameController, 0});

			// Right Flipper: R-trigger
			if (pressed & PSP_CTRL_RTRIGGER)
				pb::InputDown({InputTypes::GameController, 1});
			if (released & PSP_CTRL_RTRIGGER)
				pb::InputUp({InputTypes::GameController, 1});

			// Plunger: X button
			if (pressed & PSP_CTRL_CROSS)
				pb::InputDown({InputTypes::GameController, 4});
			if (released & PSP_CTRL_CROSS)
				pb::InputUp({InputTypes::GameController, 4});

			// Left Bump: D-pad Left
			if (pressed & PSP_CTRL_LEFT)
				pb::InputDown({InputTypes::GameController, 11});
			if (released & PSP_CTRL_LEFT)
				pb::InputUp({InputTypes::GameController, 11});

			// Right Bump: D-pad Right
			if (pressed & PSP_CTRL_RIGHT)
				pb::InputDown({InputTypes::GameController, 12});
			if (released & PSP_CTRL_RIGHT)
				pb::InputUp({InputTypes::GameController, 12});

			// Bottom Bump: D-pad Up
			if (pressed & PSP_CTRL_UP)
				pb::InputDown({InputTypes::GameController, 13});
			if (released & PSP_CTRL_UP)
				pb::InputUp({InputTypes::GameController, 13});

			// Start: Pause
			if (pressed & PSP_CTRL_START)
				pause();

			// Select: New Game
			if (pressed & PSP_CTRL_SELECT)
			{
				end_pause();
				pb::replay_level(false);
			}

			prevButtons = pad.Buttons;
		}

		if (!ProcessWindowMessages() || bQuit)
			break;

		if (has_focus)
		{
			if (!single_step && !no_time_loss)
			{
				auto dt = static_cast<float>(frameDuration.count());
				pb::frame(dt);
				updateCounter++;
			}
			no_time_loss = false;

			if (UpdateToFrameCounter >= UpdateToFrameRatio)
			{
				SDL_RenderClear(Renderer);
				render::PresentVScreen();
				SDL_RenderPresent(Renderer);
				frameCounter++;
				UpdateToFrameCounter -= UpdateToFrameRatio;
			}

			auto updateEnd = Clock::now();
			auto targetTimeDelta = TargetFrameTime - DurationMs(updateEnd - frameStart) - sleepRemainder;

			TimePoint frameEnd;
			if (targetTimeDelta > DurationMs::zero() && !Options.UncappedUpdatesPerSecond)
			{
				if (Options.HybridSleep)
					HybridSleep(targetTimeDelta);
				else
					std::this_thread::sleep_for(targetTimeDelta);
				frameEnd = Clock::now();
			}
			else
			{
				frameEnd = updateEnd;
			}

			sleepRemainder = Clamp(DurationMs(frameEnd - updateEnd) - targetTimeDelta, -TargetFrameTime,
			                       TargetFrameTime);
			frameDuration = std::min<DurationMs>(DurationMs(frameEnd - frameStart), 2 * TargetFrameTime);
			frameStart = frameEnd;
			UpdateToFrameCounter++;
		}
	}
}

int winmain::event_handler(const SDL_Event* event)
{
	switch (event->type)
	{
	case SDL_QUIT:
		end_pause();
		bQuit = true;
		fullscrn::shutdown();
		return_value = 0;
		return 0;
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		pb::InputDown({InputTypes::Keyboard, event->key.keysym.sym});
		break;
	case SDL_KEYUP:
		pb::InputUp({InputTypes::Keyboard, event->key.keysym.sym});
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		pb::InputDown({InputTypes::GameController, event->cbutton.button});
		break;
	case SDL_CONTROLLERBUTTONUP:
		pb::InputUp({InputTypes::GameController, event->cbutton.button});
		break;
	default: ;
	}

	return 1;
}

int winmain::ProcessWindowMessages()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (!event_handler(&event))
			return 0;
	}
	return 1;
}

void winmain::memalloc_failure()
{
	midi::music_stop();
	Sound::Close();
	pb::ShowMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal", "Out of memory");
	std::exit(1);
}

void winmain::end_pause()
{
	if (single_step)
	{
		pb::pause_continue();
		no_time_loss = true;
	}
}

void winmain::new_game()
{
	end_pause();
	pb::replay_level(false);
}

void winmain::pause(bool toggle)
{
	if (toggle || !single_step)
	{
		pb::pause_continue();
		no_time_loss = true;
	}
}

void winmain::Restart()
{
	restart = true;
	SDL_Event event{SDL_QUIT};
	SDL_PushEvent(&event);
}

void winmain::UpdateFrameRate()
{
	auto fps = Options.FramesPerSecond.V, ups = Options.UpdatesPerSecond.V;
	UpdateToFrameRatio = static_cast<double>(ups) / fps;
	TargetFrameTime = DurationMs(1000.0 / ups);
}

void winmain::HybridSleep(DurationMs sleepTarget)
{
	static constexpr double StdDevFactor = 0.5;

	while (sleepTarget > SpinThreshold)
	{
		auto start = Clock::now();
		std::this_thread::sleep_for(DurationMs(1));
		auto end = Clock::now();

		auto actualDuration = DurationMs(end - start);
		sleepTarget -= actualDuration;

		SleepState.Advance(actualDuration.count());
		SpinThreshold = DurationMs(SleepState.mean + SleepState.GetStdDev() * StdDevFactor);
	}

	for (auto start = Clock::now(); DurationMs(Clock::now() - start) < sleepTarget;);
}
