//
// windows.h
//

#define	USE_DSP			1
#define	ALLOW_FADE		(1 && USE_DSP)

// Externals
#include "game.h"

#if 0
extern	OSStatus DSpSetWindowToFront(WindowRef pWindow);		// DSp hack not in headers
#endif

extern void	InitWindowStuff(void);
extern void	DumpGWorld2(GWorldPtr, WindowPtr, Rect *);
extern void	DoLockPixels(GWorldPtr);
#if 0
pascal void DoBold (DialogPtr dlogPtr, short item);
pascal void DoOutline (DialogPtr dlogPtr, short item);
#endif
extern	void MakeFadeEvent(Boolean	fadeIn);
void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);

void DoLockPixels(GWorldPtr world);
void DoScreenModeDialog(void);

void Wait(long ticks);
void DoScreenModeDialog(void);

void Enter2D(Boolean pauseDSp);
void Exit2D(void);
