// SDL INPUT
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/CroMagRally

#include "game.h"
#include <SDL3/SDL.h>

extern SDL_Window* gSDLWindow;

/***************/
/* CONSTANTS   */
/***************/

InputBinding kDefaultBindings[NUM_CONTROL_NEEDS] =
{
	[kNeed_Shoot] =
	{
		.mouseButton = SDL_BUTTON_LEFT,
		.hardKey = {SDL_SCANCODE_SPACE},
	},

	[kNeed_Reload] =		// unused in final version of the game
	{
		.hardKey = {SDL_SCANCODE_R},
	},

	[kNeed_Duck] =
	{
		.mouseButton = SDL_BUTTON_RIGHT,
		.userKey = {SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL},
	},

	[kNeed_Continue] =
	{
		.mouseButton = SDL_BUTTON_MIDDLE,
		.hardKey = {SDL_SCANCODE_RETURN},
#if __APPLE__
		.userKey = {SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI},
#else
		.userKey = {SDL_SCANCODE_LALT, SDL_SCANCODE_RALT},
#endif
	},

	[kNeed_Jump] =
	{
		.hardKey = {SDL_SCANCODE_SPACE},
#if __APPLE__
		.userKey = {SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI},
#else
		.userKey = {SDL_SCANCODE_LALT, SDL_SCANCODE_RALT},
#endif
	},

	[kNeed_Left] =
	{
		.hardKey = {SDL_SCANCODE_LEFT},
		.userKey = {SDL_SCANCODE_A},
	},

	[kNeed_Right] =
	{
		.hardKey = {SDL_SCANCODE_RIGHT},
		.userKey = {SDL_SCANCODE_D},
	},

	[kNeed_UIPause] =
	{
		.hardKey = {SDL_SCANCODE_ESCAPE},
		.hardPad = {{kInputTypeButton, SDL_GAMEPAD_BUTTON_START}},
	},

	[kNeed_UIBack] =
	{
		.hardKey = {SDL_SCANCODE_BACKSPACE},
		.userKey = {SDL_SCANCODE_ESCAPE},
		.hardPad = {{kInputTypeButton, SDL_GAMEPAD_BUTTON_BACK}},
	},

	[kNeed_UIConfirm] =
	{
		.hardKey = {SDL_SCANCODE_SPACE},
		.userKey = {SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER},
		.hardPad = {{kInputTypeButton, SDL_GAMEPAD_BUTTON_BACK}},
	},

	[kNeed_UIUp] =
	{
		.hardKey = {SDL_SCANCODE_UP},
		.userKey = {SDL_SCANCODE_W},
		.hardPad = {{kInputTypeButton, SDL_GAMEPAD_BUTTON_DPAD_UP}},
	},

	[kNeed_UIDown] =
	{
		.hardKey = {SDL_SCANCODE_DOWN},
		.userKey = {SDL_SCANCODE_S},
		.hardPad = {{kInputTypeButton, SDL_GAMEPAD_BUTTON_DPAD_DOWN}},
	},

	[kNeed_UILeft] =
	{
		.hardKey = {SDL_SCANCODE_LEFT},
		.userKey = {SDL_SCANCODE_A},
		.hardPad = {{kInputTypeButton, SDL_GAMEPAD_BUTTON_DPAD_LEFT}},
	},

	[kNeed_UIRight] =
	{
		.hardKey = {SDL_SCANCODE_RIGHT},
		.userKey = {SDL_SCANCODE_D},
		.hardPad = {{kInputTypeButton, SDL_GAMEPAD_BUTTON_DPAD_RIGHT}},
	},

};

enum
{
	KEYSTATE_ACTIVE_BIT		= 0b001,
	KEYSTATE_CHANGE_BIT		= 0b010,
	KEYSTATE_IGNORE_BIT		= 0b100,

	KEYSTATE_OFF			= 0b000,
	KEYSTATE_PRESSED		= KEYSTATE_ACTIVE_BIT | KEYSTATE_CHANGE_BIT,
	KEYSTATE_HELD			= KEYSTATE_ACTIVE_BIT,
	KEYSTATE_UP				= KEYSTATE_OFF | KEYSTATE_CHANGE_BIT,
	KEYSTATE_IGNOREHELD		= KEYSTATE_OFF | KEYSTATE_IGNORE_BIT,
};

#define kJoystickDeadZone				(33 * 32767 / 100)
#define kJoystickDeadZone_UI			(66 * 32767 / 100)
#define kJoystickDeadZoneFrac			(kJoystickDeadZone / 32767.0f)
#define kJoystickDeadZoneFracSquared	(kJoystickDeadZoneFrac * kJoystickDeadZoneFrac)

#define gNumLocalPlayers				1

/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

typedef struct Gamepad
{
	bool				open;
	bool				fallbackToKeyboard;
	SDL_Gamepad*		sdlGamepad;
	KeyState			needStates[NUM_CONTROL_NEEDS];
} Gamepad;

float				gMouseDeltaX = 0;
float				gMouseDeltaY = 0;
float				gMouseDPIScaleX = 1;
float				gMouseDPIScaleY = 1;
SInt32				gScrollWheelDelta = 0;

Boolean				gUserPrefersGamepad = false;

static Boolean		gGamepadPlayerMappingLocked = false;
Gamepad				gGamepads[MAX_LOCAL_PLAYERS];

static KeyState		gKeyboardStates[SDL_SCANCODE_COUNT];
static KeyState		gMouseButtonStates[NUM_SUPPORTED_MOUSE_BUTTONS];
static KeyState		gNeedStates[NUM_CONTROL_NEEDS];

Boolean				gMouseMotionNow = false;
char				gTextInput[64];

static void OnJoystickRemoved(SDL_JoystickID which);
static SDL_Gamepad* TryOpenGamepadFromJoystick(SDL_JoystickID joystickID);
static SDL_Gamepad* TryOpenAnyGamepad(bool showMessage);
static int GetGamepadSlotFromSDLJoystickID(SDL_JoystickID joystickID);

static inline const InputBinding* GetBinding(int need)
{
	return &kDefaultBindings[need];
}

#pragma mark -

/************************* INIT INPUT *********************************/

void InitInput(void)
{
}

/**************** READ KEYBOARD *************/

void ReadKeyboard(void)
{
	DoSDLMaintenance();
}

/***************** GET MOUSE COORD *****************/

OGLPoint2D GetLogicalMouseCoord(void)
{
	float windowX = 0;
	float windowY = 0;
	GetMousePixelCoord(&windowX, &windowY);
	OGLPoint2D windowPt =
	{
		0.5f + windowX,
		0.5f + windowY
	};

	return WindowPointToLogical(windowPt);
}

void GetMousePixelCoord(float *x, float *y)
{
	float windowX = 0;
	float windowY = 0;
	SDL_GetMouseState(&windowX, &windowY);

	*x = windowX * gMouseDPIScaleX;
	*y = windowY * gMouseDPIScaleY;
}

/**********************/

static inline void UpdateKeyState(KeyState* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_PRESSED:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;

		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_PRESSED : KEYSTATE_OFF;
			break;

		case KEYSTATE_IGNOREHELD:
			*state = downNow ? KEYSTATE_IGNOREHELD : KEYSTATE_OFF;
			break;
	}
}

void InvalidateNeedState(int need)
{
	gNeedStates[need] = KEYSTATE_IGNOREHELD;
}

void InvalidateAllInputs(void)
{
	_Static_assert(1 == sizeof(KeyState), "sizeof(KeyState) has changed -- Rewrite this function without memset()!");

	SDL_memset(gNeedStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
	SDL_memset(gKeyboardStates, KEYSTATE_IGNOREHELD, SDL_SCANCODE_COUNT);
	SDL_memset(gMouseButtonStates, KEYSTATE_IGNOREHELD, NUM_SUPPORTED_MOUSE_BUTTONS);

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		SDL_memset(gGamepads[i].needStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
	}
}

static void UpdateRawKeyboardStates(void)
{
	int numkeys = 0;
	const bool* keystate = SDL_GetKeyboardState(&numkeys);

	int minNumKeys = GAME_MIN(numkeys, SDL_SCANCODE_COUNT);

	for (int i = 0; i < minNumKeys; i++)
		UpdateKeyState(&gKeyboardStates[i], keystate[i]);

	// fill out the rest
	for (int i = minNumKeys; i < SDL_SCANCODE_COUNT; i++)
		UpdateKeyState(&gKeyboardStates[i], false);
}

static void ParseAltEnter(void)
{
	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		&& (GetKeyState(SDL_SCANCODE_LALT) || GetKeyState(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = !gGamePrefs.fullscreen;
		SetFullscreenMode(false);

		InvalidateAllInputs();
	}

}

static void UpdateMouseButtonStates(int mouseWheelDelta)
{
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	for (int i = 1; i < NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL; i++)	// SDL buttons start at 1!
	{
		bool buttonBit = 0 != (mouseButtons & SDL_BUTTON_MASK(i));
		UpdateKeyState(&gMouseButtonStates[i], buttonBit);
	}

	// Fake buttons for mouse wheel up/down
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELUP], mouseWheelDelta < 0);
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELDOWN], mouseWheelDelta > 0);
}

static void UpdateInputNeeds(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const InputBinding* kb = GetBinding(i);

		bool downNow = false;

		for (int j = 0; j < MAX_BINDINGS_PER_NEED; j++)
		{
			int16_t scancode = kb->key[j];
			if (scancode && scancode < SDL_SCANCODE_COUNT)
			{
				downNow |= gKeyboardStates[scancode] & KEYSTATE_ACTIVE_BIT;
			}
		}

		downNow |= gMouseButtonStates[kb->mouseButton] & KEYSTATE_ACTIVE_BIT;

		UpdateKeyState(&gNeedStates[i], downNow);
	}
}

static void UpdateGamepadSpecificInputNeeds(int gamepadNum)
{
	if (!gGamepads[gamepadNum].open)
	{
		return;
	}

	SDL_Gamepad* sdlGamepad = gGamepads[gamepadNum].sdlGamepad;

	for (int needNum = 0; needNum < NUM_CONTROL_NEEDS; needNum++)
	{
		const InputBinding* kb = GetBinding(needNum);

		int16_t deadZone = needNum >= NUM_REMAPPABLE_NEEDS
						   ? kJoystickDeadZone_UI
						   : kJoystickDeadZone;

		bool downNow = false;

		for (int buttonNum = 0; buttonNum < MAX_BINDINGS_PER_NEED; buttonNum++)
		{
			const PadBinding* pb = &kb->pad[buttonNum];

			switch (pb->type)
			{
				case kInputTypeButton:
					downNow |= 0 != SDL_GetGamepadButton(sdlGamepad, pb->id);
					break;

				case kInputTypeAxisPlus:
					downNow |= SDL_GetGamepadAxis(sdlGamepad, pb->id) > deadZone;
					break;

				case kInputTypeAxisMinus:
					downNow |= SDL_GetGamepadAxis(sdlGamepad, pb->id) < -deadZone;
					break;

				default:
					break;
			}
		}

		UpdateKeyState(&gGamepads[gamepadNum].needStates[needNum], downNow);
	}
}

#pragma mark -

/**********************/
/* PUBLIC FUNCTIONS   */
/**********************/

void DoSDLMaintenance(void)
{
	gTextInput[0] = '\0';
	gMouseMotionNow = false;
	gMouseDeltaX = 0;
	gMouseDeltaY = 0;

	int mouseWheelDeltaX = 0;
	int mouseWheelDeltaY = 0;

	// Update mouse DPI scale
	{
		int windowW = 1;
		int windowH = 1;
		SDL_GetWindowSize(gSDLWindow, &windowW, &windowH);  // NOT in pixels
		gMouseDPIScaleX = (float) gGameWindowWidth / (float) windowW;		// gGameWindowWidth is in actual pixels
		gMouseDPIScaleY = (float) gGameWindowHeight / (float) windowH;		// gGameWindowHeight is in actual pixels
	}

			/**********************/
			/* DO SDL MAINTENANCE */
			/**********************/

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				CleanQuit();			// throws Pomme::QuitRequest
				return;

			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				CleanQuit();	// throws Pomme::QuitRequest
				return;

			case SDL_EVENT_WINDOW_RESIZED:
				// QD3D_OnWindowResized(event.window.data1, event.window.data2);
				break;

			case SDL_EVENT_TEXT_INPUT:
				SDL_snprintf(gTextInput, sizeof(gTextInput), "%s", event.text.text);
				break;

			case SDL_EVENT_MOUSE_MOTION:
				gMouseMotionNow = true;
				gMouseDeltaX += event.motion.xrel * gMouseDPIScaleX;
				gMouseDeltaY += event.motion.yrel * gMouseDPIScaleY;
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				mouseWheelDeltaX += event.wheel.x;
				mouseWheelDeltaY += event.wheel.y;
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				TryOpenGamepadFromJoystick(event.gdevice.which);
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				OnJoystickRemoved(event.gdevice.which);
				break;

			case SDL_EVENT_GAMEPAD_REMAPPED:
				SDL_Log("Gamepad device remapped! %d", event.gdevice.which);
				break;

			case SDL_EVENT_KEY_DOWN:
				gUserPrefersGamepad = false;
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				gUserPrefersGamepad = true;
				break;
		}
	}


	// Refresh the state of each individual keyboard key
	UpdateRawKeyboardStates();

	// On ALT+ENTER, toggle fullscreen, and ignore ENTER until keyup
	ParseAltEnter();

	// Refresh the state of each mouse button
	UpdateMouseButtonStates(mouseWheelDeltaY);
	gScrollWheelDelta = mouseWheelDeltaX - mouseWheelDeltaY;	// for edge scrolling in-game

	// Refresh the state of each input need
	UpdateInputNeeds();

	//-------------------------------------------------------------------------
	// Multiplayer gamepad input
	//-------------------------------------------------------------------------

	for (int gamepadNum = 0; gamepadNum < MAX_LOCAL_PLAYERS; gamepadNum++)
	{
		UpdateGamepadSpecificInputNeeds(gamepadNum);
	}
}

#pragma mark -

Boolean GetKeyState(uint16_t sdlScancode)
{
	if (sdlScancode >= SDL_SCANCODE_COUNT)
		return false;
	return 0 != (gKeyboardStates[sdlScancode] & KEYSTATE_ACTIVE_BIT);
}

Boolean GetNewKeyState(uint16_t sdlScancode)
{
	if (sdlScancode >= SDL_SCANCODE_COUNT)
		return false;
	return gKeyboardStates[sdlScancode] == KEYSTATE_PRESSED;
}

#pragma mark -

Boolean GetClickState(int mouseButton)
{
	if (mouseButton >= NUM_SUPPORTED_MOUSE_BUTTONS)
		return false;
	return 0 != (gMouseButtonStates[mouseButton] & KEYSTATE_ACTIVE_BIT);
}

Boolean GetNewClickState(int mouseButton)
{
	if (mouseButton >= NUM_SUPPORTED_MOUSE_BUTTONS)
		return false;
	return gMouseButtonStates[mouseButton] == KEYSTATE_PRESSED;
}

#pragma mark -

Boolean GetNeedStateP(int needID, int playerID)
{
	const Gamepad* gamepad = &gGamepads[playerID];

	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (gamepad->open && (gamepad->needStates[needID] & KEYSTATE_ACTIVE_BIT))
	{
		return true;
	}

	// Fallback to KB/M
	if (gNumLocalPlayers <= 1 || gamepad->fallbackToKeyboard)
	{
		return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
	}

	return false;
}

Boolean GetNeedState(int needID)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gGamepads[i].open
			&& (gGamepads[i].needStates[needID] & KEYSTATE_ACTIVE_BIT))
		{
			return true;
		}
	}

	// Fallback to KB/M
	return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
}

Boolean GetNewNeedStateP(int needID, int playerID)
{
	const Gamepad* gamepad = &gGamepads[playerID];

	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (gamepad->open && gamepad->needStates[needID] == KEYSTATE_PRESSED)
	{
		return true;
	}

	// Fallback to KB/M
    if (gNumLocalPlayers <= 1 || gamepad->fallbackToKeyboard)
	{
		return gNeedStates[needID] == KEYSTATE_PRESSED;
	}

	return false;
}

Boolean GetNewNeedState(int needID)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gGamepads[i].open
			&& (gGamepads[i].needStates[needID] == KEYSTATE_PRESSED))
		{
			return true;
		}
	}

	// Fallback to KB/M
	return gNeedStates[needID] == KEYSTATE_PRESSED;
}

Boolean UserWantsOut(void)
{
	return GetNewNeedState(kNeed_UIConfirm)
		|| GetNewNeedState(kNeed_UIBack)
		|| GetNewNeedState(kNeed_UIPause)
//		|| GetNewClickState(SDL_BUTTON_LEFT)
		;
}

Boolean IsCheatKeyComboDown(void)
{
	return (GetKeyState(SDL_SCANCODE_LGUI) || GetKeyState(SDL_SCANCODE_RGUI))
		&& (GetKeyState(SDL_SCANCODE_F10));
}

#pragma mark -

float GetGamepadAnalogSteeringAxis(SDL_Gamepad* sdlController, SDL_GamepadAxis axis)
{
			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	float steer = 0; 											// assume no control input

			/* FIRST CHECK ANALOG AXES */

	if (sdlController)
	{
		Sint16 dxRaw = SDL_GetGamepadAxis(sdlController, axis);

		steer = dxRaw / 32767.0f;
		float steerMag = fabsf(steer);

		if (steerMag < kJoystickDeadZoneFrac)
		{
			steer = 0;
		}
		else if (steer < -1.0f)
		{
			steer = -1.0f;
		}
		else if (steer > 1.0f)
		{
			steer = 1.0f;
		}
		else
		{
			// Avoid magnitude bump when thumbstick is pushed past dead zone:
			// Bring magnitude from [kJoystickDeadZoneFrac, 1.0] to [0.0, 1.0].
			float steerSign = steer < 0 ? -1.0f : 1.0f;
			steer = steerSign * (steerMag - kJoystickDeadZoneFrac) / (1.0f - kJoystickDeadZoneFrac);
		}
	}

	return steer;
}


OGLVector2D GetAnalogSteering(int playerID)
{
	OGLVector2D steer = {0, 0};								// assume no control input

	SDL_Gamepad* sdlGamepad = SDL_GetGamepadFromPlayerIndex(playerID);

			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

			/* FIRST CHECK ANALOG AXES */

	if (sdlGamepad)
	{
		steer.x = GetGamepadAnalogSteeringAxis(sdlGamepad, SDL_GAMEPAD_AXIS_LEFTX);
		steer.y = GetGamepadAnalogSteeringAxis(sdlGamepad, SDL_GAMEPAD_AXIS_LEFTY);
	}

			/* NEXT CHECK THE DIGITAL KEYS */

	if (GetNeedStateP(kNeed_UILeft, playerID))					// is Left Key pressed?
	{
		steer.x = -1.0f;
	}
	else if (GetNeedStateP(kNeed_UIRight, playerID))			// is Right Key pressed?
	{
		steer.x = 1.0f;
	}

	if (GetNeedStateP(kNeed_UIUp, playerID))					// is Up Key pressed?
	{
		steer.y = -1.0f;
	}
	else if (GetNeedStateP(kNeed_UIDown, playerID))				// is Down Key pressed?
	{
		steer.y = 1.0f;
	}

	return steer;
}

#pragma mark -

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

int GetNumControllers(void)
{
	int count = 0;

#if 0
	for (int i = 0; i < SDL_NumJoysticks(); ++i)
	{
		if (SDL_IsGameController(i))
		{
			count++;
		}
	}
#else
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gGamepads[i].open)
		{
			count++;
		}
	}
#endif

	return count;
}

static SDL_Gamepad* GetGamepad(int n)
{
	if (gGamepads[n].open)
	{
		return gGamepads[n].sdlGamepad;
	}
	else
	{
		return NULL;
	}
}

static int FindFreeGamepadSlot()
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (!gGamepads[i].open)
		{
			return i;
		}
	}

	return -1;
}

static int GetGamepadSlotFromJoystick(SDL_JoystickID joystickID)
{
	for (int slot = 0; slot < MAX_LOCAL_PLAYERS; slot++)
	{
		if (gGamepads[slot].open &&
			SDL_GetGamepadID(gGamepads[slot].sdlGamepad) == joystickID)
		{
			return slot;
		}
	}

	return -1;
}

static SDL_Gamepad* TryOpenGamepadFromJoystick(SDL_JoystickID joystickID)
{
	int gamepadSlot = -1;

	// First, check that it's not in use already
	gamepadSlot = GetGamepadSlotFromJoystick(joystickID);
	if (gamepadSlot >= 0)	// in use
	{
		return gGamepads[gamepadSlot].sdlGamepad;
	}
	
	// If we can't get an SDL_Gamepad from that joystick, don't bother
	if (!SDL_IsGamepad(joystickID))
	{
		return NULL;
	}

	// Reserve a controller slot
	gamepadSlot = FindFreeGamepadSlot();
	if (gamepadSlot < 0)
	{
		SDL_Log("All gamepad slots used up.");
		// TODO: when a controller is unplugged, if all controller slots are used up, re-scan connected joysticks and try to open any unopened joysticks.
		return NULL;
	}

	// Use this one
	SDL_Gamepad* sdlGamepad = SDL_OpenGamepad(joystickID);

	// Assign player ID
	SDL_SetGamepadPlayerIndex(sdlGamepad, gamepadSlot);

	gGamepads[gamepadSlot] = (Gamepad)
	{
		.open = true,
		.sdlGamepad = sdlGamepad,
	};

	SDL_Log("Opened joystick %d as gamepad: %s",
			joystickID,
			SDL_GetGamepadName(gGamepads[gamepadSlot].sdlGamepad));

	return gGamepads[gamepadSlot].sdlGamepad;
}

static SDL_Gamepad* TryOpenAnyUnusedGamepad(bool showMessage)
{
	int numJoysticks = 0;
	int numJoysticksAlreadyInUse = 0;

	SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
	SDL_Gamepad* newGamepad = NULL;

	for (int i = 0; i < numJoysticks; ++i)
	{
		SDL_JoystickID joystickID = joysticks[i];
		
		// Usable as an SDL_Gamepad?
		if (!SDL_IsGamepad(joystickID))
		{
			continue;
		}

		// Already in use?
		if (GetGamepadSlotFromJoystick(joystickID) >= 0)
		{
			numJoysticksAlreadyInUse++;
			continue;
		}

		// Use this one
		newGamepad = TryOpenGamepadFromJoystick(joystickID);
		if (newGamepad)
		{
			break;
		}
	}

	if (newGamepad)
	{
		// OK
	}
	else if (numJoysticksAlreadyInUse == numJoysticks)
	{
		// No-op; All joysticks already in use (or there might be zero joysticks)
	}
	else
	{
		SDL_Log("%d joysticks found, but none is suitable as an SDL_Gamepad.", numJoysticks);
		if (showMessage)
		{
			char messageBuf[1024];
			SDL_snprintf(messageBuf, sizeof(messageBuf),
					"The game does not support your controller yet (\"%s\").\n\n"
					"You can play with the keyboard and mouse instead. Sorry!",
					 SDL_GetJoystickNameForID(joysticks[0]));
			SDL_ShowSimpleMessageBox(
					SDL_MESSAGEBOX_WARNING,
					"Controller not supported",
					messageBuf,
					gSDLWindow);
		}
	}
	
	SDL_free(joysticks);

	return newGamepad;
}

void Rumble(float strength, uint32_t ms)
{
#if 0	// TODO: Rumble for specific player
	if (NULL == gSDLController || !gGamePrefs.gamepadRumble)
		return;

	SDL_GameControllerRumble(gSDLController, (Uint16)(strength * 65535), (Uint16)(strength * 65535), ms);
#else
	(void) strength;
	(void) ms;
#endif
}

static int GetGamepadSlotFromSDLJoystickID(SDL_JoystickID joystickID)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gGamepads[i].open && SDL_GetGamepadID(gGamepads[i].sdlGamepad) == joystickID)
		{
			return i;
		}
	}

	return -1;
}

static void CloseGamepad(int gamepadSlot)
{
	GAME_ASSERT(gGamepads[gamepadSlot].open);
	GAME_ASSERT(gGamepads[gamepadSlot].sdlGamepad);

	SDL_CloseGamepad(gGamepads[gamepadSlot].sdlGamepad);
	gGamepads[gamepadSlot].open = false;
	gGamepads[gamepadSlot].sdlGamepad = NULL;
}

static void MoveGamepad(int oldSlot, int newSlot)
{
	if (oldSlot == newSlot)
		return;

	SDL_Log("Remapping player gamepad %d ---> %d\n", oldSlot, newSlot);

	gGamepads[newSlot] = gGamepads[oldSlot];
	
	// TODO: Does this actually work??
	if (gGamepads[newSlot].open)
	{
		SDL_SetGamepadPlayerIndex(gGamepads[newSlot].sdlGamepad, newSlot);
	}

	// Clear duplicate slot so we don't read it by mistake in the future
	gGamepads[oldSlot].open = false;
	gGamepads[oldSlot].sdlGamepad = NULL;
}

static void CompactGamepadSlots(void)
{
	int writeIndex = 0;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		GAME_ASSERT(writeIndex <= i);

		if (gGamepads[i].open)
		{
			MoveGamepad(i, writeIndex);
			writeIndex++;
		}
	}
}

static void TryFillUpVacantGamepadSlots(void)
{
	while (TryOpenAnyUnusedGamepad(false) != NULL)
	{
		// Successful; there might be more joysticks available, keep going
	}
}

static void OnJoystickRemoved(SDL_JoystickID joystickID)
{
	int gamepadSlot = GetGamepadSlotFromSDLJoystickID(joystickID);

	if (gamepadSlot >= 0)		// we're using this joystick
	{
		SDL_Log("Joystick %d was removed, was used by gamepad slot #%d", joystickID, gamepadSlot);

		// Nuke reference to this controller+joystick
		CloseGamepad(gamepadSlot);
	}

	if (!gGamepadPlayerMappingLocked)
	{
		CompactGamepadSlots();
	}

	// Fill up any controller slots that are vacant
	TryFillUpVacantGamepadSlots();
}

void LockPlayerGamepadMapping(void)
{
	int keyboardPlayer = gNumLocalPlayers-1;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		gGamepads[i].fallbackToKeyboard = (i == keyboardPlayer);
	}

	gGamepadPlayerMappingLocked = true;
}

void UnlockPlayerGamepadMapping(void)
{
	gGamepadPlayerMappingLocked = false;
	CompactGamepadSlots();
	TryFillUpVacantGamepadSlots();
}

#if 0
const char* GetPlayerName(int whichPlayer)
{
	static char playerName[64];

	SDL_snprintf(playerName, sizeof(playerName),
			"%s %d", Localize(STR_PLAYER), whichPlayer+1);

	return playerName;
}

const char* GetPlayerNameWithInputDeviceHint(int whichPlayer)
{
	static char playerName[128];

	playerName[0] = '\0';

	snprintfcat(playerName, sizeof(playerName),
			"%s %d", Localize(STR_PLAYER), whichPlayer+1);

	if (gGameMode == GAME_MODE_CAPTUREFLAG)
	{
		snprintfcat(playerName, sizeof(playerName),
			", %s", Localize(gPlayerInfo[whichPlayer].team == 0 ? STR_RED_TEAM : STR_GREEN_TEAM));
	}

	bool enoughControllers = GetNumControllers() >= gNumLocalPlayers;

	if (!enoughControllers)
	{
		bool hasGamepad = gControllers[whichPlayer].open;
		snprintfcat(playerName, sizeof(playerName),
					"\n[%s]", Localize(hasGamepad? STR_GAMEPAD: STR_KEYBOARD));
	}

	return playerName;
}
#endif

#pragma mark -

#if 0
void ResetDefaultKeyboardBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].key, kDefaultInputBindings[i].key, sizeof(gGamePrefs.bindings[i].key));
	}
}

void ResetDefaultGamepadBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].pad, kDefaultInputBindings[i].pad, sizeof(gGamePrefs.bindings[i].pad));
	}
}

void ResetDefaultMouseBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		gGamePrefs.bindings[i].mouseButton = kDefaultInputBindings[i].mouseButton;
	}
}
#endif
