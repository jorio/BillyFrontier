//
// windows.h
//

#define	ALLOW_FADE		1

extern void	InitWindowStuff(void);
ObjNode* MakeFadeEvent(Boolean	fadeIn);

void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void));

void Wait(long ticks);

void Enter2D(Boolean pauseDSp);
void Exit2D(void);

void GetDefaultWindowSize(int display, int* width, int* height);
int GetNumDisplays(void);
void MoveToPreferredDisplay(void);
void SetFullscreenMode(bool enforceDisplayPref);
