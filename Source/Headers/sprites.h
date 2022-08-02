//
// sprites.h
//

// Externals
#include "game.h"

enum
{
	SPRITE_FLAG_GLOW = (1)
};


typedef struct
{
	int32_t			width;
	int32_t			height;
	float			aspectRatio;			// h/w
	GLint			srcFormat;
	GLint			destFormat;
	MetaObjectPtr	materialObject;
}SpriteType;


void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
void LoadSpriteGroup(int groupNum);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef);
void BlendAllSpritesInGroup(short group);
void ModifySpriteObjectFrame(ObjNode *theNode, short type);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, u_long flags);
void BlendASprite(int group, int type);

ObjNode *MakeFontStringObject(const char* cstr, NewObjectDefinitionType *newObjDef, Boolean center);
int CharToSprite(char c);
float GetCharSpacing(char c, float spacingScale);
float GetStringWidth(const char* cstr, float scale);
void DrawFontString(const char* cstr, float x, float y, float scale, Boolean center);
