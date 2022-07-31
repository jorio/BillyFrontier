//
// input.h
//

#pragma once

#define NUM_SUPPORTED_MOUSE_BUTTONS			31
#define NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL	(NUM_SUPPORTED_MOUSE_BUTTONS-2)
#define SDL_BUTTON_WHEELUP					(NUM_SUPPORTED_MOUSE_BUTTONS-2)		// make wheelup look like it's a button
#define SDL_BUTTON_WHEELDOWN				(NUM_SUPPORTED_MOUSE_BUTTONS-1)		// make wheeldown look like it's a button

#define MAX_USER_BINDINGS_PER_NEED		2
#define MAX_HARD_BINDINGS_PER_NEED		1
#define MAX_BINDINGS_PER_NEED		(MAX_USER_BINDINGS_PER_NEED + MAX_HARD_BINDINGS_PER_NEED)

#define MAX_LOCAL_PLAYERS				1

typedef struct
{
	int8_t		type;
	int8_t		id;
} PadBinding;

typedef struct
{
	// SDL scancodes
	union
	{
		int16_t			key[MAX_BINDINGS_PER_NEED];

		struct
		{
			int16_t		userKey[MAX_USER_BINDINGS_PER_NEED];
			int16_t		hardKey[MAX_HARD_BINDINGS_PER_NEED];
		};
	};

	// Controller buttons
	union
	{
		PadBinding		pad[MAX_BINDINGS_PER_NEED];

		struct
		{
			PadBinding	userPad[MAX_USER_BINDINGS_PER_NEED];
			PadBinding	hardPad[MAX_HARD_BINDINGS_PER_NEED];
		};
	};

	int8_t				mouseButton;

} InputBinding;

enum
{
	kInputTypeUnbound = 0,
	kInputTypeButton,
	kInputTypeAxisPlus,
	kInputTypeAxisMinus,
};

enum
{
	kNeed_Shoot,
	kNeed_Duck,
	kNeed_Reload,
	kNeed_Continue,
	kNeed_Jump,
	kNeed_Left,
	kNeed_Right,
	NUM_REMAPPABLE_NEEDS,

	kNeed_UIPause = NUM_REMAPPABLE_NEEDS,
	kNeed_UIBack,
	kNeed_UIConfirm,
	kNeed_UIUp,
	kNeed_UIDown,
	kNeed_UILeft,
	kNeed_UIRight,
	NUM_CONTROL_NEEDS,
};

//============================================================================================

extern	void InitInput(void);
extern	void ReadKeyboard(void);

void TurnOnISp(void);
void TurnOffISp(void);

OGLPoint2D GetLogicalMouseCoord(void);

void DoSDLMaintenance(void);
Boolean GetKeyState(uint16_t sdlScancode);
Boolean GetNewKeyState(uint16_t sdlScancode);
Boolean GetClickState(int mouseButton);
Boolean GetNewClickState(int mouseButton);
Boolean GetNeedState(int need);
Boolean GetNewNeedState(int need);
Boolean IsCheatKeyComboDown(void);
Boolean UserWantsOut(void);

void InvalidateAllInputs(void);
