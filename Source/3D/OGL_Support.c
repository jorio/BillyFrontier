/****************************/
/*   OPENGL SUPPORT.C	    */
/*   By Brian Greenstone    */
/* (c)2002 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

#include "stb_image.h"
#include "ogl_functions.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void OGL_CreateDrawContext(void);
static void OGL_DisposeDrawContext(void);
static void OGL_InitDrawContext(OGLViewDefType *viewDefPtr);
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

/*********************/
/*    VARIABLES      */
/*********************/

float					g2DLogicalWidth = 640;
float					g2DLogicalHeight = 480;

		/* THE ANAGLYPH SCALE FACTOR */
		//
		// This changes the scale of the focal length and eye separation below.
		// When > 1.0 the scene will look more toy-like and more 3D, but higher distortion of
		// objects up close.  If < 1.0 the scene will be less distorted, but less 3D and objects seem larger.
		//
		
float					gAnaglyphFocallength	= 200.0f;
float					gAnaglyphEyeSeparation 	= 25.0f;
Byte					gAnaglyphPass;
static Byte				gAnaglyphGreyTable[255];


static	Boolean					gDoAnisotropy 			= true;
static	float 					gMaxAnisotropy = 1.0;

SDL_GLContext			gAGLContext = nil;


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

Boolean		gMyState_Lighting;

		/* PICKING */
		
Boolean		gIsPicking = false;


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


		/* CREATE DRAW CONTEXT THAT WILL BE USED THROUGHOUT THE GAME */

	OGL_CreateDrawContext();
}


/******************** OGL SHUTDOWN *****************/

void OGL_Shutdown(void)
{
	OGL_DisposeDrawContext();
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

void OGL_SetupWindow(OGLSetupInputType *setupDefPtr)
{
	GAME_ASSERT_MESSAGE(gGameViewInfoPtr == NULL, "gGameViewInfoPtr is already active");

			/* ALLOC MEMORY FOR OUTPUT DATA */

	gGameViewInfoPtr = (OGLSetupOutputType *) AllocPtr(sizeof(OGLSetupOutputType));
	if (gGameViewInfoPtr == nil)
		DoFatalAlert("OGL_SetupWindow: AllocPtr failed");


				/* SETUP */

	OGL_InitDrawContext(&setupDefPtr->view);	
	OGL_SetStyles(setupDefPtr);	
	OGL_CreateLights(&setupDefPtr->lights);


				/* PASS BACK INFO */

//	gGameViewInfoPtr->drawContext 		= gAGLContext;
	gGameViewInfoPtr->clip 				= setupDefPtr->view.clip;
	gGameViewInfoPtr->hither 			= setupDefPtr->camera.hither;			// remember hither/yon
	gGameViewInfoPtr->yon 				= setupDefPtr->camera.yon;
	gGameViewInfoPtr->useFog 			= setupDefPtr->styles.useFog;
	gGameViewInfoPtr->clearBackBuffer 	= setupDefPtr->view.clearBackBuffer;
	
	gGameViewInfoPtr->isActive = true;											// it's now an active structure
	
	gGameViewInfoPtr->lightList = setupDefPtr->lights;							// copy lights

	gGameViewInfoPtr->fov = setupDefPtr->camera.fov;					// each camera will have its own fov so we can change it for special effects
	OGL_UpdateCameraFromTo(&setupDefPtr->camera.from, &setupDefPtr->camera.to);		
}


/***************** OGL_DisposeWindowSetup ***********************/
//
// Disposes of all data created by OGL_SetupWindow
//

void OGL_DisposeWindowSetup(void)
{
	GAME_ASSERT(gGameViewInfoPtr);						// see if this setup exists

			/* KILL DEBUG FONT */

	OGL_FreeFont();

		/* FREE MEMORY & NIL POINTER */

	gGameViewInfoPtr->isActive = false;									// now inactive
	SafeDisposePtr((Ptr) gGameViewInfoPtr);
	gGameViewInfoPtr = nil;
}




/**************** OGL: CREATE DRAW CONTEXT *********************/

static void OGL_CreateDrawContext(void)
{
	GAME_ASSERT_MESSAGE(!gAGLContext, "GL context already exists");
	GAME_ASSERT_MESSAGE(gSDLWindow, "Window must be created before the DC!");

			/* CREATE AGL CONTEXT & ATTACH TO WINDOW */

	gAGLContext = SDL_GL_CreateContext(gSDLWindow);

	if (!gAGLContext)
		DoFatalAlert(SDL_GetError());

	GAME_ASSERT(glGetError() == GL_NO_ERROR);


			/* ACTIVATE CONTEXT */

	bool mkc = SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);
	GAME_ASSERT_MESSAGE(mkc, SDL_GetError());

			/* ENABLE VSYNC */

	SDL_GL_SetSwapInterval(1);


			/* GET OPENGL EXTENSIONS */
			//
			// On Mac/Linux, we only need to do this once.
			// But on Windows, we must do it whenever we create a draw context.
			//

	OGL_InitFunctions();


			/* SEE IF SUPPORT 512X512 */

	GLint			maxTexSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	GAME_ASSERT_MESSAGE(maxTexSize >= 512, "Your graphics card doesn't support 512x512 textures.");
}

/**************** OGL: NUKE DRAW CONTEXT *********************/
//
// Do this when QUITTING the game!
// The game reuses the same draw context for all scenes!
//

static void OGL_DisposeDrawContext(void)
{
	if (!gAGLContext)
	{
		return;
	}

	SDL_GL_MakeCurrent(gSDLWindow, NULL);		// make context not current
	SDL_GL_DestroyContext(gAGLContext);			// nuke context
	gAGLContext = nil;
}

/**************** OGL: INIT DRAW CONTEXT *********************/

static void OGL_InitDrawContext(OGLViewDefType* viewDefPtr)
{
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

	GAME_ASSERT(!OGL_CheckError());
}



/**************** OGL: SET STYLES ****************/

static void OGL_SetStyles(OGLSetupInputType *setupDefPtr)
{
OGLStyleDefType *styleDefPtr = &setupDefPtr->styles;


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
		glFogfv(GL_FOG_COLOR, &setupDefPtr->view.clearColor.r);
		glEnable(GL_FOG);
	}
	else
		glDisable(GL_FOG);

		/* ANISOTRIPIC FILTERING */

	if (gDoAnisotropy)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gMaxAnisotropy);
		OGL_CheckError();
	}

	OGL_CheckError();
}




/********************* OGL: CREATE LIGHTS ************************/
//
// NOTE:  The Projection matrix must be the identity or lights will be transformed.
//

static void OGL_CreateLights(OGLLightDefType *lightDefPtr)
{
GLfloat	ambient[4];

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
			
	for (int i = 0; i < lightDefPtr->numFillLights; i++)
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


			/* KILL OTHER LIGHTS THAT MIGHT STILL BE ACTIVE FROM PREVIOUS SCENE */

	for (int i = lightDefPtr->numFillLights; i < MAX_FILL_LIGHTS; i++)
	{
		glDisable(GL_LIGHT0 + i);
	}
	
}

#pragma mark -

/******************* OGL DRAW SCENE *********************/

void OGL_DrawScene(void (*drawRoutine)(void))
{
	GAME_ASSERT(gGameViewInfoPtr);							// make sure it's legit
	GAME_ASSERT(gGameViewInfoPtr->isActive);

	SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);			// make context active


			/* INIT SOME STUFF */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphPass = 0;
		PrepAnaglyphCameras();
	}


	if (gDebugMode)
	{
		gVRAMUsedThisFrame = gGameWindowWidth * gGameWindowHeight * (/*gGamePrefs.depth*/32 / 8);				// backbuffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * 2;										// z-buffer size
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
				

	if (gGameViewInfoPtr->clearBackBuffer || (gDebugMode == 3))
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

	OGL_Camera_SetPlacementAndUpdateMatrices();
	OGL_CheckError();


			/* CALL INPUT DRAW FUNCTION */

	if (drawRoutine != nil)
		drawRoutine();

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
		
	if (GetNewKeyState(SDL_SCANCODE_F8))
	{
		if (++gDebugMode > 3)
			gDebugMode = 0;
			
		if (gDebugMode == 3)								// see if show wireframe
			glPolygonMode(GL_FRONT_AND_BACK ,GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK ,GL_FILL);
	}

	if ((GetKeyState(SDL_SCANCODE_LCTRL) || GetKeyState(SDL_SCANCODE_RCTRL)) && GetNewKeyState(SDL_SCANCODE_F11))	// Anisotropy
	{
		gDoAnisotropy = !gDoAnisotropy;	
		if (gDoAnisotropy)
		{
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gMaxAnisotropy);
			OGL_CheckError();
		}
		else
			gMaxAnisotropy = 1;
	
		SDL_Log("Anisotropic filtering: %f", gMaxAnisotropy);
	}

				/* SHOW BASIC DEBUG INFO */

	if (gDebugMode > 0)
	{
		int		y = 100;

		OGL_DrawString("fps:", 20,y);
		OGL_DrawInt(gFramesPerSecond+.5f, 100,y);
		y += 15;

		OGL_DrawString("tris:", 20,y);
		OGL_DrawInt(gPolysThisFrame, 100,y);
		y += 15;


#if 1							// show supertile status grid
		{
			int	row, col;
			float	x = 20;
			
			for (row = 0; row < gNumSuperTilesDeep; row++)
			{
				for (col = 0; col < gNumSuperTilesWide; col++)
				{
					if (gSuperTileStatusGrid[row][col].playerHereFlag)
						OGL_DrawString("o", x,y);
					else
						OGL_DrawString(".", x,y);
					x += 5.0f;
				}
				x = 20;
				y += 5.0f;
			}

			y += 15;
		}
#endif

		OGL_DrawString("ter y:", 20,y);
		OGL_DrawInt((int) GetTerrainY(gPlayerInfo.coord.x, gPlayerInfo.coord.z), 100,y);
		y += 15;

		OGL_DrawString("vram kb:", 20,y);
		OGL_DrawInt(gVRAMUsedThisFrame/1024, 100,y);
		y += 15;

		OGL_DrawString("sparkles:", 20,y);
		OGL_DrawInt(gNumSparkles, 100,y);
		y += 15;

		if (gPlayerInfo.objNode)
		{
			OGL_DrawString("ground", 20,y);
			if (gPlayerInfo.objNode->StatusBits & STATUS_BIT_ONGROUND)
				OGL_DrawString("Y", 100,y);
			else
				OGL_DrawString("N", 100,y);
			y += 15;
		}

		OGL_DrawString("water:", 20,y);
		OGL_DrawInt(gNumWaterDrawn, 100,y);
		y += 15;

		OGL_DrawString("pointers:", 20,y);
		OGL_DrawInt(gNumPointers, 100,y);
		y += 15;
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

void OGL_GetCurrentViewport(int *x, int *y, int *w, int *h)
{
	SDL_GetWindowSizeInPixels(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);

int	t,b,l,r;
		
	t = gGameViewInfoPtr->clip.top;
	b = gGameViewInfoPtr->clip.bottom;
	l = gGameViewInfoPtr->clip.left;
	r = gGameViewInfoPtr->clip.right;
				
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

	pixelData = (uint8_t*) stbi_load_from_memory((const stbi_uc*) imageFileData, (int) imageFileLength, &width, &height, NULL, 4);
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

	SafeDisposePtr((Ptr) pixelData);

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
uint32_t	a,q;

	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		uint32_t	*pix32 = (uint32_t *)imageMemory;
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
		uint32_t	*pix32 = (uint32_t *)imageMemory;
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
		uint16_t	*pix16 = (uint16_t *)imageMemory;
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

static void ColorBalanceRGBForAnaglyph(uint32_t *rr, uint32_t *gg, uint32_t *bb)
{
uint32_t	r,g,b;
int	d;

	r = *rr;
	g = *gg;
	b = *bb;


			/* SEE IF GREEN IS OVER SATURATED */
			
	d = g - (r+b)/2;
	if (d > 0)
	{
		d = d * 2/3;
		if ((long)r < d)
			r = d;

		if ((long)b < d)
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
		if ((long)b < d)
		{
			b = d;
		}
	}	


			/* SEE IF BLUE IS OVER SATURATED */
			
	d = b - r;											// calc diff between blue and red
	if (d > 0)
	{
		d = d * 3/5;										// divide by n to get min red
		if ((long)r < d)
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
uint32_t	r,g,b;
uint32_t	a;

	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		uint32_t	*pix32 = (uint32_t *)imageMemory;
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
		uint32_t	*pix32 = (uint32_t *)imageMemory;
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
		uint16_t	*pix16 = (uint16_t *)imageMemory;
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

void OGL_MoveCameraFromTo(float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ)
{

			/* SET CAMERA COORDS */
			
	gGameViewInfoPtr->cameraPlacement.cameraLocation.x += fromDX;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.y += fromDY;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.z += fromDZ;

	gGameViewInfoPtr->cameraPlacement.pointOfInterest.x += toDX;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.y += toDY;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.z += toDZ;

	UpdateListenerLocation();
}


/*************** OGL_MoveCameraFrom ***************/

void OGL_MoveCameraFrom(float fromDX, float fromDY, float fromDZ)
{

			/* SET CAMERA COORDS */
			
	gGameViewInfoPtr->cameraPlacement.cameraLocation.x += fromDX;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.y += fromDY;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.z += fromDZ;

	UpdateListenerLocation();
}



/*************** OGL_UpdateCameraFromTo ***************/

void OGL_UpdateCameraFromTo(const OGLPoint3D *from, const OGLPoint3D *to)
{
static const OGLVector3D up = {0,1,0};
	
	gGameViewInfoPtr->cameraPlacement.upVector 				= up;
		
	if (from)
		gGameViewInfoPtr->cameraPlacement.cameraLocation 	= *from;
		
	if (to)
		gGameViewInfoPtr->cameraPlacement.pointOfInterest 	= *to;
	
	UpdateListenerLocation();
}

/*************** OGL_UpdateCameraFromToUp ***************/

void OGL_UpdateCameraFromToUp(const OGLPoint3D *from, const OGLPoint3D *to, const OGLVector3D *up)
{
	gGameViewInfoPtr->cameraPlacement.upVector 		= *up;
	gGameViewInfoPtr->cameraPlacement.cameraLocation 	= *from;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest 	= *to;

	UpdateListenerLocation();
}



/************** OGL: CAMERA SET PLACEMENT & UPDATE MATRICES **********************/
//
// This is called by OGL_DrawScene to initialize all of the view matrices,
// and to extract the current view matrices used for culling et.al.
//

void OGL_Camera_SetPlacementAndUpdateMatrices(void)
{
int		w, h, x, y;
OGLLightDefType	*lights;

				/* SET VIEWPORT */

	OGL_GetCurrentViewport(&x, &y, &w, &h);
	glViewport(x,y, w, h);
	gCurrentAspectRatio = (float)w/(float)h;
	
				/* COMPUTE LOGICAL 2D WIDTH/HEIGHT FOR UI ELEMENTS */

	g2DLogicalHeight = 480.0f;
	if (gCurrentAspectRatio < 4.0f / 3.0f)
		g2DLogicalWidth = 640.0f;
	else
		g2DLogicalWidth = 480.0f * gCurrentAspectRatio;
	
			/* INIT PROJECTION MATRIX */
			


			/* SETUP FOR ANAGLYPH STEREO 3D CAMERA */
			
	if (gGamePrefs.anaglyph)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		float	left, right;
		float	halfFOV = gGameViewInfoPtr->fov * .5f;
		float	hither 	= gGameViewInfoPtr->hither;
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
		
		glFrustum(left, right, -wd2, wd2, gGameViewInfoPtr->hither, gGameViewInfoPtr->yon);

		glGetFloatv(GL_PROJECTION_MATRIX, gViewToFrustumMatrix.value);
	}
	
			/* SETUP STANDARD PERSPECTIVE CAMERA */
	else
	{	
		OGL_SetGluPerspectiveMatrix(
			&gViewToFrustumMatrix,		// projection
			gGameViewInfoPtr->fov,		// our version uses radians for the fov (unlike GLU)
			gCurrentAspectRatio,
			gGameViewInfoPtr->hither,
			gGameViewInfoPtr->yon);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(gViewToFrustumMatrix.value);
	}
	
	
			/* INIT MODELVIEW MATRIX */
			
	OGL_SetGluLookAtMatrix(
			&gWorldToViewMatrix,		// modelview
			&gGameViewInfoPtr->cameraPlacement.cameraLocation,
			&gGameViewInfoPtr->cameraPlacement.pointOfInterest,
			&gGameViewInfoPtr->cameraPlacement.upVector);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(gWorldToViewMatrix.value);


		/* UPDATE LIGHT POSITIONS */

	lights =  &gGameViewInfoPtr->lightList;					// point to light list
	for (int i = 0; i < lights->numFillLights; i++)
	{
		GLfloat lightVec[4];
	
		lightVec[0] = -lights->fillDirection[i].x;			// negate vector because OGL is stupid
		lightVec[1] = -lights->fillDirection[i].y;
		lightVec[2] = -lights->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);	
	}


			/* GET VARIOUS CAMERA MATRICES */

	//glGetFloatv(GL_MODELVIEW_MATRIX, gWorldToViewMatrix.value);
	//glGetFloatv(GL_PROJECTION_MATRIX, gViewToFrustumMatrix.value);
	OGLMatrix4x4_Multiply(&gWorldToViewMatrix, &gViewToFrustumMatrix, &gWorldToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(&gFrustumToWindowMatrix);
	OGLMatrix4x4_Multiply(&gWorldToFrustumMatrix, &gFrustumToWindowMatrix, &gWorldToWindowMatrix);

	UpdateListenerLocation();
	OGL_CheckError();
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
	gMyState_Lighting = true;
	glEnable(GL_LIGHTING);	
}

/******************* OGL DISABLE LIGHTING ****************************/

void OGL_DisableLighting(void)
{
	gMyState_Lighting = false;
	glDisable(GL_LIGHTING);	
}


#pragma mark -

/************************** OGL_INIT FONT **************************/

static void OGL_InitFont(void)
{
}


/******************* OGL_FREE FONT ***********************/

static void OGL_FreeFont(void)
{
}

/**************** OGL_DRAW STRING ********************/

void OGL_DrawString(const char* s, GLint x, GLint y)
{
	OGL_PushState();

	SetInfobarSpriteState(0);
	
	glDisable(GL_LIGHTING);
	
	glEnable(GL_TEXTURE_2D);

	DrawFontString(s, x, y, 15.0f, false);
	
	OGL_PopState();
}

/**************** OGL_DRAW FLOAT ********************/

void OGL_DrawFloat(float f, GLint x, GLint y)
{
	char s[16];

	SDL_snprintf(s, sizeof(s), "%f", f);
	OGL_DrawString(s, x, y);
}



/**************** OGL_DRAW INT ********************/

void OGL_DrawInt(int f, GLint x, GLint y)
{
	char s[16];

	SDL_snprintf(s, sizeof(s), "%d", f);
	OGL_DrawString(s, x, y);
}


#pragma mark - "2D viewport with 640x480-ish logical size"


#define logicalXOffset (0.5f * (g2DLogicalWidth - 640.0f))

void OGL_Ortho2DLogicalSize(void)
{
	glOrtho(-logicalXOffset, 640 + logicalXOffset, 480, 0, 0, 1);
}

OGLPoint2D WindowPointToLogical(OGLPoint2D windowPoint)
{
	float fx = g2DLogicalWidth / gGameWindowWidth;
	float fy = g2DLogicalHeight / gGameWindowHeight;

	return (OGLPoint2D)
	{
		fx * windowPoint.x - logicalXOffset,
		fy * windowPoint.y
	};
}

OGLPoint2D LogicalPointToWindow(OGLPoint2D logicalPoint)
{
	float fx = gGameWindowWidth / g2DLogicalWidth;
	float fy = gGameWindowHeight / g2DLogicalHeight;

	return (OGLPoint2D)
	{
		fx * (logicalPoint.x + logicalXOffset),
		fy * logicalPoint.y
	};
}
