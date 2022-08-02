/****************************/
/*   	MISCSCREENS.C	    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DisplayPicture_Draw(void);
static void MoveDarkenPane(ObjNode *theNode);
static void DrawDarkenPane(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

MOPictureObject 	*gBackgoundPicture = nil;

OGLSetupOutputType	*gScreenViewInfoPtr = nil;



/******* DO PROGRAM WARM-UP SCREEN AS WE PRELOAD ASSETS **********/

void DoWarmUpScreen(void)
{
	OGLSetupInputType	viewDef;

			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);
//	viewDef.view.pillarboxRatio = PILLARBOX_RATIO_FULLSCREEN;
//	viewDef.view.fontName = "rockfont";

	OGL_SetupWindow(&viewDef);

			/* SHOW IT */

	for (int i = 0; i < 8; i++)
	{
		OGL_DrawScene(DrawObjects);
		DoSDLMaintenance();
	}

			/* CLEANUP */

	DeleteAllObjects();

	OGL_DisposeWindowSetup();
}


/********************** DISPLAY PICTURE **************************/
//
// Displays a picture using OpenGL until the user clicks the mouse or presses any key.
// If showAndBail == true, then show it and bail out
//

void DisplayPicture(const char* path)
{
OGLSetupInputType	viewDef;
float	timeout = 40.0f;



			/* SETUP VIEW */
			
	OGL_NewViewDef(&viewDef);	

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

	OGL_SetupWindow(&viewDef);



			/* CREATE BACKGROUND OBJECT */

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, 0, path);
	if (!gBackgoundPicture)
		DoFatalAlert("DisplayPicture: MO_CreateNewObjectOfType failed");



		/***********/
		/* SHOW IT */
		/***********/
			
	
	ReadKeyboard();
	CalcFramesPerSecond();
	CalcFramesPerSecond();

	MakeFadeEvent(true);
		
					/* MAIN LOOP */
						
		while (1)
		{
			CalcFramesPerSecond();
			MoveObjects();
			OGL_DrawScene(DisplayPicture_Draw);
			
			ReadKeyboard();
			if (UserWantsOut() || GetNewClickState(1))
				break;
				
			timeout -= gFramesPerSecondFrac;
			if (timeout < 0.0f)
				break;
		}

			/* FADE OUT */

	OGL_FadeOutScene(DisplayPicture_Draw, MoveObjects);
		
	
			/* CLEANUP */
			
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);

	OGL_DisposeWindowSetup();	
}


/***************** DISPLAY PICTURE: DRAW *******************/

static void DisplayPicture_Draw(void)
{
	MO_DrawObject(gBackgoundPicture);
	DrawObjects();
}


#pragma mark -

/************** DO LEGAL SCREEN *********************/

void DoLegalScreen(void)
{
	DisplayPicture(":images:Logo.png");
}



#pragma mark -


/******************** MAKE DARKEN PANE **************************/

ObjNode *MakeDarkenPane(void)
{
ObjNode *pane;
		
	gNewObjectDefinition.genre		= CUSTOM_GENRE;		
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_DOUBLESIDED;									
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100;
	gNewObjectDefinition.moveCall 	= MoveDarkenPane;
	pane = MakeNewObject(&gNewObjectDefinition);

	pane->Mode = 0;							// make lighten
	
	pane->CustomDrawFunction = DrawDarkenPane;

	pane->ColorFilter.r = 0;
	pane->ColorFilter.g = 0;
	pane->ColorFilter.b = 0;
	pane->ColorFilter.a = 0;
	
	return(pane);
}


/********************* MOVE DARKEN PANE ******************************/

static void MoveDarkenPane(ObjNode *theNode)
{
	if (theNode->Mode == 0)
	{
		theNode->ColorFilter.a += gFramesPerSecondFrac * 1.0f;
		if (theNode->ColorFilter.a > .6f)
			theNode->ColorFilter.a = .6f;
	}
	else
	{
		theNode->ColorFilter.a -= gFramesPerSecondFrac * 1.0f;
		if (theNode->ColorFilter.a < 0.0f)
		{
			DeleteObject(theNode);
			return;
		}
	}
}



/********************** DRAW DARKEN PANE *****************************/

static void DrawDarkenPane(ObjNode *theNode)
{
	glDisable(GL_TEXTURE_2D);
	SetColor4fv((GLfloat *)&theNode->ColorFilter);
	glEnable(GL_BLEND);

	glBegin(GL_QUADS);				
	glVertex3f(-1000,-1000,DARKEN_PANE_Z);
	glVertex3f(1000,-1000,DARKEN_PANE_Z);
	glVertex3f(1000,1000,DARKEN_PANE_Z);
	glVertex3f(-1000,1000,DARKEN_PANE_Z);
	glEnd();	

	glDisable(GL_BLEND);
}








