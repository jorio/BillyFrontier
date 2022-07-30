/****************************/
/*     SOUND ROUTINES       */
/* By Brian Greenstone      */
/* (c)2003 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static short EmergencyFreeChannel(void);
static void Calc3DEffectVolume(short effectNum, const OGLPoint3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut);
static void UpdateGlobalVolume(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define		MAX_CHANNELS			40

#define		MAX_EFFECTS				70

#define	FULL_SONG_VOLUME	0.5f
#define FULL_EFFECTS_VOLUME	0.5f

typedef struct
{
	Byte	bank,sound;
	float	refVol;
}EffectType;


/**********************/
/*     VARIABLES      */
/**********************/

static float				gGlobalVolumeFade = 1.0f;						// multiplier applied to sfx/song volumes
static float				gEffectsVolume = FULL_EFFECTS_VOLUME;
static float				gMusicVolume = FULL_SONG_VOLUME;
static float				gMusicVolumeTweak = 1.0f;

static OGLPoint3D			gEarCoords;										// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector;


static	SndListHandle		gSndHandles[MAX_SOUND_BANKS][MAX_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[MAX_SOUND_BANKS][MAX_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
ChannelInfoType				gChannelInfo[MAX_CHANNELS];
static	SndChannelPtr		gMusicChannel;

static short				gMaxChannels = 0;

static short				gMostRecentChannel = -1;

static short				gNumSndsInBank[MAX_SOUND_BANKS] = {0,0};


Boolean						gSongPlayingFlag = false;
Boolean						gLoopSongFlag = true;
Boolean						gAllowAudioKeys = true;

int							gNumLoopingEffects;

Boolean				gMuteMusicFlag = false;
short				gCurrentSong = -1;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/
		
		/*****************/
		/* EFFECTS TABLE */
		/*****************/
		
static EffectType	gEffectsTable[] =
{
	SOUND_BANK_SONG,0, 1.0,									// EFFECT_SONG

	SOUND_BANK_MAIN,SOUND_DEFAULT_GUNSHOT, 1.5,				// EFFECT_GUNSHOT
	SOUND_BANK_MAIN,SOUND_DEFAULT_BULLETHIT, 1.3,			// EFFECT_BULLETHIT
	SOUND_BANK_MAIN,SOUND_DEFAULT_SPURS1,.8,				// EFFECT_SPURS1
	SOUND_BANK_MAIN,SOUND_DEFAULT_SPURS2, .8,				// EFFECT_SPURS2
	SOUND_BANK_MAIN,SOUND_DEFAULT_SHIELDHIT, 1.0,			// EFFECT_SHIELDHIT
	SOUND_BANK_MAIN,SOUND_DEFAULT_WIND, 1.0,				// EFFECT_WIND
	SOUND_BANK_MAIN,SOUND_DEFAULT_MOO1,1.6,					// EFFECT_MOO1
	SOUND_BANK_MAIN,SOUND_DEFAULT_MOO2,1.6,					// EFFECT_MOO2
	SOUND_BANK_MAIN,SOUND_DEFAULT_GUNSHOT2, 1.0,			// EFFECT_GUNSHOT2
	SOUND_BANK_MAIN,SOUND_DEFAULT_GUNSHOT3, 1.0,			// EFFECT_GUNSHOT3
	SOUND_BANK_MAIN,SOUND_DEFAULT_RELOAD, 1.0,				// EFFECT_RELOAD
	SOUND_BANK_MAIN,SOUND_DEFAULT_RICOCHET, 1.1,			// EFFECT_RICOCHET
	SOUND_BANK_MAIN,SOUND_DEFAULT_DUELKEY, 1.0,				// EFFECT_DUELKEY
	SOUND_BANK_MAIN,SOUND_DEFAULT_DUELFAIL, 1.0,			// EFFECT_DUELFAIL
	SOUND_BANK_MAIN,SOUND_DEFAULT_DUELKEYSDONE, 1.0,		// EFFECT_DUELKEYSDONE
	SOUND_BANK_MAIN,SOUND_DEFAULT_HOOF, 1.0,				// EFFECT_HOOF
	SOUND_BANK_MAIN,SOUND_DEFAULT_WALKERCRASH, 1.2,			// EFFECT_WALKERCRASH
	SOUND_BANK_MAIN,SOUND_DEFAULT_BULLETHITMETAL, 1.5,		// EFFECT_BULLETHITMETAL
	SOUND_BANK_MAIN,SOUND_DEFAULT_WALKERFOOTSTEP, 1.0,		// EFFECT_WALKERFOOTSTEP
	SOUND_BANK_MAIN,SOUND_DEFAULT_WALKERAMBIENT, .7,		// EFFECT_WALKERAMBIENT
	SOUND_BANK_MAIN,SOUND_DEFAULT_CREATEEXPLODE, 1.3,		// EFFECT_CRATEEXPLODE
	SOUND_BANK_MAIN,SOUND_DEFAULT_GLASSBREAK, 4.0,			// EFFECT_GLASSBREAK
	SOUND_BANK_MAIN,SOUND_DEFAULT_COINSMASH, 5.0,			// EFFECT_COINSMASH
	SOUND_BANK_MAIN,SOUND_DEFAULT_GHOSTVAPORIZE, 1.0,		// EFFECT_GHOSTVAPORIZE
	SOUND_BANK_MAIN,SOUND_DEFAULT_EMPTY, 1.0,				// EFFECT_EMPTY
	SOUND_BANK_MAIN,SOUND_DEFAULT_TRAMPLED, 2.0,			// EFFECT_TRAMPLED
	SOUND_BANK_MAIN,SOUND_DEFAULT_GETCOIN, 1.0,				// EFFECT_GETCOIN
	SOUND_BANK_MAIN,SOUND_DEFAULT_LAUNCHMISSILE, 1.0,		// EFFECT_LAUNCHMISSILE
	SOUND_BANK_MAIN,SOUND_DEFAULT_EXPLOSION, 1.3,			// EFFECT_EXPLOSION
	SOUND_BANK_MAIN,SOUND_DEFAULT_DEATHSKULL, 2.0,			// EFFECT_DEATHSKULL
	SOUND_BANK_MAIN,SOUND_DEFAULT_TIMERCHIME, 1.0,			// EFFECT_TIMERCHIME
	SOUND_BANK_MAIN,SOUND_DEFAULT_SWISH, 1.5,				// EFFECT_SWISH
	SOUND_BANK_MAIN,SOUND_DEFAULT_YELP, 1.7,				// EFFECT_YELP
	SOUND_BANK_MAIN,SOUND_DEFAULT_ALARM, 1.0,				// EFFECT_ALARM

};




/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr			iErr;
FSSpec			spec;
	
	gNumLoopingEffects = 0;

	gMaxChannels = 0;
	gMostRecentChannel = -1;

			/* INIT BANK INFO */

	memset(gNumSndsInBank, 0, sizeof(gNumSndsInBank));

			/******************/
			/* ALLOC CHANNELS */
			/******************/

	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */

		iErr = SndNewChannel(&gSndChannel[gMaxChannels], sampledSynth, 0, nil);
		if (iErr)												// if err, stop allocating channels
		{
			break;
		}
	}

			/* MUSIC CHANNEL */

	iErr = SndNewChannel(&gMusicChannel, sampledSynth, 0, nil);
	GAME_ASSERT_MESSAGE(!iErr, "Couldn't allocate music channel");


		/***********************/
		/* LOAD DEFAULT SOUNDS */
		/***********************/
		
	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec) != noErr)
		DoFatalAlert("InitSoundTools: where is Main.sounds?");
	LoadSoundBank(&spec, SOUND_BANK_MAIN);
}


/******************** SHUTDOWN SOUND ***************************/
//
// Called at Quit time
//

void ShutdownSound(void)
{
int	i;

			/* STOP ANY PLAYING AUDIO */
			
	StopAllEffectChannels();
	KillSong();


		/* DISPOSE OF CHANNELS */
		
	for (i = 0; i < gMaxChannels; i++)
		SndDisposeChannel(gSndChannel[i], true);	
	gMaxChannels = 0;
	
	
}

#pragma mark -

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(FSSpec *spec, long bankNum)
{
short			srcFile1,numSoundsInBank,i;
OSErr			iErr;

	StopAllEffectChannels();

	if (bankNum >= MAX_SOUND_BANKS)
		DoFatalAlert("LoadSoundBank: bankNum >= MAX_SOUND_BANKS");

			/* DISPOSE OF EXISTING BANK */
			
	DisposeSoundBank(bankNum);


			/* OPEN APPROPRIATE REZ FILE */
			
	srcFile1 = FSpOpenResFile(spec, fsRdPerm);
	if (srcFile1 == -1)
	{
		DoAlert("LoadSoundBank: OpenResFile failed!");
		ShowSystemErr(ResError());		
	}

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	UseResFile( srcFile1 );												// open sound resource fork
	numSoundsInBank = Count1Resources('snd ');							// count # snd's in this bank
	if (numSoundsInBank > MAX_EFFECTS)
		DoFatalAlert("LoadSoundBank: numSoundsInBank > MAX_EFFECTS");

	for (i=0; i < numSoundsInBank; i++)
	{
				/* LOAD SND REZ */
				
		gSndHandles[bankNum][i] = (SndListResource **)GetResource('snd ',BASE_EFFECT_RESOURCE+i);
		if (gSndHandles[bankNum][i] == nil) 
		{
			iErr = ResError();
			DoAlert("LoadSoundBank: GetResource failed!");
			if (iErr == memFullErr)
				DoFatalAlert("LoadSoundBank: Out of Memory");
			else
				ShowSystemErr(iErr);
		}
		DetachResource((Handle)gSndHandles[bankNum][i]);				// detach resource from rez file & make a normal Handle
			
		HNoPurge((Handle)gSndHandles[bankNum][i]);						// make non-purgeable
		HLockHi((Handle)gSndHandles[bankNum][i]);
		
				/* GET OFFSET INTO IT */
				
		GetSoundHeaderOffset(gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);


				/* PRE-DECOMPRESS IT */

		Pomme_DecompressSoundResource(&gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);
	}

	UseResFile(gMainAppRezFile );								// go back to normal res file
	CloseResFile(srcFile1);

	gNumSndsInBank[bankNum] = numSoundsInBank;					// remember how many sounds we've got
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(short bankNum)
{
short	i; 


	if (bankNum > MAX_SOUND_BANKS)
		return;

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */
			
	for (i=0; i < gNumSndsInBank[bankNum]; i++)
		DisposeHandle((Handle)gSndHandles[bankNum][i]);


	gNumSndsInBank[bankNum] = 0;
}



/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
OSErr 		myErr;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

	mySndCmd.cmd = flushCmd;	
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);
	
	*channelNum = -1;
	
	if (gChannelInfo[c].isLooping)
	{
		gNumLoopingEffects--;
		gChannelInfo[c].isLooping = false;
	}
	
	gChannelInfo[c].effectNum = -1;	
}


/********************* STOP ALL EFFECT CHANNELS **********************/

void StopAllEffectChannels(void)
{
	for (short i = 0; i < gMaxChannels; i++)
	{
		short c = i;
		StopAChannel(&c);
	}
}


#pragma mark -



/******************** PLAY SONG ***********************/
//
// if songNum == -1, then play existing open song
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(short songNum, Boolean loopFlag)
{
OSErr 	iErr;
static	SndCommand 		mySndCmd;
FSSpec	spec;
short	musicFileRefNum;



	if (songNum == gCurrentSong)					// see if this is already playing
		return;

	if (songNum < 0 || songNum >= MAX_SONGS)
		DoFatalAlert("PlaySong: unknown song #");

		/* ZAP ANY EXISTING SONG */
		
	gCurrentSong 	= songNum;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();

			/******************************/
			/* OPEN APPROPRIATE SONG FILE */
			/******************************/

	static const struct
	{
		const char* path;
		float volumeTweak;
	} songs[MAX_SONGS] =
	{
		[SONG_THEME]		= {":audio:Theme.mp3",		1.6f},
		[SONG_DUEL]			= {":audio:Duel.mp3",		1.9f},
		[SONG_STAMPEDE]		= {":audio:Stampede.mp3",	1.1f},
		[SONG_SHOOTOUT]		= {":audio:Shootout.mp3",	1.1f},
		[SONG_LOSE]			= {":audio:Lose.mp3",		1.4f},
	};

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, songs[songNum].path, &spec);
	GAME_ASSERT(!iErr);

	iErr = FSpOpenDF(&spec, fsRdPerm, &musicFileRefNum);
	GAME_ASSERT(!iErr);

	gCurrentSong = songNum;
	gMusicVolumeTweak = songs[songNum].volumeTweak;


				/*****************/
				/* START PLAYING */
				/*****************/

			/* START PLAYING FROM FILE */

	iErr = SndStartFilePlay(
		gMusicChannel,
		musicFileRefNum,
		0,
		0, //STREAM_BUFFER_SIZE
		0, //gMusicBuffer
		nil,
		nil, //SongCompletionProc
		true);

	FSClose(musicFileRefNum);		// close the file (Pomme decompresses entire song into memory)

	if (iErr)
	{
		DoAlert("PlaySong: SndStartFilePlay failed!");
		ShowSystemErr(iErr);
	}

	gSongPlayingFlag = true;


			/* SET LOOP FLAG ON STREAM (SOURCE PORT ADDITION) */
			/* So we don't need to re-read the file over and over. */

	mySndCmd.cmd = pommeSetLoopCmd;
	mySndCmd.param1 = loopFlag ? 1 : 0;
	mySndCmd.param2 = 0;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
		DoFatalAlert("PlaySong: SndDoImmediate (pomme loop extension) failed!");

	uint32_t lv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	uint32_t rv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	mySndCmd.cmd = volumeCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
		DoFatalAlert("PlaySong: SndDoImmediate (volumeCmd) failed!");

	
			/* SEE IF WANT TO MUTE THE MUSIC */
			
	if (gMuteMusicFlag)
	{
		IMPLEMENT_ME_SOFT();
#if 0
		if (gSongMovie)
			StopMovie(gSongMovie);
#endif
	}
}

/*********************** KILL SONG *********************/

void KillSong(void)
{
	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;
		
	gSongPlayingFlag = false;											// tell callback to do nothing

	SndStopFilePlay(gMusicChannel, true);								// stop it
}

/******************** TOGGLE MUSIC *********************/

void ToggleMusic(void)
{
	gMuteMusicFlag = !gMuteMusicFlag;

	IMPLEMENT_ME_SOFT();
#if 0
	if (gSongMovie)
	{
		if (gMuteMusicFlag)
			StopMovie(gSongMovie);
		else
			StartMovie(gSongMovie);
	}
#endif
}






#pragma mark -

/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(short effectNum, const OGLPoint3D *where)
{
short					theChan;
Byte					bankNum,soundNum;
u_long					leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */
			
	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);	
	}

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, 1.0, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, NORMAL_CHANNEL_RATE);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = 1.0;			// full volume adjust
						
	return(theChan);									// return channel #	
}



/***************************** PLAY EFFECT PARMS 3D ***************************/
//
// Plays an effect with parameters in 3D
//
// OUTPUT: channel # used to play sound
//

short PlayEffect_Parms3D(short effectNum, const OGLPoint3D *where, u_long rateMultiplier, float volumeAdjust)
{
short			theChan;
Byte			bankNum,soundNum;
u_long			leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */
			
	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);	
	}

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, volumeAdjust, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


				/* PLAY EFFECT */
				
	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, rateMultiplier);
	
	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = volumeAdjust;	// remember volume adjuster

	return(theChan);									// return channel #	
}


/************************* UPDATE 3D SOUND CHANNEL ***********************/
//
// Returns TRUE if effectNum was a mismatch or something went wrong
//

Boolean Update3DSoundChannel(short effectNum, short *channel, OGLPoint3D *where)
{
SCStatus		theStatus;
u_long			leftVol,rightVol;
short			c;

	c = *channel;

	if (c == -1)
		return(true);

			/* MAKE SURE THE SAME SOUND IS STILL ON THIS CHANNEL */
			
	if (effectNum != gChannelInfo[c].effectNum)
	{
		*channel = -1;
		return(true);
	}
	

			/* SEE IF SOUND HAS COMPLETED */
			
	if (!gChannelInfo[c].isLooping)										// loopers wont complete, duh.
	{
		SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
		if (!theStatus.scChannelBusy)									// see if channel not busy
		{
			StopAChannel(channel);							// make sure it's really stopped (OS X sound manager bug)
			return(true);
		}
	}

			/* UPDATE THE THING */

	if (where)
	{
		Calc3DEffectVolume(gChannelInfo[c].effectNum, where, gChannelInfo[c].volumeAdjust, &leftVol, &rightVol);
		if ((leftVol+rightVol) == 0)										// if volume goes to 0, then kill channel
		{
			StopAChannel(channel);
			return(false);
		}

		ChangeChannelVolume(c, leftVol, rightVol);
	}	
	return(false);
}

/******************** CALC 3D EFFECT VOLUME *********************/

static void Calc3DEffectVolume(short effectNum, const OGLPoint3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut)
{
float	dist;
float	volumeFactor;
u_long	volume,left,right;
u_long	maxLeft,maxRight;

	dist 	= OGLPoint3D_Distance(where, &gEarCoords);		// calc dist to sound
		

	volumeFactor = (300.0f / dist) * gEffectsTable[effectNum].refVol;
	if (volumeFactor > 1.0f)
		volumeFactor = 1.0f;

	
	volume = (float)FULL_CHANNEL_VOLUME * volumeFactor * volAdjust;	
	
				
	if (volume < 6)							// if really quiet, then just turn it off
	{
		*leftVolOut = *rightVolOut = 0;
		return;
	}

			/************************/
			/* DO STEREO SEPARATION */
			/************************/
	
	else		
	{
		float		volF = (float)volume;
		OGLVector2D	earToSound,lookVec;
		float		dot,cross;
		
		maxLeft = maxRight = 0;
		
			/* CALC VECTOR TO SOUND */
			
		earToSound.x = where->x - gEarCoords.x;
		earToSound.y = where->z - gEarCoords.z;
		FastNormalizeVector2D(earToSound.x, earToSound.y, &earToSound, true);
		
		
			/* CALC EYE LOOK VECTOR */
			
		FastNormalizeVector2D(gEyeVector.x, gEyeVector.z, &lookVec, true);
			

			/* DOT PRODUCT  TELLS US HOW MUCH STEREO SHIFT */
			
		dot = 1.0f - fabs(OGLVector2D_Dot(&earToSound,  &lookVec));
		if (dot < 0.0f)
			dot = 0.0f;
		else
		if (dot > 1.0f)
			dot = 1.0f;


			/* CROSS PRODUCT TELLS US WHICH SIDE */
			
		cross = OGLVector2D_Cross(&earToSound,  &lookVec);
		
		
				/* DO LEFT/RIGHT CALC */
				
		if (cross > 0.0f)
		{
			left 	= volF + (volF * dot);
			right 	= volF - (volF * dot);
		}
		else
		{
			right 	= volF + (volF * dot);
			left 	= volF - (volF * dot);
		}
		
		
				/* KEEP MAX */
				
		if (left > maxLeft)
			maxLeft = left;
		if (right > maxRight)
			maxRight = right;
				
	}

	*leftVolOut = maxLeft;
	*rightVolOut = maxRight;		
}



#pragma mark -

/******************* UPDATE LISTENER LOCATION ******************/
//
// Get ear coord for all local players
//

void UpdateListenerLocation(void)
{
OGLVector3D	v;

	v.x = gGameViewInfoPtr->cameraPlacement.pointOfInterest.x - gGameViewInfoPtr->cameraPlacement.cameraLocation.x;	// calc line of sight vector
	v.y = gGameViewInfoPtr->cameraPlacement.pointOfInterest.y - gGameViewInfoPtr->cameraPlacement.cameraLocation.y;
	v.z = gGameViewInfoPtr->cameraPlacement.pointOfInterest.z - gGameViewInfoPtr->cameraPlacement.cameraLocation.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);

	gEarCoords.x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
	gEarCoords.y = gGameViewInfoPtr->cameraPlacement.cameraLocation.y + (v.y * 300.0f);
	gEarCoords.z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z + (v.z * 300.0f);
	
	gEyeVector = v;							
}


/***************************** PLAY EFFECT ***************************/
//  
// OUTPUT: channel # used to play sound
//

short PlayEffect(short effectNum)
{

	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE));

}


/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short  PlayEffect_Parms(short effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
Byte			bankNum,soundNum;
OSErr			myErr;
u_long			lv2,rv2;
//static UInt32          loopStart, loopEnd;
//SoundHeaderPtr   sndPtr;

	leftVolume *= gEffectsTable[effectNum].refVol;				// adjust by ref volume
	rightVolume *= gEffectsTable[effectNum].refVol;

	
			/* GET BANK & SOUND #'S FROM TABLE */
			
	bankNum = gEffectsTable[effectNum].bank;
	soundNum = gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);	
	}

			/* LOOK FOR FREE CHANNEL */
			
	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

	lv2 = (float)leftVolume * gEffectsVolume;					// amplify by global volume
	rv2 = (float)rightVolume * gEffectsVolume;


					/* GET IT GOING */

	chanPtr = gSndChannel[theChan];						
	
	mySndCmd.cmd = flushCmd;	
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);


	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.ptr = ((Ptr) *gSndHandles[bankNum][soundNum]) + gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
	SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency 
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;
	SndDoImmediate(chanPtr, &mySndCmd);

    
    
    		/* SEE IF THIS IS A LOOPING EFFECT */    		
#if 0
    sndPtr = (SoundHeaderPtr)(((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum]);
    loopStart = sndPtr->loopStart;
    loopEnd = sndPtr->loopEnd;
    if ((loopStart + 1) < loopEnd)
    {
    	mySndCmd.cmd = callBackCmd;										// let us know when the buffer is done playing
    	mySndCmd.param1 = 0;
    	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
    	SndDoCommand(chanPtr, &mySndCmd, true);
    	
    	gChannelInfo[theChan].isLooping = true;
    	gNumLoopingEffects++;
	}
	else
		gChannelInfo[theChan].isLooping = false;
#endif

			/* SET MY INFO */
			
	gChannelInfo[theChan].effectNum 	= effectNum;		// remember what effect is playing on this channel
	gChannelInfo[theChan].leftVolume 	= leftVolume;		// remember requested volume (not the adjusted volume!)
	gChannelInfo[theChan].rightVolume 	= rightVolume;	
	return(theChan);										// return channel #	
}


#pragma mark -


/****************** UPDATE GLOBAL VOLUME ************************/
//
// Call this whenever gGlobalVolume is changed.  This will update
// all of the sounds with the correct volume.
//

static void UpdateGlobalVolume(void)
{
int		c;

			/* ADJUST VOLUMES OF ALL CHANNELS REGARDLESS IF THEY ARE PLAYING OR NOT */
			
	for (c = 0; c < gMaxChannels; c++)
	{
		ChangeChannelVolume(c, gChannelInfo[c].leftVolume, gChannelInfo[c].rightVolume);
	}



			/* UPDATE SONG VOLUME */

	// First, resume song playback if it was paused --
	// e.g. when we're adjusting the volume via pause menu
	SndCommand cmd1 = { .cmd = pommeResumePlaybackCmd };
	SndDoImmediate(gMusicChannel, &cmd1);

	// Now update song channel volume
	gMusicVolume = FULL_SONG_VOLUME /* * (0.01f * gGamePrefs.musicVolumePercent) */ * gGlobalVolumeFade;
	uint32_t lv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	uint32_t rv2 = kFullVolume * gMusicVolumeTweak * gMusicVolume;
	SndCommand cmd2 =
	{
		.cmd = volumeCmd,
		.param1 = 0,
		.param2 = (rv2 << 16) | lv2,
	};
	SndDoImmediate(gMusicChannel, &cmd2);

}

/*************** CHANGE CHANNEL VOLUME **************/
//
// Modifies the volume of a currently playing channel
//

void ChangeChannelVolume(short channel, float leftVol, float rightVol)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
u_long			lv2,rv2;

	if (channel < 0)									// make sure it's valid
		return;

	lv2 = leftVol * gEffectsVolume;				// amplify by global volume
	rv2 = rightVol * gEffectsVolume;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr				

	mySndCmd.cmd = volumeCmd;							// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;			// set volume left & right
	SndDoImmediate(chanPtr, &mySndCmd);

	gChannelInfo[channel].leftVolume = leftVol;				// remember requested volume (not the adjusted volume!)
	gChannelInfo[channel].rightVolume = rightVol;
}



/*************** CHANGE CHANNEL RATE **************/
//
// Modifies the frequency of a currently playing channel
//
// The Input Freq is a fixed-point multiplier, not the static rate via rateCmd.
// This function uses rateMultiplierCmd, so a value of 0x00020000 is x2.0
//

void ChangeChannelRate(short channel, long rateMult)
{
static	SndCommand 		mySndCmd;
SndChannelPtr			chanPtr;

	if (channel < 0)									// make sure it's valid
		return;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr				

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency 
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMult;	
	SndDoImmediate(chanPtr, &mySndCmd);
}




#pragma mark -


/******************** DO SOUND MAINTENANCE *************/
//
// 		ReadKeyboard() must have already been called
//

void DoSoundMaintenance(void)
{

#if 0
	if (gAllowAudioKeys)									
	{
					/* SEE IF TOGGLE MUSIC */

		if (GetNewKeyState_Real(KEY_M))
		{
			ToggleMusic();			
		}


				/* SEE IF CHANGE VOLUME */

		if (GetKeyState_Real(KEY_PLUS))
		{
			gGlobalVolume += 1.5f * gFramesPerSecondFrac;
			UpdateGlobalVolume();
		}
		else
		if (GetKeyState_Real(KEY_MINUS))
		{
			gGlobalVolume -= 1.5f * gFramesPerSecondFrac;
			if (gGlobalVolume < 0.0f)
				gGlobalVolume = 0.0f;
			UpdateGlobalVolume();
		}

					/* UPDATE SONG */
					
		if (gSongPlayingFlag)
		{
			if (IsMovieDone(gSongMovie))				// see if the song has completed
			{
				if (gLoopSongFlag)						// see if repeat it
				{
					GoToBeginningOfMovie(gSongMovie);
					StartMovie(gSongMovie);
				}
				else									// otherwise kill the song
					KillSong();
			}
			else
			{
				gMoviesTaskTimer -= gFramesPerSecondFrac;
				if (gMoviesTaskTimer <= 0.0f)
				{
					MoviesTask(gSongMovie, 0);
					gMoviesTaskTimer += .15f;
				}
			}
		}


	}



		/* ALSO CHECK OPTIONS */

	if (GetNewKeyState_Real(KEY_F1))
	{
		DoGameOptionsDialog();
	}

#endif
}



/******************** FIND SILENT CHANNEL *************************/

static short FindSilentChannel(void)
{
short		theChan, startChan;
OSErr		myErr;
SCStatus	theStatus;

	theChan = gMostRecentChannel + 1;					// start on channel after the most recently acquired one - assuming it has the best chance of being silent
	if (theChan >= gMaxChannels)
		theChan = 0;
	startChan = theChan;

	do
	{
		if (gChannelInfo[theChan].isLooping)				// see if this channel is playing a looping effect
			goto next;
			
		myErr = SndChannelStatus(gSndChannel[theChan],sizeof(SCStatus),&theStatus);	// get channel info
		if (myErr)
			goto next;

		if (theStatus.scChannelBusy)					// see if channel busy
			goto next;
	
		gMostRecentChannel = theChan;
		return(theChan);
		
next:		
		theChan++;										// try next channel
		if (theChan >= gMaxChannels)
			theChan = 0;	
		
	}while(theChan != startChan);
	
			/* NO FREE CHANNELS */
	
	return(-1);										
}


/********************** IS EFFECT CHANNEL PLAYING ********************/

Boolean IsEffectChannelPlaying(short chanNum)
{
SCStatus	theStatus;

	SndChannelStatus(gSndChannel[chanNum],sizeof(SCStatus),&theStatus);	// get channel info
	return (theStatus.scChannelBusy);
}

/***************** EMERGENCY FREE CHANNEL **************************/
//
// This is used by the music streamer when all channels are used.
// Since we must have a channel to play music, we arbitrarily kill some other channel.
//

static short EmergencyFreeChannel(void)
{
short	i,c;

	for (i = 0; i < gMaxChannels; i++)
	{
		c = i;
		StopAChannel(&c);
		return(i);
	}
	
		/* TOO BAD, GOTTA NUKE ONE OF THE STREAMING CHANNELS IT SEEMS */
			
	StopAChannel(0);
	return(0);
}


















