//
// sprites.h
//



enum
{
	SPRITE_FLAG_GLOW = (1)
};


typedef struct
{
	int				width,height;
	float			aspectRatio;			// h/w
	GLint			srcFormat, destFormat;
	MetaObjectPtr	materialObject;
}SpriteType;


void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
void LoadSpriteFile(FSSpec *spec, int groupNum, OGLSetupOutputType *setupInfo);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo);
void BlendAllSpritesInGroup(short group);
void ModifySpriteObjectFrame(ObjNode *theNode, short type, OGLSetupOutputType *setupInfo);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, u_long flags, const OGLSetupOutputType *setupInfo);
void BlendASprite(int group, int type);

ObjNode *MakeFontStringObject(const Str31 s, NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo, Boolean center);
int CharToSprite(char c);
float GetCharSpacing(char c, float spacingScale);
float GetStringWidth(const u_char *s, float scale);
void DrawFontString(Str255 s, float x, float y, float scale);
