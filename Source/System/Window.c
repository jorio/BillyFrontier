/****************************/
/*        WINDOWS           */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include	"window.h"
#include "Pomme.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadePane(ObjNode *theNode);
static void DrawFadePane(ObjNode *theNode);

#define FaderMode			Flag[0]
#define FaderDone			Flag[1]
#define FaderSpeed			SpecialF[0]
#define FaderFrameCounter	Special[0]

/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

static float	gGammaFadeFrac = 1.0;

int				gGameWindowWidth, gGameWindowHeight;



/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
	SDL_GetWindowSizeInPixels(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);
}




#pragma mark -


/***************** FREEZE-FRAME FADE OUT ********************/

void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void))
{
#if 0
	if (gDebugMode)
	{
		gGammaFadeFrac = 0;
		return;
	}
#endif

	ObjNode* fader = MakeFadeEvent(false);

	long pFaderFrameCount = fader->FaderFrameCounter;

	while (!fader->FaderDone)
	{
		CalcFramesPerSecond();
		DoSDLMaintenance();

		if (moveCall)
		{
			moveCall();
		}

		// Force fader object to move even if MoveObjects was skipped
		if (fader->FaderFrameCounter == pFaderFrameCount)	// fader wasn't moved by moveCall
		{
			MoveFadePane(fader);
			pFaderFrameCount = fader->FaderFrameCounter;
		}

		OGL_DrawScene(drawCall);
	}

	// Draw one more blank frame
	gGammaFadeFrac = 0;
	CalcFramesPerSecond();
	DoSDLMaintenance();
	OGL_DrawScene(drawCall);

#if 0
	if (gGameView->fadeSound)
	{
		FadeSound(0);
		KillSong();
		StopAllEffectChannels();
		FadeSound(1);		// restore sound volume for new playback
	}
#endif
}


/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

ObjNode* MakeFadeEvent(Boolean fadeIn)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;
	
	while (thisNodePtr)
	{
		if (thisNodePtr->MoveCall == MoveFadePane)
		{
			thisNodePtr->FaderMode = fadeIn;							// set new mode
			thisNodePtr->FaderDone = false;								// set new mode
			return thisNodePtr;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */

	gGammaFadeFrac = fadeIn ? 0 : 1.0f;
	
	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = 0,
		.slot = FADEPANE_SLOT,
		.moveCall = MoveFadePane,
		.scale = 1,
	};

	newObj = MakeNewObject(&def);
	newObj->CustomDrawFunction = DrawFadePane;

	newObj->FaderMode = fadeIn;
	newObj->FaderDone = false;
	newObj->FaderSpeed = 4.0f;
	newObj->FaderFrameCounter = 0;

	return newObj;
}

/***************** MOVE FADE EVENT ********************/

static void MoveFadePane(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF FADE IN */
			
	if (theNode->FaderMode)
	{
		gGammaFadeFrac += theNode->FaderSpeed * fps;
		if (gGammaFadeFrac >= 1.0f)										// see if @ 100%
		{
			theNode->FaderDone = true;
			gGammaFadeFrac = 1.0f;
		}
	}
	
			/* FADE OUT */
	else
	{
		gGammaFadeFrac -= theNode->FaderSpeed * fps;
		if (gGammaFadeFrac <= 0.0f)													// see if @ 0%
		{
			theNode->FaderDone = true;
			gGammaFadeFrac = 0;
		}
	}

	if (theNode->FaderDone)
	{
		if (theNode->FaderMode)			// nuke it if fading in, hold it if fading out
			DeleteObject(theNode);

		theNode->MoveCall = NULL;		// don't run this again
	}
}

/***************** DRAW FADE PANE ********************/

static void DrawFadePane(ObjNode* theNode)
{
	(void) theNode;

	OGL_PushState();
	SetInfobarSpriteState(0);

	glDisable(GL_TEXTURE_2D);
	SetColor4f(0, 0, 0, 1.0f - gGammaFadeFrac);// (GLfloat*)&theNode->ColorFilter);
	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
	glVertex3f(-1000, -1000, 0);
	glVertex3f( 1000, -1000, 0);
	glVertex3f( 1000,  1000, 0);
	glVertex3f(-1000,  1000, 0);
	glEnd();

	glDisable(GL_BLEND);

	OGL_PopState();
}


/************************** ENTER 2D *************************/

void Enter2D(Boolean pauseDSp)
{
	(void) pauseDSp;

	SDL_ShowCursor();
	MyFlushEvents();
}


/************************** EXIT 2D *************************/

void Exit2D(void)
{
}


#pragma mark -


/********************** WAIT **********************/

void Wait(long ticks)
{
long	start;
	
	start = TickCount();

	while (TickCount()-start < ticks)
		MyFlushEvents(); 

}

/********************** GET DEFAULT WINDOW SIZE **********************/

void GetDefaultWindowSize(int display, int* width, int* height)
{
	const float aspectRatio = 4.0f / 3.0f;
	const float screenCoverage = .8f;

	SDL_Rect displayBounds = { .x = 0, .y = 0, .w = 640, .h = 480 };
	SDL_GetDisplayUsableBounds(display, &displayBounds);

	if (displayBounds.w > displayBounds.h)
	{
		*width	= displayBounds.h * screenCoverage * aspectRatio;
		*height	= displayBounds.h * screenCoverage;
	}
	else
	{
		*width	= displayBounds.w * screenCoverage;
		*height	= displayBounds.w * screenCoverage / aspectRatio;
	}
}

/********************** GET NUM DISPLAYS **********************/

int GetNumDisplays(void)
{
	int numDisplays = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
	SDL_free(displays);
	return numDisplays;
}

/******************** MOVE WINDOW TO PREFERRED DISPLAY *******************/
//
// This works best in windowed mode.
// Turn off fullscreen before calling this!
//

void MoveToPreferredDisplay(void)
{
	if (gGamePrefs.displayNumMinus1 >= GetNumDisplays())
	{
		gGamePrefs.displayNumMinus1 = 0;
	}

	SDL_DisplayID display = gGamePrefs.displayNumMinus1 + 1;

	int w = 640;
	int h = 480;
	GetDefaultWindowSize(display, &w, &h);
	SDL_SetWindowSize(gSDLWindow, w, h);
	SDL_SyncWindow(gSDLWindow);

	int centered = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
	SDL_SetWindowPosition(gSDLWindow, centered, centered);
	SDL_SyncWindow(gSDLWindow);
}

/*********************** SET FULLSCREEN MODE **********************/

void SetFullscreenMode(bool enforceDisplayPref)
{
	if (!gGamePrefs.fullscreen)
	{
		SDL_SetWindowFullscreen(gSDLWindow, 0);
		SDL_SyncWindow(gSDLWindow);

		if (enforceDisplayPref)
		{
			MoveToPreferredDisplay();
		}
	}
	else
	{
		if (enforceDisplayPref)
		{
			SDL_DisplayID currentDisplay = SDL_GetDisplayForWindow(gSDLWindow);
			SDL_DisplayID desiredDisplay = gGamePrefs.displayNumMinus1 + 1;

			if (currentDisplay != desiredDisplay)
			{
				// We must switch back to windowed mode for the preferred monitor to take effect
				SDL_SetWindowFullscreen(gSDLWindow, false);
				SDL_SyncWindow(gSDLWindow);
				MoveToPreferredDisplay();
			}
		}

		// Enter fullscreen mode
		SDL_SetWindowFullscreen(gSDLWindow, true);
		SDL_SyncWindow(gSDLWindow);
	}

	SDL_GL_SetSwapInterval(1);
}
