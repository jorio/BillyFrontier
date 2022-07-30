//
// windows.h
//

#define	ALLOW_FADE		1

extern void	InitWindowStuff(void);
extern	void MakeFadeEvent(Boolean	fadeIn);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);

void Wait(long ticks);

void Enter2D(Boolean pauseDSp);
void Exit2D(void);
