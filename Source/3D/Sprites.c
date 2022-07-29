/****************************/
/*   	SPRITES.C			*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "window.h"
#include "3dmath.h"
#include "infobar.h"


extern	float	gCurrentAspectRatio,gGlobalTransparency;
extern	int		gPolysThisFrame;
extern	Boolean			gSongPlayingFlag,gCanDo512,gLowMemMode,gMuteMusicFlag;
extern	u_long			gGlobalMaterialFlags;

/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/

#define	FONT_WIDTH	.51f



enum
{
	kMacRoman_Auml		= 0x80,
	kMacRoman_Aring,
	kMacRoman_Ccedil,
	kMacRoman_Eacute,
	kMacRoman_Ntilde,
	kMacRoman_Ouml,
	kMacRoman_Uuml,
	kMacRoman_aacute,
	kMacRoman_agrave,
	kMacRoman_acirc,
	kMacRoman_auml,
	kMacRoman_atilde,
	kMacRoman_aring,
	kMacRoman_ccedil,
	kMacRoman_eacute,
	kMacRoman_egrave,
	kMacRoman_ecirc,
	kMacRoman_euml,
	kMacRoman_iacute,
	kMacRoman_igrave,
	kMacRoman_icirc,
	kMacRoman_iuml,
	kMacRoman_ntilde,
	kMacRoman_oacute,
	kMacRoman_ograve,
	kMacRoman_ocirc,
	kMacRoman_ouml,
	kMacRoman_otilde,
	kMacRoman_uacute,
	kMacRoman_ugrave,
	kMacRoman_ucirc,
	kMacRoman_uuml,
	kMacRoman_szlig		= 0xA7,
	kMacRoman_iquest	= 0xC0,
	kMacRoman_iexcl		= 0xC1,
	kMacRoman_Agrave	= 0xCB,
	kMacRoman_Acirc		= 0xE5,
	kMacRoman_Ecirc,
	kMacRoman_Aacute,
	kMacRoman_Euml,
	kMacRoman_Egrave,
	kMacRoman_Iacute,
	kMacRoman_Icirc,
	kMacRoman_Iuml,
	kMacRoman_Igrave,
	kMacRoman_Oacute,
	kMacRoman_Ocirc,
	kMacRoman_VendorLogo,
	kMacRoman_Ograve,
	kMacRoman_Uacute,
	kMacRoman_Ucirc,
	kMacRoman_Ugrave,
};

/*********************/
/*    VARIABLES      */
/*********************/

SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
long		gNumSpritesInGroupList[MAX_SPRITE_GROUPS];		// note:  this must be long's since that's what we read from the sprite file!



/****************** INIT SPRITE MANAGER ***********************/

void InitSpriteManager(void)
{
int	i;

	for (i = 0; i < MAX_SPRITE_GROUPS; i++)
	{
		gSpriteGroupList[i] = nil;
		gNumSpritesInGroupList[i] = 0;
	}
}


/******************* DISPOSE ALL SPRITE GROUPS ****************/

void DisposeAllSpriteGroups(void)
{
int	i;

	for (i = 0; i < MAX_SPRITE_GROUPS; i++)
	{
		if (gSpriteGroupList[i])
			DisposeSpriteGroup(i);
	}
}


/************************** DISPOSE BG3D *****************************/

void DisposeSpriteGroup(int groupNum)
{
int 		i,n;

	n = gNumSpritesInGroupList[groupNum];						// get # sprites in this group
	if ((n == 0) || (gSpriteGroupList[groupNum] == nil))
		return;


			/* DISPOSE OF ALL LOADED OPENGL TEXTURENAMES */
			
	for (i = 0; i < n; i++)
		MO_DisposeObjectReference(gSpriteGroupList[groupNum][i].materialObject);
	
	
		/* DISPOSE OF GROUP'S ARRAY */
		
	SafeDisposePtr((Ptr)gSpriteGroupList[groupNum]);
	gSpriteGroupList[groupNum] = nil;
	gNumSpritesInGroupList[groupNum] = 0;
}



/********************** LOAD SPRITE FILE **************************/
//
// NOTE:  	All sprite files must be imported AFTER the draw context has been created,
//			because all imported textures are named with OpenGL and loaded into OpenGL!
//

void LoadSpriteFile(FSSpec *spec, int groupNum, OGLSetupOutputType *setupInfo)
{
short			refNum;
int				i,w,h;
long			count;
MOMaterialData	matData;


		/* OPEN THE FILE */

	if (FSpOpenDF(spec, fsRdPerm, &refNum) != noErr)
		DoFatalAlert("LoadSpriteFile: FSpOpenDF failed");

		/* READ # SPRITES IN THIS FILE */
		
	count = sizeof(long);
	FSRead(refNum, &count, &gNumSpritesInGroupList[groupNum]);


		/* ALLOCATE MEMORY FOR SPRITE RECORDS */
		
	gSpriteGroupList[groupNum] = (SpriteType *)AllocPtr(sizeof(SpriteType) * gNumSpritesInGroupList[groupNum]);
	if (gSpriteGroupList[groupNum] == nil)
		DoFatalAlert("LoadSpriteFile: AllocPtr failed");


			/********************/
			/* READ EACH SPRITE */
			/********************/

	for (i = 0; i < gNumSpritesInGroupList[groupNum]; i++)
	{
		int		bufferSize;
		u_char *buffer;
	
			/* READ WIDTH/HEIGHT, ASPECT RATIO */
			
		count = sizeof(int);								
		FSRead(refNum, &count, &gSpriteGroupList[groupNum][i].width);		
		count = sizeof(int);								
		FSRead(refNum, &count, &gSpriteGroupList[groupNum][i].height);		
		count = sizeof(float);								
		FSRead(refNum, &count, &gSpriteGroupList[groupNum][i].aspectRatio);		
	
	
			/* READ SRC FORMAT */
			
		count = sizeof(GLint);								
		FSRead(refNum, &count, &gSpriteGroupList[groupNum][i].srcFormat);		


			/* READ DEST FORMAT */
			
		count = sizeof(GLint);	
		FSRead(refNum, &count, &gSpriteGroupList[groupNum][i].destFormat);		


			/* READ BUFFER SIZE */
			
		count = sizeof(int);	
		FSRead(refNum, &count, &bufferSize);		
			
		buffer = AllocPtr(bufferSize);							// alloc memory for buffer
		if (buffer == nil)
			DoFatalAlert("LoadSpriteFile: AllocPtr failed");
		
	
			/* READ THE SPRITE PIXEL BUFFER */
			
		count = bufferSize;				
		FSRead(refNum, &count, buffer);
	
	

				/*****************************/
				/* CREATE NEW TEXTURE OBJECT */
				/*****************************/

		matData.setupInfo		= setupInfo;
		matData.flags			= BG3D_MATERIALFLAG_TEXTURED;
		matData.diffuseColor.r	= 1;
		matData.diffuseColor.g	= 1;
		matData.diffuseColor.b	= 1;
		matData.diffuseColor.a	= 1;
		
		matData.numMipmaps		= 1;
		w = matData.width		= gSpriteGroupList[groupNum][i].width;
		h = matData.height		= gSpriteGroupList[groupNum][i].height;
		
		matData.pixelSrcFormat	= gSpriteGroupList[groupNum][i].srcFormat;		
		matData.pixelDstFormat	= gSpriteGroupList[groupNum][i].destFormat;
		
		matData.texturePixels[0]= nil;											// we're going to preload
		
			/* SEE IF NEED TO SHRINK FOR VOODOO 2 */
						
		if ((w == 512) || (h == 512))
		{
			if (!gCanDo512)
			{
				if (matData.pixelSrcFormat == GL_RGB)
				{
					int		x,y;
					u_char	*src,*dest;
					
					dest = src = (u_char *)buffer;
					
					for (y = 0; y < h; y+=2)
					{
						for (x = 0; x < w; x+=2)
						{
							*dest++ = src[x*3];			
							*dest++ = src[x*3+1];			
							*dest++ = src[x*3+2];			
						}
						src += w*2*3;
					}
					matData.width /= 2;
					matData.height /= 2;		
				}
				else
				if (matData.pixelSrcFormat == GL_RGBA)
				{
					int		x,y;
					u_long	*src,*dest;
					
					dest = src = (u_long *)buffer;
					
					for (y = 0; y < h; y+=2)
					{
						for (x = 0; x < w; x+=2)
						{
							*dest++ = src[x];			
						}
						src += w*2;
					}
					matData.width /= 2;
					matData.height /= 2;		
				}

				else
				if (matData.pixelSrcFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV)
				{
					int		x,y;
					u_short	*src,*dest;
					
					dest = src = (u_short *)buffer;
					
					for (y = 0; y < h; y+=2)
					{
						for (x = 0; x < w; x+=2)
						{
							*dest++ = src[x];			
						}
						src += w*2;
					}
					matData.width /= 2;
					matData.height /= 2;		
				}
			}
		}
		
					/* SPRITE IS 16-BIT PACKED PIXEL FORMAT */
					
		if (matData.pixelSrcFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV)
		{
			matData.textureName[0] = OGL_TextureMap_Load(buffer, matData.width, matData.height, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV); // load 16 as 16
		
		}
		
				/* CONVERT 24-BIT TO 16--BIT */
				
		else
		if ((matData.pixelSrcFormat == GL_RGB) && (matData.pixelDstFormat == GL_RGB5_A1))
		{
			u_short	*buff16 = (u_short *)AllocPtr(matData.width*matData.height*2);			// alloc buff for 16-bit texture
						
			ConvertTexture24To16(buffer, buff16, matData.width, matData.height);
			matData.textureName[0] = OGL_TextureMap_Load(buff16, matData.width, matData.height, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV); // load 16 as 16
			
			SafeDisposePtr((Ptr)buff16);							// dispose buff

		}
		
				/* USE INPUT FORMATS */
		else
		{		
			matData.textureName[0] 	= OGL_TextureMap_Load(buffer,
													 matData.width,
													 matData.height,
													 matData.pixelSrcFormat,
													 matData.pixelDstFormat, GL_UNSIGNED_BYTE);
		}
			
		gSpriteGroupList[groupNum][i].materialObject = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);

		if (gSpriteGroupList[groupNum][i].materialObject == nil)
			DoFatalAlert("LoadSpriteFile: MO_CreateNewObjectOfType failed");
	
	
		SafeDisposePtr((Ptr)buffer);														// free the buffer

	}


	
		/* CLOSE FILE */
			
	FSClose(refNum);
	

}


#pragma mark -

/************* MAKE NEW SRITE OBJECT *************/

ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo)
{
ObjNode				*newObj;
MOSpriteObject		*spriteMO;
MOSpriteSetupData	spriteData;

			/* ERROR CHECK */
			
	if (newObjDef->type >= gNumSpritesInGroupList[newObjDef->group])
		DoFatalAlert("MakeSpriteObject: illegal type");


			/* MAKE OBJNODE */

	newObjDef->genre = SPRITE_GENRE;
	newObjDef->flags |= STATUS_BIT_DOUBLESIDED|STATUS_BIT_NOZBUFFER|STATUS_BIT_NOLIGHTING;
	
	newObj = MakeNewObject(newObjDef);		
	if (newObj == nil)
		return(nil);

			/* MAKE SPRITE META-OBJECT */

	spriteData.loadFile = false;										// these sprites are already loaded into gSpriteList
	spriteData.group	= newObjDef->group;								// set group
	spriteData.type 	= newObjDef->type;								// set group subtype


	spriteMO = MO_CreateNewObjectOfType(MO_TYPE_SPRITE, (u_long)setupInfo, &spriteData);
	if (!spriteMO)
		DoFatalAlert("MakeSpriteObject: MO_CreateNewObjectOfType failed!");


			/* SET SPRITE MO INFO */
			
	spriteMO->objectData.scaleX =
	spriteMO->objectData.scaleY = newObj->Scale.x;
	spriteMO->objectData.coord = newObj->Coord;


			/* ATTACH META OBJECT TO OBJNODE */
			
	newObj->SpriteMO = spriteMO;
	
	return(newObj);
}


/*********************** MODIFY SPRITE OBJECT IMAGE ******************************/

void ModifySpriteObjectFrame(ObjNode *theNode, short type, OGLSetupOutputType *setupInfo)
{
MOSpriteSetupData	spriteData;
MOSpriteObject		*spriteMO;


	if (type == theNode->Type)										// see if it is the same
		return;

		/* DISPOSE OF OLD TYPE */		
		
	MO_DisposeObjectReference(theNode->SpriteMO);


		/* MAKE NEW SPRITE MO */
		
	spriteData.loadFile = false;									// these sprites are already loaded into gSpriteList
	spriteData.group	= theNode->Group;							// set group
	spriteData.type 	= type;										// set group subtype

	spriteMO = MO_CreateNewObjectOfType(MO_TYPE_SPRITE, (u_long)setupInfo, &spriteData);
	if (!spriteMO)
		DoFatalAlert("ModifySpriteObjectFrame: MO_CreateNewObjectOfType failed!");


			/* SET SPRITE MO INFO */
			
	spriteMO->objectData.scaleX =
	spriteMO->objectData.scaleY = theNode->Scale.x;
	spriteMO->objectData.coord = theNode->Coord;


			/* ATTACH META OBJECT TO OBJNODE */
			
	theNode->SpriteMO = spriteMO;	
	theNode->Type = type;
}


#pragma mark -

/*********************** BLEND ALL SPRITES IN GROUP ********************************/
//
// Set the blending flag for all sprites in the group.
//

void BlendAllSpritesInGroup(short group)
{
int		i,n;
MOMaterialObject	*m;

	n = gNumSpritesInGroupList[group];								// get # sprites in this group
	if ((n == 0) || (gSpriteGroupList[group] == nil))
		DoFatalAlert("BlendAllSpritesInGroup: this group is empty");


			/* DISPOSE OF ALL LOADED OPENGL TEXTURENAMES */
			
	for (i = 0; i < n; i++)
	{
		m = gSpriteGroupList[group][i].materialObject; 				// get material object ptr		
		if (m == nil)
			DoFatalAlert("BlendAllSpritesInGroup: material == nil");
		
		m->objectData.flags |= 	BG3D_MATERIALFLAG_ALWAYSBLEND;		// set flag
	}
}


/*********************** BLEND A SPRITE ********************************/
//
// Set the blending flag for 1 sprite in the group.
//

void BlendASprite(int group, int type)
{
MOMaterialObject	*m;

	if (type >= gNumSpritesInGroupList[group])
		DoFatalAlert("BlendASprite: illegal type");


			/* DISPOSE OF ALL LOADED OPENGL TEXTURENAMES */
			
	m = gSpriteGroupList[group][type].materialObject; 				// get material object ptr		
	if (m == nil)
		DoFatalAlert("BlendASprite: material == nil");
	
	m->objectData.flags |= 	BG3D_MATERIALFLAG_ALWAYSBLEND;		// set flag
}


/************************** DRAW SPRITE ************************/

void DrawSprite(int	group, int type, float x, float y, float scale, float rot, u_long flags, const OGLSetupOutputType *setupInfo)
{
			/* SET STATE */
					
	OGL_PushState();								// keep state									

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();					

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);							
	glDisable(GL_DEPTH_TEST);


	if (flags & SPRITE_FLAG_GLOW)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	if (rot != 0.0f)
		glRotatef(OGLMath_RadiansToDegrees(rot), 0, 0, 1);											// remember:  rotation is in degrees, not radians!


		/* ACTIVATE THE MATERIAL */
					
	MO_DrawMaterial(gSpriteGroupList[group][type].materialObject, setupInfo);			


			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, y);
	glTexCoord2f(1,1);	glVertex2f(x+scale, y);
	glTexCoord2f(1,0);	glVertex2f(x+scale, y+scale);
	glTexCoord2f(0,0);	glVertex2f(x, y+scale);
	glEnd();	


		/* CLEAN UP */
			
	OGL_PopState();									// restore state
	gGlobalMaterialFlags = 0;

	gPolysThisFrame += 2;						// 2 tris drawn
}



#pragma mark -


/******************* DRAW FONT STRING ****************************/

void DrawFontString(Str255 s, float x, float y, float scale)
{
int		i, sp;
float	width = 0;

			/* CALCULATE THE WIDTH OF THE STRING */

	for (i = 1; i <= s[0]; i++)
		width += GetCharSpacing(s[i], scale);								
			
	x -= width * .5f;				// center it
	
	
			/* DRAW EACH CHARACTER */
			

	for (i = 1; i <= s[0]; i++)
	{
		sp = CharToSprite(s[i]);
	
		DrawInfobarSprite2_Centered(x, y, scale, SPRITE_GROUP_FONT, sp);
	
		x += GetCharSpacing(s[i], scale);								
	
	}	





}


/************* MAKE FONT STRING OBJECT *************/

ObjNode *MakeFontStringObject(const Str31 s, NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo, Boolean center)
{
ObjNode				*newObj;
MOSpriteObject		*spriteMO;
MOSpriteSetupData	spriteData;
int					i,len;
char				letter;			
float				scale,x;
			
	newObjDef->group = SPRITE_GROUP_FONT;
	newObjDef->genre = FONTSTRING_GENRE;
	newObjDef->flags |= STATUS_BIT_DOUBLESIDED|STATUS_BIT_NOZBUFFER|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOTEXTUREWRAP;



			/* MAKE OBJNODE */
	
	newObj = MakeNewObject(newObjDef);		

	newObj->NumStringSprites = 0;											// no sprites in there yet

	len = s[0];																// get length of string
	if (len > 31)
		DoFatalAlert("MakeFontStringObject: string > 31 characters!");


			/* ADJUST FOR CENTERING */
			
	scale = newObj->Scale.x;												// get scale factor
	
	if (center)
		x = newObj->Coord.x - GetStringWidth(s, scale) * .5f;				// calc center starting x coord on left
	else
		x = newObj->Coord.x;												// dont center text

	x += (scale * .5f);														// offset half a character

			/****************************/
			/* MAKE SPRITE META-OBJECTS */
			/****************************/
	
	for (i = 1; i <= len; i++)
	{
		letter = s[i];	
		
		spriteData.type = CharToSprite(letter);								// convert letter to sprite index
		if (spriteData.type == -1)											// skip spaces
			goto next;
			
		spriteData.loadFile = false;										// these sprites are already loaded into gSpriteList
		spriteData.group	= newObjDef->group;								// set group

		spriteMO = MO_CreateNewObjectOfType(MO_TYPE_SPRITE, (u_long)setupInfo, &spriteData);
		if (!spriteMO)
			DoFatalAlert("MakeFontStringObject: MO_CreateNewObjectOfType failed!");


				/* SET SPRITE MO INFO */

		spriteMO->objectData.coord.x = x;
		spriteMO->objectData.coord.y = newObj->Coord.y;
		spriteMO->objectData.coord.z = newObj->Coord.z;
				
		spriteMO->objectData.scaleX =
		spriteMO->objectData.scaleY = scale;


				/* ATTACH META OBJECT TO OBJNODE */
			
		newObj->StringCharacters[newObj->NumStringSprites++] = spriteMO;

next:
		x += GetCharSpacing(letter, scale);									// next letter x coord
	}

	
	return(newObj);
}







/***************** CHAR TO SPRITE **********************/

int CharToSprite(char c)
{

	if ((c >= 'a') && (c <= 'z'))
	{
		return(FONT_SObjType_a + (c - 'a'));
	}
	else
	if ((c >= 'A') && (c <= 'Z'))
	{
		return(FONT_SObjType_A + (c - 'A'));
	}
	else
	if ((c >= '0') && (c <= '9'))
	{
		return(FONT_SObjType_0 + (c - '0'));
	}
	else
	{
		short	s;
		switch((uint8_t)c)
		{
			case	'.':
					s = FONT_SObjType_Period;
					break;

			case	',':
					s = FONT_SObjType_Comma;
					break;

			case	'-':
					s = FONT_SObjType_Dash;
					break;

			case	'?':
					s = FONT_SObjType_QuestionMark;
					break;

			case	'!':
					s = FONT_SObjType_ExclamationMark;
					break;
					
			case	kMacRoman_iexcl:
					s = FONT_SObjType_ExclamationMark2;
					break;

			case	kMacRoman_Uuml:
					s = FONT_SObjType_UU;
					break;

			case	kMacRoman_uuml:
					s = FONT_SObjType_uu;
					break;

			case	kMacRoman_uacute:
					s = FONT_SObjType_ua;
					break;

			case	kMacRoman_Ouml:
					s = FONT_SObjType_OO;
					break;

			case	kMacRoman_ouml:
					s = FONT_SObjType_oo;
					break;

			case	kMacRoman_Auml:
					s = FONT_SObjType_AA;
					break;

			case	kMacRoman_Aring:
					s = FONT_SObjType_AO;
					break;

			case	kMacRoman_acirc:
					s = FONT_SObjType_av;
					break;

			case	kMacRoman_auml:
					s = FONT_SObjType_au;
					break;

			case	kMacRoman_aacute:
					s = FONT_SObjType_aa;
					break;

			case	kMacRoman_Ntilde:
					s = FONT_SObjType_NN;
					break;

			case	kMacRoman_ntilde:
					s = FONT_SObjType_nn;
					break;

			case	kMacRoman_Eacute:
					s = FONT_SObjType_EE;
					break;

			case	kMacRoman_eacute:
					s = FONT_SObjType_ee;
					break;

			case	kMacRoman_egrave:
					s = FONT_SObjType_ee;
					break;

			case	kMacRoman_ecirc:
					s = FONT_SObjType_ev;
					break;

			case	kMacRoman_Egrave:
					s = FONT_SObjType_EE;
					break;

			case	kMacRoman_Ecirc:
					s = FONT_SObjType_E;
					break;

			case	kMacRoman_Agrave:
					s = FONT_SObjType_Ax;
					break;

			case	kMacRoman_agrave:
					s = FONT_SObjType_ax;
					break;

			case	kMacRoman_aring:
					s = FONT_SObjType_ao;
					break;

			case	kMacRoman_Ocirc:
					s = FONT_SObjType_Ox;
					break;

			case	kMacRoman_Oacute:
					s = FONT_SObjType_Oa;
					break;

			case	kMacRoman_oacute:
					s = FONT_SObjType_oa;
					break;

			case	kMacRoman_szlig:
					s = FONT_SObjType_beta;
					break;

			case	kMacRoman_iacute:
					s = FONT_SObjType_ia;
					break;

			case	kMacRoman_Ccedil:
					s = FONT_SObjType_C;
					break;

			case	kMacRoman_ccedil:
					s = FONT_SObjType_c;
					break;

			case	'\'':
					s = FONT_SObjType_Apostrophe;
					break;
		
		
			default:
					s = -1;
		
		}
	return(s);
	}	
	
}


/***************** GET STRING WIDTH *************************/

float GetStringWidth(const u_char *s, float scale)
{
int		i;
float	w = 0;

	for (i = 1; i <= s[0]; i++)
		w += GetCharSpacing(s[i], scale);


	return(w);
}


/******************** GET CHAR SPACING *************************/

float GetCharSpacing(char c, float spacingScale)
{
float	s;

	switch((uint8_t)c)
	{				
		case	'i':
		case	kMacRoman_iacute:
		case	'.':
		case	',':
		case	'!':
		case	kMacRoman_iexcl:
				s = .25;
				break;

		case	CHAR_APOSTROPHE:
		case	's':
		case	'p':
		case	'l':				// L
		case	'o':
		case	kMacRoman_ouml:
		case	kMacRoman_oacute:
		case	'c':
		case	kMacRoman_ccedil:
				s = .3;
				break;

		case	'I':
		case	'f':
		case	't':
		case	'r':
		case	'j':
		case	'9':
		case	kMacRoman_auml:
		case	'a':
		case	kMacRoman_aacute:
		case	kMacRoman_agrave:
		case	kMacRoman_acirc:
		case	kMacRoman_aring:
		case	'b':
		case	'd':
		case	'e':
		case	kMacRoman_eacute:
		case	kMacRoman_ecirc:
		case	'g':
		case	'h':
		case	'k':
		case	'n':
		case	kMacRoman_ntilde:
		case	'q':
		case	'u':
		case	kMacRoman_uuml:
		case	kMacRoman_uacute:
		case	'v':
		case	'x':
		case	'y':
		case	'z':
				s = .35;
				break;

		case	' ':
		case	'm':
		case	'J':
		case	'0':
		case	'1':
		case	'2':
		case	'3':
		case	'4':
		case	'5':
		case	'6':
		case	'7':
		case	'8':
		case	'S':
				s = .4f;
				break;

		case	'Y':
		case	'O':
				s = .45;
				break;

		case	'A':
		case	'R':
		case	'V':
		case	'U':
				s = .55;
				break;

		case	'N':
		case	'H':
				s = .6f;
				break;

		case	'W':
		case	'M':
				s = .65;
				break;

		default:
				s = .5f;
	}





	return(spacingScale * s);


}



