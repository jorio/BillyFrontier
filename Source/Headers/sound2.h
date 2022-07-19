//
// Sound2.h
//

// Externals
#include "game.h"

typedef struct
{
	short	effectNum;
	float	volumeAdjust;
	float	leftVolume, rightVolume;
	Boolean	isLooping;
}ChannelInfoType;



#define		BASE_EFFECT_RESOURCE	10000

#define		FULL_CHANNEL_VOLUME		kFullVolume
#define		NORMAL_CHANNEL_RATE		0x10000



enum
{
	SONG_THEME = 0,
	SONG_DUEL,
	SONG_STAMPEDE,
	SONG_SHOOTOUT,
	SONG_LOSE
};

enum
{
	SOUND_BANK_MAIN 			= 0,
	SOUND_BANK_MENU				= 1,
	SOUND_BANK_BONUS			= 1,
	SOUND_BANK_LOSE				= 1,
	SOUND_BANK_TITLE			= 1,
	SOUND_BANK_MAINMENU			= 1,
	SOUND_BANK_LEVELSPECIFIC	= 2,
	SOUND_BANK_SONG				= 3,
	
	MAX_SOUND_BANKS
};



/***************** EFFECTS *************************/
// This table must match gEffectsTable
//

enum
{
		/* SONG */
		
	EFFECT_SONG = 0, 
	
		/* DEFAULT */
		
	EFFECT_GUNSHOT,
	EFFECT_BULLETHIT,
	EFFECT_SPURS1,
	EFFECT_SPURS2,
	EFFECT_SHIELDHIT,
	EFFECT_WIND,
	EFFECT_MOO1,
	EFFECT_MOO2,
	EFFECT_GUNSHOT2,
	EFFECT_GUNSHOT3,
	EFFECT_RELOAD,
	EFFECT_RICOCHET,
	EFFECT_DUELKEY,
	EFFECT_DUELFAIL,
	EFFECT_DUELKEYSDONE,
	EFFECT_HOOF,
	EFFECT_WALKERCRASH,
	EFFECT_BULLETHITMETAL,
	EFFECT_WALKERFOOTSTEP,
	EFFECT_WALKERAMBIENT,
	EFFECT_CRATEEXPLODE,
	EFFECT_GLASSBREAK,
	EFFECT_COINSMASH,
	EFFECT_GHOSTVAPORIZE,
	EFFECT_EMPTY,
	EFFECT_TRAMPLED,
	EFFECT_GETCOIN,
	EFFECT_LAUNCHMISSILE,
	EFFECT_EXPLOSION,
	EFFECT_DEATHSKULL,
	EFFECT_TIMERCHIME,
	EFFECT_SWISH,
	EFFECT_YELP,
	EFFECT_ALARM,
		
	NUM_EFFECTS
};



/**********************/
/* SOUND BANK TABLES  */
/**********************/
//
// These are simple enums for equating the sound effect #'s in each sound
// bank's rez file.
//

/******** SOUND_BANK_MAIN *********/

enum
{
	SOUND_DEFAULT_GUNSHOT = 0,
	SOUND_DEFAULT_BULLETHIT,
	SOUND_DEFAULT_SPURS1,
	SOUND_DEFAULT_SPURS2,
	SOUND_DEFAULT_SHIELDHIT,
	SOUND_DEFAULT_WIND,
	SOUND_DEFAULT_MOO1,
	SOUND_DEFAULT_MOO2,
	
	SOUND_DEFAULT_GUNSHOT2,
	SOUND_DEFAULT_GUNSHOT3,
	SOUND_DEFAULT_RELOAD,
	SOUND_DEFAULT_RICOCHET,
	SOUND_DEFAULT_DUELKEY,
	SOUND_DEFAULT_DUELFAIL,
	SOUND_DEFAULT_DUELKEYSDONE,
	SOUND_DEFAULT_HOOF,
	SOUND_DEFAULT_WALKERCRASH,
	SOUND_DEFAULT_BULLETHITMETAL,
	SOUND_DEFAULT_WALKERFOOTSTEP,
	SOUND_DEFAULT_WALKERAMBIENT,
	SOUND_DEFAULT_CREATEEXPLODE,
	SOUND_DEFAULT_GLASSBREAK,
	SOUND_DEFAULT_COINSMASH,
	SOUND_DEFAULT_GHOSTVAPORIZE,
	SOUND_DEFAULT_EMPTY,
	SOUND_DEFAULT_TRAMPLED,
	SOUND_DEFAULT_GETCOIN,
	SOUND_DEFAULT_LAUNCHMISSILE,
	SOUND_DEFAULT_EXPLOSION,
	SOUND_DEFAULT_DEATHSKULL,
	SOUND_DEFAULT_TIMERCHIME,
	SOUND_DEFAULT_SWISH,
	SOUND_DEFAULT_YELP,
	SOUND_DEFAULT_ALARM
	
};




//===================== PROTOTYPES ===================================


extern void	InitSoundTools(void);
void ShutdownSound(void);

void StopAChannel(short *channelNum);
extern	void StopAllEffectChannels(void);
void PlaySong(short songNum, Boolean loopFlag);
extern void	KillSong(void);
extern	short PlayEffect(short effectNum);
short PlayEffect_Parms3D(short effectNum, const OGLPoint3D *where, u_long rateMultiplier, float volumeAdjust);
extern void	ToggleMusic(void);
extern void	DoSoundMaintenance(void);
extern	void LoadSoundBank(FSSpec *spec, long bankNum);
extern	void WaitEffectsSilent(void);
extern	void DisposeSoundBank(short bankNum);
short PlayEffect_Parms(short effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier);
void ChangeChannelVolume(short channel, float leftVol, float rightVol);
short PlayEffect3D(short effectNum, const OGLPoint3D *where);
Boolean Update3DSoundChannel(short effectNum, short *channel, OGLPoint3D *where);
Boolean IsEffectChannelPlaying(short chanNum);
void UpdateListenerLocation(OGLSetupOutputType *setupInfo);
void ChangeChannelRate(short channel, long rateMult);
Boolean StopAChannelIfEffectNum(short *channelNum, short effectNum);






