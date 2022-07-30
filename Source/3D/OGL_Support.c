/****************************/
/*   OPENGL SUPPORT.C	    */
/*   By Brian Greenstone    */
/* (c)2002 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/

#define aglGetError() GAME_ASSERT(!OGL_CheckError())
#define aglSetCurrentContext(junk) IMPLEMENT_ME_SOFT()

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <string.h>

#include "stb_image.h"
#include "ogl_functions.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr);
static void OGL_SetStyles(OGLSetupInputType *setupDefPtr);
static void OGL_CreateLights(OGLLightDefType *lightDefPtr);
static void OGL_InitFont(void);
static void OGL_FreeFont(void);

static void	ConvertTextureToColorAnaglyph(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType);
static void	ConvertTextureToGrey(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	STATE_STACK_SIZE	20

#define	PICK_BUFFER_SIZE	100

/*********************/
/*    VARIABLES      */
/*********************/

		/* THE ANAGLYPH SCALE FACTOR */
		//
		// This changes the scale of the focal length and eye separation below.
		// When > 1.0 the scene will look more toy-like and more 3D, but higher distortion of
		// objects up close.  If < 1.0 the scene will be less distorted, but less 3D and objects seem larger.
		//
		
float					gAnaglyphFocallength	= 200.0f;
float					gAnaglyphEyeSeparation 	= 25.0f;
Byte					gAnaglyphPass;
u_char					gAnaglyphGreyTable[255];


Boolean							gATIDriver				= false;
static	Boolean					gDoAnisotropy 			= true;
static	float 					gMaxAnisotropy = 1.0;

typedef void(*glPNTrianglesiATIXFUNC)(GLenum pname, GLint param);
static glPNTrianglesiATIXFUNC 	ptrTo_glPNTrianglesiATIX= nil;


SDL_GLContext		gAGLContext = nil;

static GLuint 			gFontList;


OGLMatrix4x4	gViewToFrustumMatrix,gWorldToViewMatrix,gWorldToFrustumMatrix;
OGLMatrix4x4	gWorldToWindowMatrix,gFrustumToWindowMatrix;

float	gCurrentAspectRatio = 1;


Boolean		gStateStack_Lighting[STATE_STACK_SIZE];					
Boolean		gStateStack_CullFace[STATE_STACK_SIZE];					
Boolean		gStateStack_DepthTest[STATE_STACK_SIZE];			
Boolean		gStateStack_Normalize[STATE_STACK_SIZE];
Boolean		gStateStack_Texture2D[STATE_STACK_SIZE];
Boolean		gStateStack_Blend[STATE_STACK_SIZE];
Boolean		gStateStack_Fog[STATE_STACK_SIZE];
GLboolean	gStateStack_DepthMask[STATE_STACK_SIZE];
GLint		gStateStack_BlendDst[STATE_STACK_SIZE];
GLint		gStateStack_BlendSrc[STATE_STACK_SIZE];
GLfloat		gStateStack_Color[STATE_STACK_SIZE][4];

int			gStateStackIndex = 0;

int			gPolysThisFrame;
int			gVRAMUsedThisFrame = 0;
static int			gMinRAM = 10000000000000;

Boolean		gMyState_Lighting;

		/* PICKING */
		
Boolean		gIsPicking = false;
GLuint		gPickBuffer[PICK_BUFFER_SIZE];
int			gNumPickHits = 0;


/******************** OGL BOOT *****************/
//
// Initialize my OpenGL stuff.
//

void OGL_Boot(void)
{
short	i;
float	f;

		/* GENERATE ANAGLYPH GREY CONVERSION TABLE */
		
	f = 0;
	for (i = 0; i < 255; i++)
	{
		gAnaglyphGreyTable[i] = sin(f) * 255.0f;
		f += (PI/2.0) / 255.0f;
	}
}


/*********************** OGL: NEW VIEW DEF **********************/
//
// fills a view def structure with default values.
//

void OGL_NewViewDef(OGLSetupInputType *viewDef)
{
const OGLColorRGBA		clearColor = {0,0,0,1};
const OGLPoint3D			cameraFrom = { 0, 0, 0.0 };
const OGLPoint3D			cameraTo = { 0, 0, -1 };
const OGLVector3D			cameraUp = { 0.0, 1.0, 0.0 };
const OGLColorRGBA			ambientColor = { .3, .3, .3, 1 };
const OGLColorRGBA			fillColor = { 1.0, 1.0, 1.0, 1 };
static OGLVector3D			fillDirection1 = { 1, 0, -1 };
static OGLVector3D			fillDirection2 = { -1, -.3, -.3 };


	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

	viewDef->view.clearColor 		= clearColor;
	viewDef->view.clip.left 	= 0;
	viewDef->view.clip.right 	= 0;
	viewDef->view.clip.top 		= 0;
	viewDef->view.clip.bottom 	= 0;
	viewDef->view.clearBackBuffer = true;

	viewDef->camera.from			= cameraFrom;
	viewDef->camera.to 				= cameraTo;
	viewDef->camera.up 				= cameraUp;
	viewDef->camera.hither 			= 10;
	viewDef->camera.yon 			= 4000;
	viewDef->camera.fov 			= 1.1;
	
	viewDef->styles.useFog			= false;
	viewDef->styles.fogStart		= viewDef->camera.yon * .5f;
	viewDef->styles.fogEnd			= viewDef->camera.yon;
	viewDef->styles.fogDensity		= 1.0;
	viewDef->styles.fogMode			= GL_LINEAR;

	viewDef->lights.ambientColor 	= ambientColor;
	viewDef->lights.numFillLights 	= 1;
	viewDef->lights.fillDirection[0] = fillDirection1;
	viewDef->lights.fillDirection[1] = fillDirection2;
	viewDef->lights.fillColor[0] 	= fillColor;
	viewDef->lights.fillColor[1] 	= fillColor;
}


/************** SETUP OGL WINDOW *******************/

void OGL_SetupWindow(OGLSetupInputType *setupDefPtr, OGLSetupOutputType **outputHandle)
{
static OGLVector3D	v = {0,0,0};
OGLSetupOutputType	*outputPtr;

	HideCursor();		// do this just as a safety precaution to make sure no cursor lingering around

			/* ALLOC MEMORY FOR OUTPUT DATA */

	outputPtr = (OGLSetupOutputType *)AllocPtr(sizeof(OGLSetupOutputType));
	if (outputPtr == nil)
		DoFatalAlert("OGL_SetupWindow: AllocPtr failed");


				/* SETUP */

	OGL_CreateDrawContext(&setupDefPtr->view);	
	OGL_SetStyles(setupDefPtr);	
	OGL_CreateLights(&setupDefPtr->lights);


				/* PASS BACK INFO */

//	outputPtr->drawContext 		= gAGLContext;
	outputPtr->clip 			= setupDefPtr->view.clip;
	outputPtr->hither 			= setupDefPtr->camera.hither;			// remember hither/yon
	outputPtr->yon 				= setupDefPtr->camera.yon;
	outputPtr->useFog 			= setupDefPtr->styles.useFog;
	outputPtr->clearBackBuffer 	= setupDefPtr->view.clearBackBuffer;
	
	outputPtr->isActive = true;											// it's now an active structure
	
	outputPtr->lightList = setupDefPtr->lights;							// copy lights

	outputPtr->fov = setupDefPtr->camera.fov;					// each camera will have its own fov so we can change it for special effects
	OGL_UpdateCameraFromTo(outputPtr, &setupDefPtr->camera.from, &setupDefPtr->camera.to);		
		
	*outputHandle = outputPtr;											// return value to caller
}


/***************** OGL_DisposeWindowSetup ***********************/
//
// Disposes of all data created by OGL_SetupWindow
//

void OGL_DisposeWindowSetup(OGLSetupOutputType **dataHandle)
{
OGLSetupOutputType	*data;

	data = *dataHandle;
	if (data == nil)												// see if this setup exists
		DoFatalAlert("OGL_DisposeWindowSetup: data == nil");
	
			/* KILL DEBUG FONT */

	OGL_FreeFont();

			/* NUKE GL CONTEXT */

	if (gAGLContext)
	{
		SDL_GL_MakeCurrent(gSDLWindow, NULL);		// make context not current
		SDL_GL_DeleteContext(gAGLContext);			// nuke context
		gAGLContext = nil;
	}

		/* FREE MEMORY & NIL POINTER */

	data->isActive = false;									// now inactive
	SafeDisposePtr((Ptr)data);
	*dataHandle = nil;
}




/**************** OGL: CREATE DRAW CONTEXT *********************/

static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr)
{
	GLint			maxTexSize;


	GAME_ASSERT_MESSAGE(gSDLWindow, "Window must be created before the DC!");

			/* CREATE AGL CONTEXT & ATTACH TO WINDOW */

	gAGLContext = SDL_GL_CreateContext(gSDLWindow);

	if (!gAGLContext)
		DoFatalAlert(SDL_GetError());

	GAME_ASSERT(glGetError() == GL_NO_ERROR);


			/* ACTIVATE CONTEXT */

	int mkc = SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);
	GAME_ASSERT_MESSAGE(mkc == 0, SDL_GetError());


			/* GET OPENGL EXTENSIONS */
			//
			// On Mac/Linux, we only need to do this once.
			// But on Windows, we must do it whenever we create a draw context.
			//

	OGL_InitFunctions();



			/* CLEAR ALL BUFFERS TO BLACK */
			
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);		// clear buffer
	SDL_GL_SwapWindow(gSDLWindow);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);		// clear buffer
	SDL_GL_SwapWindow(gSDLWindow);


				/* SET VARIOUS STATE INFO */


	glClearColor(viewDefPtr->clearColor.r, viewDefPtr->clearColor.g, viewDefPtr->clearColor.b, 1.0);
	glEnable(GL_DEPTH_TEST);								// use z-buffer
	
	{
		GLfloat	color[] = {1,1,1,1};									// set global material color to white
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
	}
	
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);	
	if (USE_GL_COLOR_MATERIAL)
		glEnable(GL_COLOR_MATERIAL);		
	else
		glDisable(GL_COLOR_MATERIAL);
  	glEnable(GL_NORMALIZE);


			/* INIT DEBUG FONT */

	OGL_InitFont();


			/* SEE IF SUPPORT 512X512 */

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	GAME_ASSERT_MESSAGE(maxTexSize >= 512, "Your graphics card doesn't support 512x512 textures.");

	GAME_ASSERT(!OGL_CheckError());
}



/**************** OGL: SET STYLES ****************/

static void OGL_SetStyles(OGLSetupInputType *setupDefPtr)
{
OGLStyleDefType *styleDefPtr = &setupDefPtr->styles;
SDL_GLContext agl_ctx = gAGLContext;


	glEnable(GL_CULL_FACE);									// activate culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);									// CCW is front face
//	glEnable(GL_DITHER);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// set default blend func
	glDisable(GL_BLEND);									// but turn it off by default

#if 0
	glHint(GL_TRANSFORM_HINT_APPLE, GL_FASTEST);
#endif
	glDisable(GL_RESCALE_NORMAL);

    glHint(GL_FOG_HINT, GL_NICEST);		// pixel accurate fog?

	glSelectBuffer(PICK_BUFFER_SIZE, gPickBuffer);				// set the selection buffer for picking


			/* ENABLE ALPHA CHANNELS */
			
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);	// draw any pixel who's Alpha != 0


		/* SET FOG */

	glHint(GL_FOG_HINT, GL_FASTEST);

	if (styleDefPtr->useFog)
	{
		glFogi(GL_FOG_MODE, styleDefPtr->fogMode);
		glFogf(GL_FOG_DENSITY, styleDefPtr->fogDensity);
		glFogf(GL_FOG_START, styleDefPtr->fogStart);
		glFogf(GL_FOG_END, styleDefPtr->fogEnd);
		glFogfv(GL_FOG_COLOR, (float *)&setupDefPtr->view.clearColor);
		glEnable(GL_FOG);
	}
	else
		glDisable(GL_FOG);

		/* ANISOTRIPIC FILTERING */

	if (gDoAnisotropy)
	{
#if 1
		IMPLEMENT_ME_SOFT();
#else
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gMaxAnisotropy);
		aglGetError();
#endif
	}

	OGL_CheckError();
}




/********************* OGL: CREATE LIGHTS ************************/
//
// NOTE:  The Projection matrix must be the identity or lights will be transformed.
//

static void OGL_CreateLights(OGLLightDefType *lightDefPtr)
{
int		i;
GLfloat	ambient[4];
SDL_GLContext agl_ctx = gAGLContext;

	OGL_EnableLighting();

	
			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	ambient[0] = lightDefPtr->ambientColor.r;
	ambient[1] = lightDefPtr->ambientColor.g;
	ambient[2] = lightDefPtr->ambientColor.b;
	ambient[3] = 1;
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);			// set scene ambient light


			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/
			
	for (i=0; i < lightDefPtr->numFillLights; i++)
	{
		static GLfloat lightamb[4] = { 0.0, 0.0, 0.0, 1.0 };
		GLfloat lightVec[4];
		GLfloat	diffuse[4];
	
					/* SET FILL DIRECTION */
					
		OGLVector3D_Normalize(&lightDefPtr->fillDirection[i], &lightDefPtr->fillDirection[i]);
		lightVec[0] = -lightDefPtr->fillDirection[i].x;		// negate vector because OGL is stupid
		lightVec[1] = -lightDefPtr->fillDirection[i].y;
		lightVec[2] = -lightDefPtr->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);

		
					/* SET COLOR */
					
		glLightfv(GL_LIGHT0+i, GL_AMBIENT, lightamb);
		
		diffuse[0] = lightDefPtr->fillColor[i].r;
		diffuse[1] = lightDefPtr->fillColor[i].g;
		diffuse[2] = lightDefPtr->fillColor[i].b;
		diffuse[3] = 1;
		
		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse);
	
			
		glEnable(GL_LIGHT0+i);								// enable the light
	}	
	
}

#pragma mark -

/******************* OGL PICK SCENE *************************/

void OGL_PickScene(OGLSetupOutputType *setupInfo, void (*drawRoutine)(OGLSetupOutputType *),
					float pickX, float pickY, float pickWidth, float pickHeight)
{
	if (setupInfo == nil)										// make sure it's legit
		DoFatalAlert("OGL_PickScene setupInfo == nil");
	if (!setupInfo->isActive)									
		DoFatalAlert("OGL_PickScene isActive == false");

	if (pickWidth < 1)
		pickWidth = 1;
	if (pickHeight < 1)
		pickHeight = 1;

  	aglSetCurrentContext(setupInfo->drawContext);				// make context current


			/* LET'S GET INTO PICKING MODE */

			
	gIsPicking 		= true;
	gNumPickHits 	= 0;	
	glRenderMode(GL_SELECT);
	glInitNames();
	gCurrentPickID 	= 0;
	puts("TODO: This glPushName is probably not going to work! We may be forcing a 64-bit pointer into a 32-bit int");
	glPushName(gCurrentPickID);



			/* GET UPDATED GLOBAL COPIES OF THE VARIOUS MATRICES */

	OGL_Camera_SetPlacementAndUpdateMatricesForPicking(setupInfo, pickX, pickY, pickWidth, pickHeight);


			/* CALL INPUT DRAW FUNCTION */

	if (drawRoutine != nil)
		drawRoutine(setupInfo);


		/* CLEANUP & GET OUT OF PICKING MODE */
			
	gNumPickHits = glRenderMode(GL_RENDER);
	gIsPicking = false;
	
//	OGL_Camera_SetPlacementAndUpdateMatrices(setupInfo);
}


/******************* OGL DRAW SCENE *********************/

void OGL_DrawScene(OGLSetupOutputType *setupInfo, void (*drawRoutine)(OGLSetupOutputType *))
{
//int	x,y,w,h;

	if (setupInfo == nil)										// make sure it's legit
		DoFatalAlert("OGL_DrawScene setupInfo == nil");
	if (!setupInfo->isActive)									
		DoFatalAlert("OGL_DrawScene isActive == false");

	SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);			// make context active


			/* INIT SOME STUFF */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphPass = 0;
		PrepAnaglyphCameras();
	}


	if (gDebugMode)
	{
		gVRAMUsedThisFrame = gGameWindowWidth * gGameWindowHeight * (gGamePrefs.depth / 8);				// backbuffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * 2;										// z-buffer size
		gVRAMUsedThisFrame += gGamePrefs.screenWidth * gGamePrefs.screenHeight * (gGamePrefs.depth / 8);	// display size
	}
			
			
	gPolysThisFrame 	= 0;										// init poly counter
	gMostRecentMaterial = nil;
	gGlobalMaterialFlags = 0;		
	gGlobalTransparency = 1.0f;	
	SetColor4f(1,1,1,1);


				/*****************/
				/* CLEAR BUFFERS */
				/*****************/

				/* MAKE SURE GREEN CHANNEL IS CLEAR */
				//
				// Bringing up dialogs can write into green channel, so always be sure it's clear
				//
				

	if (setupInfo->clearBackBuffer || (gDebugMode == 3))
	{
		if (gGamePrefs.anaglyph)
		{
			if (gGamePrefs.anaglyphColor)
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);		// make sure clearing Red/Green/Blue channels
			else
				glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);		// make sure clearing Red/Blue channels
		}
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	else
		glClear(GL_DEPTH_BUFFER_BIT);
	OGL_CheckError();

			/*************************/
			/* SEE IF DOING ANAGLYPH */
			/*************************/
			
do_anaglyph:

	if (gGamePrefs.anaglyph)
	{
				/* SET COLOR MASK */
				
		if (gAnaglyphPass == 0)
		{
			glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
		}
		else
		{
			if (gGamePrefs.anaglyphColor)
				glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);			
			else
				glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		
		
		CalcAnaglyphCameraOffset(gAnaglyphPass);		
	}



			/* GET UPDATED GLOBAL COPIES OF THE VARIOUS MATRICES */

	OGL_Camera_SetPlacementAndUpdateMatrices(setupInfo);
	OGL_CheckError();


			/* CALL INPUT DRAW FUNCTION */

	if (drawRoutine != nil)
		drawRoutine(setupInfo);

			/***********************************/	
			/* SEE IF DO ANOTHER ANAGLYPH PASS */
			/***********************************/	

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphPass++;
		if (gAnaglyphPass == 1)
			goto do_anaglyph;
	}



		/**************************/
		/* SEE IF SHOW DEBUG INFO */
		/**************************/
		
	if (GetNewKeyState_Real(KEY_F8))
	{
		if (++gDebugMode > 3)
			gDebugMode = 0;
			
		if (gDebugMode == 3)								// see if show wireframe
			glPolygonMode(GL_FRONT_AND_BACK ,GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK ,GL_FILL);
	}

#if 0
	if (gATIDriver)			// only if running on ATI
	{
		if (GetKeyState_Real(KEY_CTRL))
		{	
			if (GetNewKeyState_Real(KEY_F11))								// Anisotropy
			{
				gDoAnisotropy = !gDoAnisotropy;	
				if (gDoAnisotropy)
				{
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gMaxAnisotropy);
					aglGetError();
				}
				else
					gMaxAnisotropy = 1;		
			}			
		}
	}
#endif
					
				/* SHOW BASIC DEBUG INFO */

	if (gDebugMode > 0)
	{
#if 1
		IMPLEMENT_ME_SOFT();
#else
		int		y = 100;
		int		mem = FreeMem();
		
		if (mem < gMinRAM)		// poll for lowest RAM free
			gMinRAM = mem;
		
		
		OGL_DrawString("fps:", 20,y);
		OGL_DrawInt(gFramesPerSecond+.5f, 100,y);
		y += 15;

		OGL_DrawString("#tri:", 20,y);
		OGL_DrawInt(gPolysThisFrame, 100,y);
		y += 15;
#endif


#if 0							// show supertile status grid
		{
			int	row, col;
			float	x = 0;
			
			for (row = 0; row < gNumSuperTilesDeep; row++)
			{
				for (col = 0; col < gNumSuperTilesWide; col++)
				{
					if (gSuperTileStatusGrid[row][col].playerHereFlag)
						OGL_DrawString("X", x,y);
					else
						OGL_DrawString("O", x,y);
					x += 5.0f;
				}
				x = 0;
				y += 5.0f;
			}		
		}
#endif		

#if 0

		OGL_DrawString("#scratch:", 20,y);
		OGL_DrawInt(gScratch, 100,y);
		y += 15;		
		

		OGL_DrawString("input x:", 20,y);
		OGL_DrawFloat(gPlayerInfo.analogControlX, 100,y);
		y += 15;
		OGL_DrawString("input y:", 20,y);
		OGL_DrawFloat(gPlayerInfo.analogControlZ, 100,y);
		y += 15;

		OGL_DrawString("ter Y:", 20,y);
		OGL_DrawInt(GetTerrainY(gPlayerInfo.coord.x, gPlayerInfo.coord.z), 100,y);
		y += 15;		
		
		OGL_DrawString("#loopsfx:", 20,y);
		OGL_DrawInt(gNumLoopingEffects, 100,y);
		y += 15;

		OGL_DrawString("#free RAM:", 20,y);
		OGL_DrawInt(mem, 100,y);
		y += 15;

		OGL_DrawString("min RAM:", 20,y);
		OGL_DrawInt(gMinRAM, 100,y);
		y += 15;

		OGL_DrawString("used VRAM:", 20,y);
		OGL_DrawInt(gVRAMUsedThisFrame, 100,y);
		y += 15;

		OGL_DrawString("OGL Mem:", 20,y);
		OGL_DrawInt(glmGetInteger(GLM_CURRENT_MEMORY), 100,y);
		y += 15;


		OGL_DrawString("#sparkles:", 20,y);
		OGL_DrawInt(gNumSparkles, 100,y);
		y += 15;

		if (gPlayerInfo.objNode)
		{
			OGL_DrawString("ground?:", 20,y);
			if (gPlayerInfo.objNode->StatusBits & STATUS_BIT_ONGROUND)
				OGL_DrawString("Y", 100,y);
			else
				OGL_DrawString("N", 100,y);
			y += 15;
		}		


		OGL_DrawString("#H2O:", 20,y);
		OGL_DrawInt(gNumWaterDrawn, 100,y);
		y += 15;

		OGL_DrawString("#scratchI:", 20,y);
		OGL_DrawInt(gScratch, 100,y);
		y += 15;





//		OGL_DrawString("# pointers:", 20,y);
//		OGL_DrawInt(gNumPointers, 100,y);
//		y += 15;

#endif

	}		
			


            /**************/
			/* END RENDER */
			/**************/
           	           	
           	
           /* SWAP THE BUFFS */

	SDL_GL_SwapWindow(gSDLWindow);					// end render loop


	if (gGamePrefs.anaglyph)
		RestoreCamerasFromAnaglyph();


}


/********************** OGL: GET CURRENT VIEWPORT ********************/
//
// Remember that with OpenGL, the bottom of the screen is y==0, so some of this code
// may look upside down.
//

void OGL_GetCurrentViewport(const OGLSetupOutputType *setupInfo, int *x, int *y, int *w, int *h)
{
int	t,b,l,r;
		
	t = setupInfo->clip.top;
	b = setupInfo->clip.bottom;
	l = setupInfo->clip.left;
	r = setupInfo->clip.right;
				
	*x = l;
	*y = t;
	*w = gGameWindowWidth-l-r;
	*h = gGameWindowHeight-t-b;	
}


#pragma mark -


/***************** OGL TEXTUREMAP LOAD **************************/
 
GLuint OGL_TextureMap_Load(void *imageMemory, int width, int height,
							GLint srcFormat,  GLint destFormat, GLint dataType)
{	
GLuint	textureName;
SDL_GLContext agl_ctx = gAGLContext;
								
	if (gGamePrefs.anaglyph)
	{
		if (gGamePrefs.anaglyphColor)
			ConvertTextureToColorAnaglyph(imageMemory, width, height, srcFormat, dataType);
		else
			ConvertTextureToGrey(imageMemory, width, height, srcFormat, dataType);
	}
				
			/* GET A UNIQUE TEXTURE NAME & INITIALIZE IT */

	glGenTextures(1, &textureName);		
	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glGenTextures failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);				// this is now the currently active texture
	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glBindTexture failed!");
			
			
				/* LOAD TEXTURE AND/OR MIPMAPS */
				
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D,
					0,										// mipmap level
					destFormat,								// format in OpenGL
					width,									// width in pixels
					height,									// height in pixels
					0,										// border
					srcFormat,								// what my format is
					dataType,								// size of each r,g,b
					imageMemory);							// pointer to the actual texture pixels
	}

			/* SEE IF RAN OUT OF MEMORY WHILE COPYING TO OPENGL */

	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glTexImage2D failed!");


				/* SET THIS TEXTURE AS CURRENTLY ACTIVE FOR DRAWING */

	OGL_Texture_SetOpenGLTexture(textureName);

	return(textureName);
}


/***************** OGL TEXTUREMAP LOAD FROM PNG/JPG **********************/

GLuint OGL_TextureMap_LoadImageFile(const char* path, int* outWidth, int* outHeight)
{
uint8_t*				pixelData = nil;
int						width;
int						height;
long					imageFileLength = 0;
Ptr						imageFileData = nil;

				/* LOAD PICTURE FILE */

	imageFileData = LoadDataFile(path, &imageFileLength);
	GAME_ASSERT(imageFileData);

	pixelData = (uint8_t*) stbi_load_from_memory((const stbi_uc*) imageFileData, imageFileLength, &width, &height, NULL, 4);
	GAME_ASSERT(pixelData);

	SafeDisposePtr(imageFileData);
	imageFileData = NULL;

			/* PRE-PROCESS IMAGE */

	int internalFormat = GL_RGBA;

#if 0
	if (flags & kLoadTextureFlags_KeepOriginalAlpha)
	{
		internalFormat = GL_RGBA;
	}
	else
	{
		internalFormat = GL_RGB;
	}
#endif

			/* LOAD TEXTURE */

	GLuint glTextureName = OGL_TextureMap_Load(
			pixelData,
			width,
			height,
			GL_RGBA,
			internalFormat,
			GL_UNSIGNED_BYTE);
	OGL_CheckError();

			/* CLEAN UP */

	//DisposePtr((Ptr) pixelData);
	free(pixelData);  // TODO: define STBI_MALLOC/STBI_REALLOC/STBI_FREE in stb_image.c?

	if (outWidth)
		*outWidth = width;
	if (outHeight)
		*outHeight = height;

	return glTextureName;
}


/******************** CONVERT TEXTURE TO GREY **********************/
//
// The NTSC luminance standard where grayscale = .299r + .587g + .114b
//

#define	ANAGLYPH_GREY_ADJUST	1.4

static void	ConvertTextureToGrey(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType)
{
long	x,y;
float	r,g,b;
u_long	a,q;

	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		u_long	*pix32 = (u_long *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{	
				
				r = (float)((pix32[x] >> 16) & 0xff) / 255.0f * .299f;
				g = (float)((pix32[x] >> 8) & 0xff) / 255.0f * .586f;
				b = (float)(pix32[x] & 0xff) / 255.0f * .114f;
				a = (pix32[x] >> 24) & 0xff;
				
				
				q = (r + g + b) * 255.0f;
				if (q > 0xff)
					q = 0xff;
				q = gAnaglyphGreyTable[q];
			
				pix32[x] = (a << 24) | (q << 16) | (q << 8) | q;
				
			}
			pix32 += width;
		}
	}
	
	else
	if ((dataType == GL_UNSIGNED_BYTE) && (srcFormat == GL_RGBA))
	{
		u_long	*pix32 = (u_long *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{	
				r = (float)((pix32[x] >> 24) & 0xff) / 255.0f * .299f;
				g = (float)((pix32[x] >> 16) & 0xff) / 255.0f * .586f;
				b = (float)((pix32[x] >> 8)  & 0xff) / 255.0f * .114f;
				a = pix32[x] & 0xff;
				
				q = (r + g + b) * 255.0f;
				if (q > 0xff)
					q = 0xff;
				q = gAnaglyphGreyTable[q];
				
				pix32[x] = (q << 24) | (q << 16) | (q << 8) | a;
				
			}
			pix32 += width;
		}
	}
	else
	if (dataType == GL_UNSIGNED_SHORT_1_5_5_5_REV)
	{
		u_short	*pix16 = (u_short *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{					
				r = (float)((pix16[x] >> 10) & 0x1f) / 31.0f * .299f;
				g = (float)((pix16[x] >> 5) & 0x1f) / 31.0f * .586f;
				b = (float)(pix16[x] & 0x1f) / 31.0f * .114f;
				a = pix16[x] & 0x8000;
				
				q = (r + g + b) * 255.0f;
				if (q > 0xff)
					q = 0xff;
				q = gAnaglyphGreyTable[q];

				q = (float)q / 8.0f;				
				if (q > 0x1f)
					q = 0x1f;
			
				pix16[x] = a | (q << 10) | (q << 5) | q;
				
			}
			pix16 += width;
		}
	}
	
	
}


/******************* COLOR BALANCE RGB FOR ANAGLYPH *********************/

static void ColorBalanceRGBForAnaglyph(u_long *rr, u_long *gg, u_long *bb)
{
u_long	r,g,b;
long	d;

	r = *rr;
	g = *gg;
	b = *bb;


			/* SEE IF GREEN IS OVER SATURATED */
			
	d = g - (r+b)/2;
	if (d > 0)
	{
		d = d * 2/3;
		if (r < d)
			r = d;

		if (b < d)
			b = d;			
	}	


#if 1
	
			/* LIMIT GREEN BLEED */
	
	d = g - 0xa0;		
	if (d > 0)
	{
		g -= d;
	
		r += d * 1/3;
		b += d / 3;
		
		if (r > 0xff)
			r = 0xff;
		if (b > 0xff)
			b = 0xff;
	}	
#endif	
	
	
	
			/* SEE IF RED IS OVER SATURATED */
			
	d = r - b;											// calc diff between red and blue
	if (d > 0)
	{
		d = d * 2 / 5;										// divide by n to get min blue
		if (b < d)
		{
			b = d;
		}
	}	


			/* SEE IF BLUE IS OVER SATURATED */
			
	d = b - r;											// calc diff between blue and red
	if (d > 0)
	{
		d = d * 3/5;										// divide by n to get min red
		if (r < d)
		{
			r = d;
		}
	}	


	
	*rr = r;
	*gg = g;
	*bb = b;	
}



/******************** CONVERT TEXTURE TO COLOR ANAGLYPH **********************/


static void	ConvertTextureToColorAnaglyph(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType)
{
long	x,y;
u_long	r,g,b;
u_long	a;

	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		u_long	*pix32 = (u_long *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{					
				a = ((pix32[x] >> 24) & 0xff);
				r = ((pix32[x] >> 16) & 0xff);
				g = ((pix32[x] >> 8) & 0xff);
				b = ((pix32[x] >> 0) & 0xff);
		
				ColorBalanceRGBForAnaglyph(&r, &g, &b);
										
				pix32[x] = (a << 24) | (r << 16) | (g << 8) | b;
				
			}
			pix32 += width;
		}
	}
	else
	if ((dataType == GL_UNSIGNED_BYTE) && (srcFormat == GL_RGBA))
	{
		u_long	*pix32 = (u_long *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{	
				a = ((pix32[x] >> 0) & 0xff);
				r = ((pix32[x] >> 24) & 0xff);
				g = ((pix32[x] >> 16) & 0xff);
				b = ((pix32[x] >> 8) & 0xff);
		
				ColorBalanceRGBForAnaglyph(&r, &g, &b);

				pix32[x] = (r << 24) | (g << 16) | (b << 8) | a;
							
			}
			pix32 += width;
		}
	}
	else
	if (dataType == GL_UNSIGNED_SHORT_1_5_5_5_REV)
	{
		u_short	*pix16 = (u_short *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{					
				r = ((pix16[x] >> 10) & 0x1f) << 3;			// load 5 bits per channel & convert to 8 bits
				g = ((pix16[x] >> 5) & 0x1f) << 3;
				b = (pix16[x] & 0x1f) << 3;
				a = pix16[x] & 0x8000;

				ColorBalanceRGBForAnaglyph(&r, &g, &b);

				r >>= 3;
				g >>= 3;
				b >>= 3;
				
				pix16[x] = a | (r << 10) | (g << 5) | b;
				
			}
			pix16 += width;
		}
	}
	
}


/****************** OGL: TEXTURE SET OPENGL TEXTURE **************************/
//
// Sets the current OpenGL texture using glBindTexture et.al. so any textured triangles will use it.
//
 
void OGL_Texture_SetOpenGLTexture(GLuint textureName)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (OGL_CheckError())
		DoFatalAlert("OGL_Texture_SetOpenGLTexture: glPixelStorei failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);		
	if (OGL_CheckError())
		DoFatalAlert("OGL_Texture_SetOpenGLTexture: glBindTexture failed!");
	
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	// disable mipmaps & turn on filtering
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
 	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &gMaxAnisotropy);
	glGetError();
		
	glEnable(GL_TEXTURE_2D);
}



#pragma mark -

/*************** OGL_MoveCameraFromTo ***************/

void OGL_MoveCameraFromTo(OGLSetupOutputType *setupInfo, float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ)
{

			/* SET CAMERA COORDS */
			
	setupInfo->cameraPlacement.cameraLocation.x += fromDX;
	setupInfo->cameraPlacement.cameraLocation.y += fromDY;
	setupInfo->cameraPlacement.cameraLocation.z += fromDZ;

	setupInfo->cameraPlacement.pointOfInterest.x += toDX;
	setupInfo->cameraPlacement.pointOfInterest.y += toDY;
	setupInfo->cameraPlacement.pointOfInterest.z += toDZ;

	UpdateListenerLocation(setupInfo);
}


/*************** OGL_MoveCameraFrom ***************/

void OGL_MoveCameraFrom(OGLSetupOutputType *setupInfo, float fromDX, float fromDY, float fromDZ)
{

			/* SET CAMERA COORDS */
			
	setupInfo->cameraPlacement.cameraLocation.x += fromDX;
	setupInfo->cameraPlacement.cameraLocation.y += fromDY;
	setupInfo->cameraPlacement.cameraLocation.z += fromDZ;

	UpdateListenerLocation(setupInfo);
}



/*************** OGL_UpdateCameraFromTo ***************/

void OGL_UpdateCameraFromTo(OGLSetupOutputType *setupInfo, const OGLPoint3D *from, const OGLPoint3D *to)
{
static const OGLVector3D up = {0,1,0};
	
	setupInfo->cameraPlacement.upVector 			= up;
		
	if (from)
		setupInfo->cameraPlacement.cameraLocation 	= *from;
		
	if (to)
		setupInfo->cameraPlacement.pointOfInterest 	= *to;
	
	UpdateListenerLocation(setupInfo);
}

/*************** OGL_UpdateCameraFromToUp ***************/

void OGL_UpdateCameraFromToUp(OGLSetupOutputType *setupInfo, const OGLPoint3D *from, const OGLPoint3D *to, const OGLVector3D *up)
{			
			
	setupInfo->cameraPlacement.upVector 		= *up;
	setupInfo->cameraPlacement.cameraLocation 	= *from;
	setupInfo->cameraPlacement.pointOfInterest 	= *to;

	UpdateListenerLocation(setupInfo);
}



/************** OGL: CAMERA SET PLACEMENT & UPDATE MATRICES **********************/
//
// This is called by OGL_DrawScene to initialize all of the view matrices,
// and to extract the current view matrices used for culling et.al.
//

void OGL_Camera_SetPlacementAndUpdateMatrices(OGLSetupOutputType *setupInfo)
{
OGLCameraPlacement	*placement;
int		w, h, i, x, y;
OGLLightDefType	*lights;

				/* SET VIEWPORT */

	OGL_GetCurrentViewport(setupInfo, &x, &y, &w, &h);
	glViewport(x,y, w, h);
	gCurrentAspectRatio = (float)w/(float)h;
	
	
			/* INIT PROJECTION MATRIX */
			
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	

			/* SETUP FOR ANAGLYPH STEREO 3D CAMERA */
			
	if (gGamePrefs.anaglyph)
	{
		float	left, right;
		float	halfFOV = setupInfo->fov * .5f;
		float	hither 	= setupInfo->hither;
	   	float	wd2     = hither * tan(halfFOV);
		float	ndfl    = hither / gAnaglyphFocallength;
	
		if (gAnaglyphPass == 0)
		{
			left  = - gCurrentAspectRatio * wd2 + 0.5 * gAnaglyphEyeSeparation * ndfl;
			right =   gCurrentAspectRatio * wd2 + 0.5 * gAnaglyphEyeSeparation * ndfl;
		}
		else
		{
			left  = - gCurrentAspectRatio * wd2 - 0.5 * gAnaglyphEyeSeparation * ndfl;
			right =   gCurrentAspectRatio * wd2 - 0.5 * gAnaglyphEyeSeparation * ndfl;
		}
		
		glFrustum(left, right, -wd2, wd2, setupInfo->hither, setupInfo->yon);
	}
	
			/* SETUP STANDARD PERSPECTIVE CAMERA */
	else
	{	
		gluPerspective (OGLMath_RadiansToDegrees(setupInfo->fov),	// fov
						gCurrentAspectRatio,					// aspect
						setupInfo->hither,		// hither
						setupInfo->yon);		// yon
	}	
	
	
			/* INIT MODELVIEW MATRIX */
			
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	placement = &setupInfo->cameraPlacement;
	gluLookAt(placement->cameraLocation.x, placement->cameraLocation.y, placement->cameraLocation.z,	
			placement->pointOfInterest.x, placement->pointOfInterest.y, placement->pointOfInterest.z,	
			placement->upVector.x, placement->upVector.y, placement->upVector.z);


		/* UPDATE LIGHT POSITIONS */
		
	lights =  &setupInfo->lightList;						// point to light list
	for (i=0; i < lights->numFillLights; i++)
	{
		GLfloat lightVec[4];
	
		lightVec[0] = -lights->fillDirection[i].x;			// negate vector because OGL is stupid
		lightVec[1] = -lights->fillDirection[i].y;
		lightVec[2] = -lights->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);	
	}


			/* GET VARIOUS CAMERA MATRICES */
			
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&gWorldToViewMatrix);
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)&gViewToFrustumMatrix);
	OGLMatrix4x4_Multiply(&gWorldToViewMatrix, &gViewToFrustumMatrix, &gWorldToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(setupInfo, &gFrustumToWindowMatrix);
	OGLMatrix4x4_Multiply(&gWorldToFrustumMatrix, &gFrustumToWindowMatrix, &gWorldToWindowMatrix);

	UpdateListenerLocation(setupInfo);
	OGL_CheckError();
}

/************** OGL: CAMERA SET PLACEMENT & UPDATE MATRICES FOR PICKING **********************/
//
// This is called by OGL_PickScene to initialize all of the view matrices,
// and to extract the current view matrices used for culling et.al.
//

void OGL_Camera_SetPlacementAndUpdateMatricesForPicking(OGLSetupOutputType *setupInfo, float pickX, float pickY, float pickWidth, float pickHeight)
{
SDL_GLContext agl_ctx = gAGLContext;
OGLCameraPlacement	*placement;
int					x,y, w, h;
GLint				viewport[4];

	OGL_GetCurrentViewport(setupInfo, &x, &y, &w, &h);
	gCurrentAspectRatio = (float)w/(float)h;

			/* ADJUST PICK COORDS */
			
	pickY -= setupInfo->clip.bottom;				// adjust this for reasons I dont know, but it works
	
	pickX += pickWidth * .5f;						// make pick coords the center of the pick
	pickY += pickHeight * .5f;


	
			/* INIT PROJECTION MATRIX */
			
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	
	viewport[0] = x;
	viewport[1] = y;
	viewport[2] = w;
	viewport[3] = h;
	gluPickMatrix(pickX, h - pickY, pickWidth, pickHeight, viewport);		// limit picking to this area

	OGL_CheckError();
	
	gluPerspective (OGLMath_RadiansToDegrees(setupInfo->fov),	// fov
					gCurrentAspectRatio,	// aspect
					setupInfo->hither,		// hither
					setupInfo->yon);		// yon

	OGL_CheckError();
	
	
			/* INIT MODELVIEW MATRIX */
			
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	placement = &setupInfo->cameraPlacement;
	gluLookAt(placement->cameraLocation.x, placement->cameraLocation.y, placement->cameraLocation.z,	
			placement->pointOfInterest.x, placement->pointOfInterest.y, placement->pointOfInterest.z,	
			placement->upVector.x, placement->upVector.y, placement->upVector.z);


			/* SET CLIPPING TO OUR PANE */
			
	glViewport(0,0, w, h);


			/* GET VARIOUS CAMERA MATRICES */
			
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&gWorldToViewMatrix);
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)&gViewToFrustumMatrix);
	OGLMatrix4x4_Multiply(&gWorldToViewMatrix, &gViewToFrustumMatrix, &gWorldToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(setupInfo, &gFrustumToWindowMatrix);
	OGLMatrix4x4_Multiply(&gWorldToFrustumMatrix, &gFrustumToWindowMatrix, &gWorldToWindowMatrix);


}


#pragma mark -


/******************** OGL: CHECK ERROR ********************/

GLenum _OGL_CheckError(const char* file, const int line)
{
	GLenum error = glGetError();
	if (error != 0)
	{
		const char* text;
		switch (error)
		{
			case	GL_INVALID_ENUM:		text = "invalid enum"; break;
			case	GL_INVALID_VALUE:		text = "invalid value"; break;
			case	GL_INVALID_OPERATION:	text = "invalid operation"; break;
			case	GL_STACK_OVERFLOW:		text = "stack overflow"; break;
			case	GL_STACK_UNDERFLOW:		text = "stack underflow"; break;
			default:
				text = "";
		}

		DoFatalAlert("OpenGL error 0x%x (%s)\nin %s:%d", error, text, file, line);
	}
	return error;
}


#pragma mark -


/********************* PUSH STATE **************************/

void OGL_PushState(void)
{
int	i;
SDL_GLContext agl_ctx = gAGLContext;

		/* PUSH MATRIES WITH OPENGL */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);										// in my code, I keep modelview matrix as the currently active one all the time.
	

		/* SAVE OTHER INFO */
	
	i = gStateStackIndex++;											// get stack index and increment
	
	if (i >= STATE_STACK_SIZE)
		DoFatalAlert("OGL_PushState: stack overflow");
	
	gStateStack_Lighting[i] = gMyState_Lighting;					
	gStateStack_CullFace[i] = glIsEnabled(GL_CULL_FACE);					
	gStateStack_DepthTest[i] = glIsEnabled(GL_DEPTH_TEST);		
	gStateStack_Normalize[i] = glIsEnabled(GL_NORMALIZE);		
	gStateStack_Texture2D[i] = glIsEnabled(GL_TEXTURE_2D);		
	gStateStack_Fog[i] 		= glIsEnabled(GL_FOG);		
	gStateStack_Blend[i] 	= glIsEnabled(GL_BLEND);	
	
	glGetFloatv(GL_CURRENT_COLOR, &gStateStack_Color[i][0]);
	
	glGetIntegerv(GL_BLEND_SRC, &gStateStack_BlendSrc[i]);	
	glGetIntegerv(GL_BLEND_DST, &gStateStack_BlendDst[i]);	
	glGetBooleanv(GL_DEPTH_WRITEMASK, &gStateStack_DepthMask[i]);	
}


/********************* POP STATE **************************/

void OGL_PopState(void)
{
int		i;
SDL_GLContext agl_ctx = gAGLContext;

		/* RETREIVE OPENGL MATRICES */
		
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
		/* GET OTHER INFO */
		
	i = --gStateStackIndex;												// dec stack index
		
	if (i < 0)
		DoFatalAlert("OGL_PopState: stack underflow!");

	if (gStateStack_Lighting[i])
		OGL_EnableLighting();
	else
		OGL_DisableLighting();


	if (gStateStack_CullFace[i])
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);


	if (gStateStack_DepthTest[i])
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	if (gStateStack_Normalize[i])
		glEnable(GL_NORMALIZE);
	else
		glDisable(GL_NORMALIZE);

	if (gStateStack_Texture2D[i])
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);

	if (gStateStack_Blend[i])
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	if (gStateStack_Fog[i])
		glEnable(GL_FOG);
	else
		glDisable(GL_FOG);

	glDepthMask(gStateStack_DepthMask[i]);
	glBlendFunc(gStateStack_BlendSrc[i], gStateStack_BlendDst[i]);

	glColor4fv(&gStateStack_Color[i][0]);

}


/******************* OGL ENABLE LIGHTING ****************************/

void OGL_EnableLighting(void)
{
	SDL_GLContext agl_ctx = gAGLContext;

	gMyState_Lighting = true;
	glEnable(GL_LIGHTING);	
}

/******************* OGL DISABLE LIGHTING ****************************/

void OGL_DisableLighting(void)
{
	SDL_GLContext agl_ctx = gAGLContext;

	gMyState_Lighting = false;
	glDisable(GL_LIGHTING);	
}


#pragma mark -

/************************** OGL_INIT FONT **************************/

static void OGL_InitFont(void)
{
#if 1
	IMPLEMENT_ME_SOFT();
#else
	gFontList = glGenLists(256);
 
    if (!aglUseFont(gAGLContext, kFontIDMonaco, bold, 9, 0, 256, gFontList))
		DoFatalAlert("OGL_InitFont: aglUseFont failed");
#endif
}


/******************* OGL_FREE FONT ***********************/

static void OGL_FreeFont(void)
{

	SDL_GLContext agl_ctx = gAGLContext;
	glDeleteLists(gFontList, 256);
	
}

/**************** OGL_DRAW STRING ********************/

void OGL_DrawString(Str255 s, GLint x, GLint y)
{

	SDL_GLContext agl_ctx = gAGLContext;

	OGL_PushState();

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 0, 480, -10.0, 10.0);
	
	glDisable(GL_LIGHTING);
	
	glDisable(GL_TEXTURE_2D);
	glColor3f(1,1,1);
	glRasterPos2i(x, 480-y);

	glListBase(gFontList);
	glCallLists(s[0], GL_UNSIGNED_BYTE, &s[1]);
	
	OGL_PopState();
	
}

/**************** OGL_DRAW FLOAT ********************/

void OGL_DrawFloat(float f, GLint x, GLint y)
{

Str255	s;

	FloatToString(f,s);
	OGL_DrawString(s,x,y);
	
}



/**************** OGL_DRAW INT ********************/

void OGL_DrawInt(int f, GLint x, GLint y)
{

Str255	s;

	NumToString(f,s);
	OGL_DrawString(s,x,y);

}
