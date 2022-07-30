//
// miscscreens.h
//

// Externals
#include "game.h"



#define	SCORE_NUM_DIGITS	5
#define SCORE_FMT			"%05d"


#define	DARKEN_PANE_Z	450.0f


void DisplayPicture(const char* path);
void DoPaused(void);

void DoLegalScreen(void);
void DoGameOptionsDialog(void);
ObjNode *MakeDarkenPane(void);

void DoMainMenuScreen(void);
void MoveBulletHole(ObjNode *theNode);


	/* BIG BOARD */
	
void DoBigBoardScreen(void);



		/* HIGH SCORES */
		
void NewScore(Boolean justShowScores);
void LoadHighScores(void);
void ClearHighScores(void);
		








