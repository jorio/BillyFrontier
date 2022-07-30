/****************************/
/*        WINDOWS           */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
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

static void MoveFadeEvent(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

float		gGammaFadePercent = 100.0;

int				gGameWindowWidth, gGameWindowHeight;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
	IMPLEMENT_ME_SOFT();
	SDL_GetWindowSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);
}




#pragma mark -


/**************** GAMMA FADE IN *************************/

void GammaFadeIn(void)
{
	while(gGammaFadePercent < 100.0f)
	{
		gGammaFadePercent += 7.0f;
		if (gGammaFadePercent > 100.0f)
			gGammaFadePercent = 100.0f;

#if ALLOW_FADE	
		IMPLEMENT_ME_SOFT(); //  DSpContext_FadeGamma(gDisplayContext, gGammaFadePercent, nil);
		Wait(1);
#endif		
	}
}

/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(void)
{
	while(gGammaFadePercent > 0.0f)
	{
		gGammaFadePercent -= 7.0f;
		if (gGammaFadePercent < 0.0f)
			gGammaFadePercent = 0.0f;

#if ALLOW_FADE	
		IMPLEMENT_ME_SOFT(); // DSpContext_FadeGamma(gDisplayContext, gGammaFadePercent, nil);
		Wait(1);
#endif	
	}
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
	if (gGammaFadePercent != 100.0f)
	{
#if ALLOW_FADE	
		IMPLEMENT_ME_SOFT(); // DSpContext_FadeGamma(MONITORS_TO_FADE, 100, nil);
#endif		
		gGammaFadePercent = 100.0f;
	}
}

/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
}


/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean	fadeIn)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;
	
	while (thisNodePtr)
	{
		if (thisNodePtr->MoveCall == MoveFadeEvent)
		{
			thisNodePtr->Flag[0] = fadeIn;								// set new mode
			return;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */
			
	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB + 1000;
	gNewObjectDefinition.moveCall = MoveFadeEvent;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Flag[0] = fadeIn;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
				
			/* SEE IF FADE IN */
			
	if (theNode->Flag[0])
	{
		gGammaFadePercent += 400.0f*fps;
		if (gGammaFadePercent >= 100.0f)										// see if @ 100%
		{
			gGammaFadePercent = 100.0f;
			DeleteObject(theNode);
		}
#if ALLOW_FADE
#if 1
		IMPLEMENT_ME_SOFT();
#else
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
#endif
#endif
	}
	
			/* FADE OUT */
	else
	{
		gGammaFadePercent -= 400.0f*fps;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			DeleteObject(theNode);
		}
#if ALLOW_FADE
#if 1
		IMPLEMENT_ME_SOFT();
#else
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
#endif
#endif
	}
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
