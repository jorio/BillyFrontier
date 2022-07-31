//
// Sound2.h
//

#pragma once

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
	SONG_LOSE,

	MAX_SONGS
};


/***************** EFFECTS *************************/
// This table must match gEffectsTable
//

enum
{
	EFFECT_GUNSHOT1,
	EFFECT_GUNSHOT2,
	EFFECT_GUNSHOT3,
	EFFECT_BULLETHIT,
	EFFECT_SPURS1,
	EFFECT_SPURS2,
	EFFECT_SHIELDHIT,
	EFFECT_WIND,
	EFFECT_MOO1,
	EFFECT_MOO2,
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


//===================== PROTOTYPES ===================================


void InitSoundTools(void);
void ShutdownSound(void);

void StopAChannel(short *channelNum);
extern	void StopAllEffectChannels(void);
void PlaySong(short songNum, Boolean loopFlag);
extern void	KillSong(void);
extern	short PlayEffect(short effectNum);
short PlayEffect_Parms3D(short effectNum, const OGLPoint3D *where, u_long rateMultiplier, float volumeAdjust);
extern void	ToggleMusic(void);
extern void	DoSoundMaintenance(void);
void LoadSoundBank(void);
void DisposeSoundBank(void);
short PlayEffect_Parms(short effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier);
void ChangeChannelVolume(short channel, float leftVol, float rightVol);
short PlayEffect3D(short effectNum, const OGLPoint3D *where);
Boolean Update3DSoundChannel(short effectNum, short *channel, OGLPoint3D *where);
Boolean IsEffectChannelPlaying(short chanNum);
void UpdateListenerLocation(void);
void ChangeChannelRate(short channel, long rateMult);






