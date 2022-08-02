//
// miscscreens.h
//

#pragma once

#define	SCORE_NUM_DIGITS	5
#define SCORE_FMT			"%05d"


#define	DARKEN_PANE_Z	450.0f


void DoWarmUpScreen(void);
void DisplayPicture(const char* path);
void DoPaused(void);

void DoLegalScreen(void);
ObjNode *MakeDarkenPane(void);

void BuildMainMenu(int);
void DoMainMenuScreen(Byte startMenu);
void MoveBulletHole(ObjNode *theNode);


	/* BIG BOARD */

void DoBigBoardScreen(void);



		/* HIGH SCORES */

void NewScore(Boolean justShowScores);
void LoadHighScores(void);
void ClearHighScores(void);

