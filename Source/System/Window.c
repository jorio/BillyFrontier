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

static void MoveFadePane(ObjNode *theNode);
static void DrawFadePane(ObjNode *theNode, const OGLSetupOutputType* setupInfo);

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
	IMPLEMENT_ME_SOFT();
	SDL_GetWindowSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);
}




#pragma mark -


/**************** GAMMA FADE IN *************************/

void GammaFadeIn(void)
{
	while (gGammaFadeFrac < 1.0f)
	{
		gGammaFadeFrac += 0.07f;
		if (gGammaFadeFrac > 1.0f)
			gGammaFadeFrac = 1.0f;

#if ALLOW_FADE	
		IMPLEMENT_ME_SOFT(); //  DSpContext_FadeGamma(gDisplayContext, gGammaFadePercent, nil);
		Wait(1);
#endif		
	}
}

/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(void)
{
	while(gGammaFadeFrac > 0.0f)
	{
		gGammaFadeFrac -= 0.07f;
		if (gGammaFadeFrac < 0.0f)
			gGammaFadeFrac = 0.0f;

#if ALLOW_FADE	
		IMPLEMENT_ME_SOFT(); // DSpContext_FadeGamma(gDisplayContext, gGammaFadePercent, nil);
		Wait(1);
#endif	
	}
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
	if (gGammaFadeFrac != 1.0f)
	{
#if ALLOW_FADE	
		IMPLEMENT_ME_SOFT(); // DSpContext_FadeGamma(MONITORS_TO_FADE, 100, nil);
#endif		
		gGammaFadeFrac = 1.0f;
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
		if (thisNodePtr->MoveCall == MoveFadePane)
		{
			thisNodePtr->Flag[0] = fadeIn;								// set new mode
			return;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */
			
	gNewObjectDefinition.genre = CUSTOM_GENRE;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB + 1000;
	gNewObjectDefinition.moveCall = MoveFadePane;
	newObj = MakeNewObject(&gNewObjectDefinition);
	newObj->CustomDrawFunction = DrawFadePane;

	newObj->Flag[0] = fadeIn;
}

/***************** MOVE FADE EVENT ********************/

static void MoveFadePane(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF FADE IN */
			
	if (theNode->Flag[0])
	{
		gGammaFadeFrac += 4.0f*fps;
		if (gGammaFadeFrac >= 1.0f)										// see if @ 100%
		{
			gGammaFadeFrac = 1.0f;
			DeleteObject(theNode);
		}
	}
	
			/* FADE OUT */
	else
	{
		gGammaFadeFrac -= 4.0f*fps;
		if (gGammaFadeFrac <= 0.0f)													// see if @ 0%
		{
			gGammaFadeFrac = 0;
			DeleteObject(theNode);
		}
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
