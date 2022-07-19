//
// miscscreens.h
//

// Externals
#include "game.h"



#define	SCORE_NUM_DIGITS	5


#define	DARKEN_PANE_Z	450.0f


void DisplayPicture(FSSpec *spec);
void DoPaused(void);

void DoLegalScreen(void);
void DoGameOptionsDialog(void);
ObjNode *MakeDarkenPane(void);

void DoDemoExpiredScreen(void);
void ShowDemoQuitScreen(void);

void DoMainMenuScreen(void);
void MoveBulletHole(ObjNode *theNode);


	/* BIG BOARD */
	
void DoBigBoardScreen(void);



		/* HIGH SCORES */
		
void NewScore(Boolean justShowScores);
void LoadHighScores(void);
void ClearHighScores(void);
		








