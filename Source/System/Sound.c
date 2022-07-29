/****************************/
/*     SOUND ROUTINES       */
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
//#include <Components.h>
#include "3dmath.h"
#include "windows.h"
//#include <fp.h>
#include "Pomme.h"

extern	short		gMainAppRezFile;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec				gDataSpec;
extern	float		gFramesPerSecondFrac;
extern	PrefsType			gGamePrefs;
extern	Boolean				gOSX,gISpActive;


/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static short EmergencyFreeChannel(void);
static void Calc3DEffectVolume(short effectNum, const OGLPoint3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut);
static void UpdateGlobalVolume(void);
//static pascal void CallBackFn (SndChannelPtr chan, SndCommand *cmd);


/****************************/
/*    CONSTANTS             */
/****************************/


#define FloatToFixed16(a)      ((Fixed)((float)(a) * 0x000100L))		// convert float to 16bit fixed pt


#define		MAX_CHANNELS			40

#define		MAX_EFFECTS				70


typedef struct
{
	Byte	bank,sound;
	float	refVol;
}EffectType;



#define	VOLUME_DISTANCE_FACTOR	.04f		// bigger == sound decays FASTER with dist, smaller = louder far away

/**********************/
/*     VARIABLES      */
/**********************/


			/* SONG RELATED */
			
static CGrafPtr		gQTDummyPort = nil;
//Movie				gSongMovie = nil;
float				gMoviesTaskTimer = 0;

const float	gSongVolumeTweaks[]=
{
	1.6,				// theme
	1.9,				// duel
	1.1,				// stampede
	1.1,				// shooutout
	1.4,				// lose
};


const Str32	gSongNames[] =
{
	":audio:Theme.mp3",
	":audio:Duel.mp3",
	":audio:Stampede.mp3",
	":audio:Shootout.mp3",
	":audio:Lose.mp3",
};



		/* OTHER */
		
float						gGlobalVolume = 1.0;

static OGLPoint3D			gEarCoords;										// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector;


static	SndListHandle		gSndHandles[MAX_SOUND_BANKS][MAX_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[MAX_SOUND_BANKS][MAX_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
ChannelInfoType				gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;

static short				gMostRecentChannel = -1;

static short				gNumSndsInBank[MAX_SOUND_BANKS] = {0,0};


Boolean						gSongPlayingFlag = false;
Boolean						gLoopSongFlag = true;
Boolean						gAllowAudioKeys = true;

int							gNumLoopingEffects;

Boolean				gMuteMusicFlag = false;
short				gCurrentSong = -1;
short				gSongChannel = -1;


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
#if 0
OSErr			iErr;
short			i;
ExtSoundHeader	sndHdr;
const double	crap = rate44khz;
FSSpec			spec;
	
	gNumLoopingEffects = 0;

	gMaxChannels = 0;
	gMostRecentChannel = -1;

			/* INIT BANK INFO */
			
	for (i = 0; i < MAX_SOUND_BANKS; i++)
		gNumSndsInBank[i] = 0;

			/******************/
			/* ALLOC CHANNELS */
			/******************/

				/* MAKE DUMMY SOUND HEADER */
				
	sndHdr.samplePtr 		= nil;
    sndHdr.sampleRate		= rate44khz;  
    sndHdr.loopStart		= 0;         
    sndHdr.loopEnd			= 0;          
    sndHdr.encode			= extSH;           
    sndHdr.baseFrequency 	= 0;      
    sndHdr.numFrames		= 0;              
    sndHdr.numChannels		= 2;  
   	dtox80(&crap, &sndHdr.AIFFSampleRate);        
    sndHdr.markerChunk		= 0;          
    sndHdr.instrumentChunks	= 0;       
    sndHdr.AESRecording		= 0;
    sndHdr.sampleSize		= 16;           
    sndHdr.futureUse1		= 0;            
    sndHdr.futureUse2		= 0;            
    sndHdr.futureUse3		= 0;    
    sndHdr.futureUse4		= 0;  
    sndHdr.sampleArea[0]		= 0;  

	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */
			
		SndCommand 		mySndCmd;
		
		iErr = SndNewChannel(&gSndChannel[gMaxChannels],sampledSynth,initMono+initNoInterp,NewSndCallBackUPP(CallBackFn));
		if (iErr)												// if err, stop allocating channels
			break;
			
		gChannelInfo[gMaxChannels].isLooping = false;
		
			
			/* FOR POST- SM 3.6.5 DO THIS! */
	
		mySndCmd.cmd = soundCmd;	
		mySndCmd.param1 = 0;
		mySndCmd.param2 = (long)&sndHdr;
		if ((iErr = SndDoImmediate(gSndChannel[gMaxChannels], &mySndCmd)) != noErr)
		{
			DoAlert("InitSoundTools: SndDoImmediate failed!");
			ShowSystemErr_NonFatal(iErr);
		}
		
		
		mySndCmd.cmd = reInitCmd;	
		mySndCmd.param1 = 0;
		mySndCmd.param2 = initNoInterp|initStereo;
		if ((iErr = SndDoImmediate(gSndChannel[gMaxChannels], &mySndCmd)) != noErr)
		{
			DoAlert("InitSoundTools: SndDoImmediate failed 2!");
			ShowSystemErr_NonFatal(iErr);
		}
	}
	

		/***********************/
		/* LOAD DEFAULT SOUNDS */
		/***********************/
		
	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec) != noErr)
		DoFatalAlert("InitSoundTools: where is Main.sounds?");
	LoadSoundBank(&spec, SOUND_BANK_MAIN);	
#endif
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
Str255			error = "Couldnt Open Sound Resource File.";
OSErr			iErr;

	StopAllEffectChannels();

	if (bankNum >= MAX_SOUND_BANKS)
		DoFatalAlert("LoadSoundBank: bankNum >= MAX_SOUND_BANKS");

			/* DISPOSE OF EXISTING BANK */
			
	DisposeSoundBank(bankNum);


			/* OPEN APPROPRIATE REZ FILE */
			
	srcFile1 = FSpOpenResFile(spec, fsCurPerm);
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


/********************* STOP A CHANNEL IF EFFECT NUM **********************/
//
// Stops the indicated sound channel from playing if it is still this effect #
//

Boolean StopAChannelIfEffectNum(short *channelNum, short effectNum)
{
short		c = *channelNum;

	if (gChannelInfo[c].effectNum != effectNum)		// make sure its the right effect
		return(false);

	StopAChannel(channelNum);
	
	return(true);
}



/********************* STOP ALL EFFECT CHANNELS **********************/

void StopAllEffectChannels(void)
{
short		i;

	for (i=0; i < gMaxChannels; i++)
	{
		short	c;
		
		if (i != gSongChannel)							// don't stop the music channel!
		{
			c = i;
			StopAChannel(&c);
		}
	}
}


/****************** WAIT EFFECTS SILENT *********************/

void WaitEffectsSilent(void)
{
short	i;
Boolean	isBusy;
SCStatus				theStatus;

	do
	{
		isBusy = 0;
		for (i=0; i < gMaxChannels; i++)
		{
			SndChannelStatus(gSndChannel[i],sizeof(SCStatus),&theStatus);	// get channel info
			isBusy |= theStatus.scChannelBusy;
		}
	}while(isBusy);
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
short	myRefNum;
GrafPtr	oldPort;



	if (songNum == gCurrentSong)					// see if this is already playing
		return;


		/* ZAP ANY EXISTING SONG */
		
	gCurrentSong 	= songNum;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();

			/******************************/
			/* OPEN APPROPRIATE SND FILE */
			/******************************/
			
	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, gSongNames[songNum], &spec);
	if (iErr)
		DoFatalAlert("PlaySong: song file not found");

	gCurrentSong = songNum;
	
	
				/*****************/
				/* START PLAYING */
				/*****************/

			/* GOT TO SET A DUMMY PORT OR QT MAY FREAK */
			
	if (gQTDummyPort == nil)						// create a blank graf port
		gQTDummyPort = CreateNewPort();

	GetPort(&oldPort);								// set as active before NewMovieFromFile()
	SetPort(gQTDummyPort);


	iErr = OpenMovieFile(&spec, &myRefNum, fsRdPerm);
	if (myRefNum && (iErr == noErr))
	{
#if 0
		iErr = NewMovieFromFile(&gSongMovie, myRefNum, 0, nil, newMovieActive, nil);
		CloseMovieFile(myRefNum);
								
		if (iErr == noErr)
		{
//			if (gOSX)
//				LoadMovieIntoRam(gSongMovie, 0, GetMovieDuration(gSongMovie), keepInRam);
				
			SetMoviePlayHints(gSongMovie, 0, hintsUseSoundInterp|hintsHighQuality);	// turn these off			
			if (loopFlag)
				SetMoviePlayHints(gSongMovie, hintsLoop, hintsLoop);	// turn this on
						

			SetMovieVolume(gSongMovie, FloatToFixed16(gGlobalVolume * gSongVolumeTweaks[songNum]));						// set volume	
			StartMovie(gSongMovie);

			gSongPlayingFlag = true;
		}
#endif
	}

	SetPort(oldPort);
		

	
			/* SEE IF WANT TO MUTE THE MUSIC */
			
	if (gMuteMusicFlag)
	{
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

#if 0
	StopMovie(gSongMovie);
	DisposeMovie(gSongMovie);
		
	gSongMovie = nil;
#endif
	
//	gMusicFileRefNum = 0x0ded;
}

/******************** TOGGLE MUSIC *********************/

void ToggleMusic(void)
{
	gMuteMusicFlag = !gMuteMusicFlag;

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

void UpdateListenerLocation(OGLSetupOutputType *setupInfo)
{
OGLVector3D	v;

	v.x = setupInfo->cameraPlacement.pointOfInterest.x - setupInfo->cameraPlacement.cameraLocation.x;	// calc line of sight vector
	v.y = setupInfo->cameraPlacement.pointOfInterest.y - setupInfo->cameraPlacement.cameraLocation.y;
	v.z = setupInfo->cameraPlacement.pointOfInterest.z - setupInfo->cameraPlacement.cameraLocation.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);

	gEarCoords.x = setupInfo->cameraPlacement.cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
	gEarCoords.y = setupInfo->cameraPlacement.cameraLocation.y + (v.y * 300.0f);
	gEarCoords.z = setupInfo->cameraPlacement.cameraLocation.z + (v.z * 300.0f);
	
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

/***************************** CALLBACKFN ***************************/
//  
// Called by the Sound Manager at interrupt time to let us know that
// the sound is done playing.
//

#if 0
static pascal void CallBackFn (SndChannelPtr chan, SndCommand *cmd) 
{
SndCommand      theCmd;

    // Play the sound again (loop it)

    theCmd.cmd = bufferCmd;
    theCmd.param1 = 0;
    theCmd.param2 = cmd->param2;
	SndDoCommand (chan, &theCmd, true);

    // Just reuse the callBackCmd that got us here in the first place
    SndDoCommand (chan, cmd, true);
}
#endif

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
static UInt32          loopStart, loopEnd;
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

	lv2 = (float)leftVolume * gGlobalVolume;							// amplify by global volume
	rv2 = (float)rightVolume * gGlobalVolume;					


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
	myErr = SndDoCommand(chanPtr, &mySndCmd, true);


	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
    SndDoCommand(chanPtr, &mySndCmd, true);
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
			
	/*if (gSongPlayingFlag)
		SetMovieVolume(gSongMovie, FloatToFixed16(gGlobalVolume) * gSongVolumeTweaks[gCurrentSong]);*/
	

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

	lv2 = leftVol * gGlobalVolume;				// amplify by global volume
	rv2 = rightVol * gGlobalVolume;			

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
#if 0
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
#endif
		}				


	}



		/* ALSO CHECK OPTIONS */

	if (GetNewKeyState_Real(KEY_F1))
	{
		DoGameOptionsDialog();
	}

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


















