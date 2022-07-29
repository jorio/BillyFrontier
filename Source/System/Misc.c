/****************************/
/*      MISC ROUTINES       */
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/


//#include <DriverServices.h>
//#include <osutils.h>
//#include <timer.h>
//#include 	<DrawSprocket.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "game.h"
#include "Pomme.h"

extern	short		gMainAppRezFile;
extern	Boolean		gISPInitialized,gOSX,gG4;
extern	Boolean		gISpActive, gShareware;
//extern	DSpContextReference 	gDisplayContext;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int			gPolysThisFrame;
extern	SDL_GLContext		gAGLContext;
//extern	AGLDrawable		gAGLWin;
extern	float			gDemoVersionTimer;
extern	short	gPrefsFolderVRefNum;
extern	long		gPrefsFolderDirID;
extern	PrefsType			gGamePrefs;
extern	FSSpec				gDataSpec;

/****************************/
/*    CONSTANTS             */
/****************************/

#define		ERROR_ALERT_ID		401


#define	USE_MALLOC		1



/**********************/
/*     VARIABLES      */
/**********************/


u_long 	gSeed0 = 0, gSeed1 = 0, gSeed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;


Boolean	gGameIsRegistered = false;

unsigned char	gRegInfo[64];

Boolean	gSlowCPU = false;

Boolean	gSerialWasVerified = false;

Boolean	gLittleSnitch = false;

Str255  gSerialFileName = ":Billy:Info";


/**********************/
/*     PROTOTYPES     */
/**********************/

static void DoSerialDialog(unsigned char *out);


#include "serialVerify.h"


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

	Enter2D(true);


	/*if (gDisplayContext)
		GammaOn();*/
	MyFlushEvents();
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!
	UseResFile(gMainAppRezFile);
	NumToString(err, numStr);
	DoAlert (numStr);
	
//	if (gOSX)
//		DebugStr("ShowSystemErr has been called");
	
	Exit2D();
	
	CleanQuit();
}

/****************** DO SYSTEM ERROR : NONFATAL ***************/
//
// nonfatal
//
void ShowSystemErr_NonFatal(long err)
{
Str255		numStr;

	Enter2D(true);


	/*if (gDisplayContext)
		GammaOn();*/
	MyFlushEvents();
	NumToString(err, numStr);
	DoAlert (numStr);
	
	Exit2D();
			
}


/*********************** DO ALERT *******************/

void DoAlert(const char* format, ...)
{
	Enter2D(true);

	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	printf("CMR Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Billy Frontier", message, /*gSDLWindow*/NULL);

	Exit2D();
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* format, ...)
{
	Enter2D(true);

	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	printf("CMR Fatal Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Billy Frontier", message, /*gSDLWindow*/NULL);

	Exit2D();
	CleanQuit();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{	
static Boolean	beenHere = false;

	if (!beenHere)
	{
		beenHere = true;
		
		DeleteAllObjects();
		DisposeAllBG3DContainers();						// nuke all models
		DisposeAllSpriteGroups();						// nuke all sprites

		if (gGameViewInfoPtr)							// see if need to dispose this
			OGL_DisposeWindowSetup(&gGameViewInfoPtr);

		if (!gGameIsRegistered)
		{
			GammaFadeOut();
			ShowDemoQuitScreen();

		}

		ShutdownSound();								// cleanup sound stuff
	}

	GameScreenToBlack();
	CleanupDisplay();								// unloads Draw Sprocket
    
	if (gISPInitialized)							// unload ISp
		ISpShutdown();

	UseResFile(gMainAppRezFile);
	
	InitCursor();
	MyFlushEvents();

	SavePrefs();							// save prefs before bailing

	ExitToShell();		
}



#pragma mark -


/******************** MY RANDOM LONG **********************/
//
// My own random number generator that returns a LONG
//
// NOTE: call this instead of MyRandomShort if the value is going to be
//		masked or if it just doesnt matter since this version is quicker
//		without the 0xffff at the end.
//

unsigned long MyRandomLong(void)
{
  return gSeed2 ^= (((gSeed1 ^= (gSeed2>>5)*1568397607UL)>>7)+
                   (gSeed0 = (gSeed0+1)*3141592621UL))*2435386481UL;
}


/************************* RANDOM RANGE *************************/
//
// THE RANGE *IS* INCLUSIVE OF MIN AND MAX
//

u_short	RandomRange(unsigned short min, unsigned short max)
{
u_short		qdRdm;											// treat return value as 0-65536
u_long		range, t;

	qdRdm = MyRandomLong();
	range = max+1 - min;
	t = (qdRdm * range)>>16;	 							// now 0 <= t <= range
	
	return( t+min );
}



/************** RANDOM FLOAT ********************/
//
// returns a random float between 0 and 1
//

float RandomFloat(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;		
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (1.0f/(float)0xfff);			// get # between 0..1
	return(f);
} 
 

/************** RANDOM FLOAT 2 ********************/
//
// returns a random float between -1 and +1
//

float RandomFloat2(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;		
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (2.0f/(float)0xfff);			// get # between 0..2
	f -= 1.0f;								// get -1..+1
	return(f);
} 



/**************** SET MY RANDOM SEED *******************/

void SetMyRandomSeed(unsigned long seed)
{
	gSeed0 = seed;
	gSeed1 = 0;
	gSeed2 = 0;	
	
}

/**************** INIT MY RANDOM SEED *******************/

void InitMyRandomSeed(void)
{
	gSeed0 = 0x2a80ce30;
	gSeed1 = 0;
	gSeed2 = 0;	
}


#pragma mark -


/******************* FLOAT TO STRING *******************/

void FloatToString(float num, Str255 string)
{
Str255	sf;
long	i,f;

	i = num;						// get integer part
	
	
	f = (fabs(num)-fabs((float)i)) * 10000;		// reduce num to fraction only & move decimal --> 5 places	

	if ((i==0) && (num < 0))		// special case if (-), but integer is 0
	{
		string[0] = 2;
		string[1] = '-';
		string[2] = '0';
	}
	else
		NumToString(i,string);		// make integer into string
		
	NumToString(f,sf);				// make fraction into string
	
	string[++string[0]] = '.';		// add "." into string
	
	if (f >= 1)
	{
		if (f < 1000)
			string[++string[0]] = '0';	// add 1000's zero
		if (f < 100)
			string[++string[0]] = '0';	// add 100's zero
		if (f < 10)
			string[++string[0]] = '0';	// add 10's zero
	}
	
	for (i = 0; i < sf[0]; i++)
	{
		string[++string[0]] = sf[i+1];	// copy fraction into string
	}
}

/*********************** STRING TO FLOAT *************************/

float StringToFloat(Str255 textStr)
{
short	i;
short	length;
Byte	mode = 0;
long	integer = 0;
long	mantissa = 0,mantissaSize = 0;
float	f;
float	tens[8] = {1,10,100,1000,10000,100000,1000000,10000000};
char	c;
float	sign = 1;												// assume positive

	length = textStr[0];										// get string length

	if (length== 0)												// quick check for empty
		return(0);


			/* SCAN THE NUMBER */

	for (i = 1; i <= length; i++)
	{
		c  = textStr[i];										// get this char
		
		if (c == '-')											// see if negative
		{
			sign = -1;
			continue;
		}
		else
		if (c == '.')											// see if hit the decimal
		{
			mode = 1;
			continue;
		}
		else
		if ((c < '0') || (c > '9'))								// skip all but #'s
			continue;
	
	
		if (mode == 0)
			integer = (integer * 10) + (c - '0');
		else
		{
			mantissa = (mantissa * 10) + (c - '0');
			mantissaSize++;
		}
	}

			/* BUILT A FLOAT FROM IT */
			
	f = (float)integer + ((float)mantissa/tens[mantissaSize]);
	f *= sign;

	return(f);
}





#pragma mark -

/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
Handle	hand;
OSErr	err;

	hand = NewHandle(size);							// alloc in APPL
	if (hand == nil)
	{
		DoAlert("AllocHandle: using temp mem");
		hand = TempNewHandle(size,&err);			// try TEMP mem
		if (hand == nil)
		{
			DoAlert("AllocHandle: failed!");
			return(nil);
		}
		else
			return(hand);							
	}
	return(hand);		
								
}



/****************** ALLOC PTR ********************/

void *AllocPtr(long size)
{
Ptr	pr;
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = malloc(size);
#else
	pr = NewPtr(size);
#endif	
	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr++ = 'FACE';
	*cookiePtr++ = 'PTR2';
	*cookiePtr++ = 'PTR3';
	*cookiePtr = 'PTR4';

	pr += 16;
	
	gNumPointers++;
	
	return(pr);
}


/****************** ALLOC PTR CLEAR ********************/

void *AllocPtrClear(long size)
{
Ptr	pr;
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = calloc(1, size);
#else
	pr = NewPtrClear(size);						// alloc in Application
#endif	

	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr++ = 'FACE';
	*cookiePtr++ = 'PTC2';
	*cookiePtr++ = 'PTC3';
	*cookiePtr = 'PTC4';

	pr += 16;
	
	gNumPointers++;
	
	return(pr);
}


/***************** SAFE DISPOSE PTR ***********************/

void SafeDisposePtr(void *ptr)
{
u_long	*cookiePtr;
Ptr		p = ptr;

	p -= 16;					// back up to pt to cookie
	
	cookiePtr = (u_long *)p;
	
	if (*cookiePtr != 'FACE')
		DoFatalAlert("SafeSafeDisposePtr: invalid cookie!");
		
	*cookiePtr = 0;
	
#if USE_MALLOC	
	free(p);
#else
	DisposePtr(p);
#endif
	
	gNumPointers--;
}



#pragma mark -

/******************* COPY P STRING ********************/

void CopyPString(Str255 from, Str255 to)
{
short	i,n;

	n = from[0];			// get length
	
	for (i = 0; i <= n; i++)
		to[i] = from[i];

}


/***************** P STRING TO C ************************/

void PStringToC(char *pString, char *cString)
{
Byte	pLength,i;

	pLength = pString[0];
	
	for (i=0; i < pLength; i++)					// copy string
		cString[i] = pString[i+1];
		
	cString[pLength] = 0x00;					// add null character to end of c string
}


/***************** DRAW C STRING ********************/

void DrawCString(char *string)
{
	while(*string != 0x00)
		DrawChar(*string++);
}


/******************* VERIFY SYSTEM ******************/

void VerifySystem(void)
{
	gOSX = true;
	gSlowCPU = false;

#if 0
OSErr	iErr;
long		 cpuFamily, cpuSpeed;
NumVersion	vers;
short		i;



		/* REQUIRE CARBONLIB 1.4 */

	iErr = Gestalt(gestaltCarbonVersion,(long *)&vers);
	if (iErr)
	{
		ShowSystemErr_NonFatal(iErr);
		goto carbonerr;
	}
	if (vers.stage == 1)
	{
		if (vers.nonRelRev < 0x40)
		{
carbonerr:		
			DoFatalAlert("This application requires CarbonLib 1.4 or newer.  Run Software Update to get the latest version");
		}
	}



			/* SEE IF PROCESSOR IS G4 OR NOT */

	gSlowCPU = false;														// assume not slow
				
	iErr = Gestalt(gestaltNativeCPUfamily,&cpuFamily);
	if (iErr != noErr)
		DoFatalAlert("VerifySystem: gestaltNativeCPUfamily failed!");
	
	if (cpuFamily >= gestaltCPUG4)
		gG4 = true;
	else
		gG4 = false;

	if (!gG4)																// if not G4, check processor speed to see if on really fast G3
	{
		iErr = Gestalt(gestaltProcClkSpeed,&cpuSpeed);							
		if (iErr == noErr)
		{
			if ((cpuSpeed/1000000) >= 600)										// must be at least 600mhz G3 for us to treat it like a G4
				gG4 = true;
			else
			if ((cpuSpeed/1000000) <= 450)										// if 450 or less then it's a slow G3
				gSlowCPU = true;
		}
	}


		/* DETERMINE IF RUNNING ON OS X */

	iErr = Gestalt(gestaltSystemVersion,(long *)&vers);
	if (iErr != noErr)
		DoFatalAlert("VerifySystem: gestaltSystemVersion failed!");
				
	if (vers.stage >= 0x10)													// see if at least OS 10
	{
		gOSX = true;
		
		if ((vers.stage == 0x10) && (vers.nonRelRev < 0x20))				// must be at least OS 10.1 !!!
		{
			if (!gGamePrefs.oldOSWarned)
			{
				DoAlert("This game requires MacOS 10.2 or later.  It might run for you on 10.1, but it will probably freeze up your computer at some point.");
				gGamePrefs.oldOSWarned = true;
			}
		}
	}
	else
	{
		gOSX = false;
		if (vers.stage < 9)						// check for OS 9
			DoFatalAlert("This game will not run on MacOS 8.");
		else
		if (!gGamePrefs.oldOSWarned)
		{
			DoAlert("This game requires MacOS 10.2 or later.  It might run for you on MacOS 9.2.2, however, we only support it on 10.2 or later.");
			gGamePrefs.oldOSWarned = true;
		}
	}
	

			/* CHECK TIME-BOMB */
	{
		unsigned long secs;
		DateTimeRec	d;

		GetDateTime(&secs);
		SecondsToDate(secs, &d);
		
		if ((d.year > 2003) ||
			((d.year == 2003) && (d.month > 7)))
		{
			DoFatalAlert("Sorry, but this beta has expired");
		}
	}



			/* CHECK OPENGL */
			
	if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) aglChoosePixelFormat) 				// check for existance of OpenGL
		DoFatalAlert("This application needs OpenGL to function.  Please install OpenGL and try again.");
			

		/* CHECK SPROCKETS */
		
	if (!gOSX)
	{
		if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) ISpStartup) 							// check for existance of Input Sprocket
			DoFatalAlert("This application needs Input Sprocket to function.  Please install Game Sprockets and try again.");
	
		if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) DSpStartup) 							// check for existance of Draw Sprocket
			DoFatalAlert("This application needs Draw Sprocket to function.  Please install Game Sprockets and try again.");

	}
	
	
		/***********************************/
		/* SEE IF LITTLE-SNITCH IS RUNNING */
		/***********************************/
	
	{	
		ProcessSerialNumber psn = {kNoProcess, kNoProcess};
		ProcessInfoRec	info;
		
		Str255		s;
		const char snitch[] = "LittleSnitchDaemon";
		
		info.processName = s;
		info.processInfoLength = sizeof(ProcessInfoRec);
		info.processAppSpec = nil;
		
		while(GetNextProcess(&psn) == noErr)
		{
			char	pname[256];
			char	*matched;
		
			iErr = GetProcessInformation(&psn, &info);
			if (iErr)
				break;

			p2cstrcpy(pname, &s[0]);					// convert pstring to cstring
			
			matched = strstr (pname, "Snitc");			// does "Snitc" appear anywhere in the process name?
			if (matched != nil)
			{
				gLittleSnitch = true;
				break;
			}
		}
	}
#endif
}


/******************** REGULATE SPEED ***************/

void RegulateSpeed(short fps)
{
short	n;
static oldTick = 0;
	
	n = 60 / fps;
	while ((TickCount() - oldTick) < n) {}			// wait for n ticks
	oldTick = TickCount();							// remember current time
}


/************* COPY PSTR **********************/

void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr)
{
short	dataLen = inSourceStr[0] + 1;
	
	BlockMoveData(inSourceStr, outDestStr, dataLen);
	outDestStr[0] = dataLen - 1;
}





#pragma mark -



/************** CALC FRAMES PER SECOND *****************/
//
// This version uses UpTime() which is only available on PCI Macs.
//

void CalcFramesPerSecond(void)
{
#if 0
static float		wakeTimer =0;
AbsoluteTime currTime,deltaTime;
static AbsoluteTime time = {0,0};
Nanoseconds	nano;

do_again:
	currTime = UpTime();

	deltaTime = SubAbsoluteFromAbsolute(currTime, time);
	nano = AbsoluteToNanoseconds(deltaTime);

	gFramesPerSecond = 1000000.0f / (float)nano.lo;
	gFramesPerSecond *= 1000.0f;

	if (gFramesPerSecond < MIN_FPS)					// keep at a minimum
		gFramesPerSecond = MIN_FPS;
	else
	if (gFramesPerSecond > MAX_FPS)					// if over maximum then wait
		goto do_again;
		
	gFramesPerSecondFrac = 1.0f/gFramesPerSecond;		// calc fractional for multiplication


	time = currTime;	// reset for next time interval
	
	
	
			/* PREVENT SYSTEM SLEEP ON OS X */
			
	wakeTimer -= gFramesPerSecondFrac;
	if (wakeTimer <= 0.0f)
	{
		wakeTimer += 25.0f;
		UpdateSystemActivity(OverallAct);
	}
#endif
}


/********************* IS POWER OF 2 ****************************/

Boolean IsPowerOf2(int num)
{
int		i;
	
	i = 2;
	do
	{
		if (i == num)				// see if this power of 2 matches
			return(true);
		i *= 2;						// next power of 2	
	}while(i <= num);				// search until power is > number			

	return(false);
}

#pragma mark-

/******************* MY FLUSH EVENTS **********************/
//
// This call makes damed sure that there are no events anywhere in any event queue.
//

void MyFlushEvents(void)
{
//EventRecord 	theEvent;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);	
	FlushEventQueue(GetMainEventQueue());

#if 0
			/* POLL EVENT QUEUE TO BE SURE THINGS ARE FLUSHED OUT */
			
	while (GetNextEvent(mDownMask|mUpMask|keyDownMask|keyUpMask|autoKeyMask, &theEvent));


	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);	
	FlushEventQueue(GetMainEventQueue());
#endif	
}


#pragma mark -



/******************** PSTR CAT / COPY *************************/

StringPtr PStrCat(StringPtr dst, ConstStr255Param   src)
{
SInt16 size = src[0];
	
	if (0xff - dst[0] < size)
		size = 0xff - dst[0];
	
	BlockMoveData(&src[1], &dst[dst[0]], size);
	dst[0] = dst[0] + size;
	
	return dst;
}

StringPtr PStrCopy(StringPtr dst, ConstStr255Param   src)
{
	dst[0] = src[0]; BlockMoveData(&src[1], &dst[1], src[0]); return dst;
}





