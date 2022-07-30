//
// windows.h
//

#define	ALLOW_FADE		1

extern void	InitWindowStuff(void);
ObjNode* MakeFadeEvent(Boolean	fadeIn);

extern	void CleanupDisplay(void);
void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void));

extern	void GameScreenToBlack(void);

void Wait(long ticks);

void Enter2D(Boolean pauseDSp);
void Exit2D(void);

