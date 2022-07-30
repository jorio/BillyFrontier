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
static void DrawFadePane(ObjNode *theNode, const OGLSetupOutputType* setupInfo);

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



void DoSDLMaintenance(void)
{
	IMPLEMENT_ME_SOFT();
}

/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
	IMPLEMENT_ME_SOFT();
	SDL_GetWindowSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);
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

		OGL_DrawScene(gGameViewInfoPtr, drawCall);
	}

	// Draw one more blank frame
	gGammaFadeFrac = 0;
	CalcFramesPerSecond();
	DoSDLMaintenance();
	OGL_DrawScene(gGameViewInfoPtr, drawCall);

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


/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
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
	
	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = 0,
		.slot = SLOT_OF_DUMB + 1000,
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

static void DrawFadePane(ObjNode* theNode, const OGLSetupOutputType* setupInfo)
{
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

/************************ GAME SCREEN TO BLACK ************************/

void GameScreenToBlack(void)
{
}


/************************** ENTER 2D *************************/

void Enter2D(Boolean pauseDSp)
{
	InitCursor();
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
