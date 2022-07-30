/****************************/
/*   	  INPUT.C	   	    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/**********************/
/*     PROTOTYPES     */
/**********************/

static void ReadInputValues(KeyMap *keyMap);
//static pascal OSStatus MyMouseEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata);
static void Install_MouseEventHandler(void);
static void Remove_MouseEventHandlers(void);


/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

Boolean		gMouseButtonState = false, gMouseNewButtonState = false;
Boolean		gMouseRightButtonDown = false, gMouseMiddleButtonDown = false;
SInt32		gScrollWheelDelta = 0;


#if 0
static	EventHandlerUPP			gMouseEventHandlerUPP = nil;
static	EventHandlerRef			gMouseEventHandlerRef = 0;
#endif

long					gMouseDeltaX = 0;
long					gMouseDeltaY = 0;

float			gMouseX = 640/2, gMouseY = 480/2;		// only used with ISp

static	float					gReadMouseDeltasTimer = 0;

KeyMap gKeyMap,gNewKeys,gOldKeys,gKeyMap_Real,gNewKeys_Real,gOldKeys_Real;


Boolean	gISpActive 				= false;							
Boolean	gISPInitialized			= false;


		/* CONTORL NEEDS */
		
#define	NEED_NUM_MOUSEMOTION	0

#if 0
ISpNeed	gControlNeeds[NUM_CONTROL_NEEDS] =
{
	{										// 0
		"Mouse Movement Left & Right",
		132,
		0,
		0,
		kISpElementKind_Delta,
		kISpElementLabel_Delta_X,
		0,
		0,
		0,
		0
	},
	
	{										// 1
		"Mouse Movement Up & Down",
		133,
		0,
		0,
		kISpElementKind_Delta,
		kISpElementLabel_Delta_Y,
		0,
		0,
		0,
		0
	},

	{										// 2
		"Mouse Button",
		133,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Fire,
		0,
		0,
		0,
		0
	},
	
};


static ISpElementReference	gVirtualElements[NUM_CONTROL_NEEDS];
#endif



/************************* INIT INPUT *********************************/

void InitInput(void)
{
#if 0
OSErr				iErr;
ISpDeviceReference	dev[10];
UInt32				count = 0;

	
			/********************************************/
			/* SEE IF USE INPUT SPROCKET OR HID MANAGER */
			/********************************************/
	
				/* OS X */
						
	if (gOSX)
	{
		Install_MouseEventHandler();			// install our carbon even mouse handler		
//		MyInitHID();
	}
				/* OS 9 */
	else
	{
		ISpStartup();
		gISPInitialized = true;

					/* CREATE NEW NEEDS */
		
		iErr = ISpElement_NewVirtualFromNeeds(NUM_CONTROL_NEEDS, gControlNeeds, gVirtualElements, 0);
		if (iErr)
			DoFatalAlert("InitInput: ISpElement_NewVirtualFromNeeds failed!");
			
		iErr = ISpInit(NUM_CONTROL_NEEDS, gControlNeeds, gVirtualElements, kGameID,'Inp1', 0, 1000, 0);
		if (iErr)
			DoFatalAlert("InitInput: ISpInit failed!");
		
			
				/* ACTIVATE ALL DEVICES */

		if (ISpDevices_Extract(10,&count,dev) != noErr)
			DoFatalAlert("InitInput: ISpDevices_Extract failed!");
			
		if (ISpDevices_Activate(count, dev) != noErr)
			DoFatalAlert("InitInput: ISpDevices_Activate failed!");
			
		gISpActive = true;
			

	}
	
	
	if (!gOSX)
	{
		ReadKeyboard();
		if ((!gGamePrefs.hasConfiguredISpControls) || (GetKeyState_Real(KEY_APPLE)))					// see if need to do initial control setup
		{
			DoKeyConfigDialog();	
			gGamePrefs.hasConfiguredISpControls = true;
			SavePrefs();
		}
	}
#endif
}

/**************** READ KEYBOARD *************/
//
//
//

void ReadKeyboard(void)
{
u_short	pauseKey;

			/* READ REAL KEYBOARD & INPUT SPROCKET KEYMAP */
			//
			// Note:  	This will convert ISp data into a KeyMap, so the rest
			// 			of the code looks like its just reading the real keyboard.
			//
			
	ReadInputValues(&gKeyMap);


			/* CALC WHICH KEYS ARE NEW THIS TIME */
			
	gNewKeys[0] = (gOldKeys[0] ^ gKeyMap[0]) & gKeyMap[0];
	gNewKeys[1] = (gOldKeys[1] ^ gKeyMap[1]) & gKeyMap[1];
	gNewKeys[2] = (gOldKeys[2] ^ gKeyMap[2]) & gKeyMap[2];
	gNewKeys[3] = (gOldKeys[3] ^ gKeyMap[3]) & gKeyMap[3];



				/* SEE IF QUIT GAME */
				
	if (GetKeyState_Real(KEY_Q) && GetKeyState_Real(KEY_APPLE))			// see if real key quit
		CleanQuit();	



			/* SEE IF DO SAFE PAUSE FOR SCREEN SHOTS */
			
//	if (gOSX)
		pauseKey = KEY_F15;
//	else
//		pauseKey = KEY_F12;
		
	if (GetNewKeyState_Real(pauseKey))
	{
		Boolean o = gISpActive;
		TurnOffISp();

#if 1
		IMPLEMENT_ME_SOFT();
#else
		if (gAGLContext)
			aglSetDrawable(gAGLContext, nil);			// diable gl
#endif
				
		do
		{
#if 0
			EventRecord	e;
			WaitNextEvent(everyEvent,&e, 0, 0);
#endif
			ReadKeyboard_Real();
		}while(!GetNewKeyState_Real(pauseKey));
		if (o)
			TurnOnISp();
			
		if (gAGLContext)
			//aglSetDrawable(gAGLContext, gAGLWin);		// reenable gl	
			
		CalcFramesPerSecond();
		CalcFramesPerSecond();		
	}
		
		
			/* REMEMBER AS OLD MAP */

	gOldKeys[0] = gKeyMap[0];
	gOldKeys[1] = gKeyMap[1];
	gOldKeys[2] = gKeyMap[2];
	gOldKeys[3] = gKeyMap[3];
}


/**************** READ KEYBOARD_REAL *************/
//
// This just does a simple read of the REAL keyboard (regardless of Input Sprockets)
//

void ReadKeyboard_Real(void)
{
	GetKeys(gKeyMap_Real);										

			/* CALC WHICH KEYS ARE NEW THIS TIME */
			
	gNewKeys_Real[0] = (gOldKeys_Real[0] ^ gKeyMap_Real[0]) & gKeyMap_Real[0];
	gNewKeys_Real[1] = (gOldKeys_Real[1] ^ gKeyMap_Real[1]) & gKeyMap_Real[1];
	gNewKeys_Real[2] = (gOldKeys_Real[2] ^ gKeyMap_Real[2]) & gKeyMap_Real[2];
	gNewKeys_Real[3] = (gOldKeys_Real[3] ^ gKeyMap_Real[3]) & gKeyMap_Real[3];


			/* REMEMBER AS OLD MAP */

	gOldKeys_Real[0] = gKeyMap_Real[0];
	gOldKeys_Real[1] = gKeyMap_Real[1];
	gOldKeys_Real[2] = gKeyMap_Real[2];
	gOldKeys_Real[3] = gKeyMap_Real[3];
}


/****************** GET KEY STATE: REAL ***********/
//
// for data from ReadKeyboard_Real
//

Boolean GetKeyState_Real(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gKeyMap_Real;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}

/****************** GET NEW KEY STATE: REAL ***********/
//
// for data from ReadKeyboard_Real
//

Boolean GetNewKeyState_Real(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gNewKeys_Real;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}


/****************** GET KEY STATE ***********/
//
// NOTE: Assumes that ReadKeyboard has already been called!!
//

#if 0
Boolean GetKeyState(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gKeyMap;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}

/****************** GET NEW KEY STATE ***********/
//
// NOTE: Assumes that ReadKeyboard has already been called!!
//

Boolean GetNewKeyState(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gNewKeys;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}

#endif


/********************** READ INPUT VALUES ******************************/
//
// Depending on mode, will either read key map from GetKeys or
// will "fake" a keymap using Input Sprockets.
//

void ReadInputValues(KeyMap *keyMap)
{
keyMap;

	ReadKeyboard_Real();												// always read real keyboard anyway

	
	
			/***********************************************/
			/* IF NO ANALOG INPUT, THEN CHECK MOUSE DELTAS */
			/***********************************************/

				/* READ DELTAS REGARDLESS */

	GetMouseDeltas();							// get new deltas

	
	
			/*********************/
			/* READ MOUSE BUTTON */
			/*********************/

				/* OS X */
							
	if (gOSX)
	{
		if (Button())							// is mouse button down?
		{
			if (!gMouseButtonState)				// is this a new click?
				gMouseNewButtonState = gMouseButtonState = true;	
			else
				gMouseNewButtonState = false;	
		}
		else
		{
			gMouseButtonState = gMouseNewButtonState = false;	
		}
	}
	
#if 0
				/* OS 9 */
	else
	{
		UInt32	state;		
		ISpElement_GetSimpleState(gVirtualElements[2], &state);
		if (state)
		{
			if (!gMouseButtonState)				// is this a new click?
				gMouseNewButtonState = gMouseButtonState = true;	
			else
				gMouseNewButtonState = false;	
		}
		else
		{
			gMouseButtonState = gMouseNewButtonState = false;	
		}
	
	}
#endif
}




#pragma mark -

/******************** TURN ON ISP *********************/

void TurnOnISp(void)
{
#if 0
ISpDeviceReference	dev[10];
UInt32		count = 0;
OSErr		iErr;

	if (!gISPInitialized)
		return;

	if (!gISpActive)
	{
		ISpResume();
		gISpActive = true;
		
				/* ACTIVATE ALL DEVICES */

		iErr = ISpDevices_Extract(10,&count,dev);
		if (iErr)
			DoFatalAlert("TurnOnISp: ISpDevices_Extract failed!");
		iErr = ISpDevices_Activate(count, dev);
		if (iErr)
			DoFatalAlert("TurnOnISp: ISpDevices_Activate failed!");

	}
#endif
}

/******************** TURN OFF ISP *********************/

void TurnOffISp(void)
{
#if 0
ISpDeviceReference	dev[10];
UInt32		count = 0;

	if (!gISPInitialized)
		return;

	if (gISpActive)
	{	
				/* DEACTIVATE ALL DEVICES */

		ISpDevices_Extract(10,&count,dev);
		ISpDevices_Deactivate(count, dev);
		ISpSuspend();		
	
		gISpActive = false;
	}
#endif
}



/************************ DO KEY CONFIG DIALOG ***************************/
//
// NOTE:  ISp must be turned ON when this is called!
//

void DoKeyConfigDialog(void)
{
	MyFlushEvents();

	if (gOSX)
		return;

#if 0
				/* DO ISP CONFIG DIALOG */
	else
	{		
		Boolean	ispWasOn = gISpActive;
		
		if (gAGLContext)
			aglSetDrawable(gAGLContext, nil);			// diable gl for dialog

		TurnOnISp();
		InitCursor();
		ISpConfigure(nil);	
		if (!ispWasOn)
			TurnOffISp();

		if (gAGLContext)
			//aglSetDrawable(gAGLContext, gAGLWin);		// reenable gl
			
		HideCursor();
	}
#endif
}


/***************** ARE ANY NEW KEYS PRESSED ****************/

Boolean AreAnyNewKeysPressed(void)
{
	if (gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3])
		return(true);
	else
		return(false);
}


#pragma mark -

/***************** GET MOUSE COORD *****************/

void GetMouseCoord(Point *point)
{

	if (gOSX)
		GetMouse(point);
#if 0
	else
	{	
		gMouseX += gMouseDeltaX * .13f;
		gMouseY += gMouseDeltaY * .13f;
	
		if (gMouseX < 0.0f)
			gMouseX = 0.0f;
		else
		if (gMouseX > gGameWindowWidth)
			gMouseX = gGameWindowWidth;

		if (gMouseY < 0.0f)
			gMouseY = 0.0f;
		else
		if (gMouseY > gGameWindowHeight)
			gMouseY = gGameWindowHeight;
	
		point->h = gMouseX;
		point->v = gMouseY;
	}
#endif
}


/***************** GET MOUSE DELTA *****************/

void GetMouseDeltas(void)
{
#if 0
			/******************************************/
			/* USE CARBON EVENTS TO READ MOUSE DELTAS */
			/******************************************/
			
	if (gOSX)
	{
		EventRecord   theEvent;

		gReadMouseDeltasTimer -= gFramesPerSecondFrac;			// regulate the rate that we read mouse deltas so we don't go too fast and get just 0's
		if (gReadMouseDeltasTimer <= 0.0f)
		{
			gReadMouseDeltasTimer += .1f;						// read 10 times per second
	

			gMouseDeltaX = gMouseDeltaY = 0;		 			// assume none
		  
			WaitNextEvent(everyEvent, &theEvent, 0, nil);		// trigger a mouse event if any
		}
	}
	
					/******************************/
					/* READ MOUSE DELTAS FROM ISP */
					/******************************/
	else
	{
		ISpDeltaData	deltaX,deltaY;
		OSStatus		err;
		float			dFactor;
	
	
		if (!gISpActive)									// make sure ISp active
		{
			gMouseDeltaX = gMouseDeltaY = 0;
			return;
		}
					
		
						/* DX */
						
		err = ISpElement_GetComplexState(gVirtualElements[NEED_NUM_MOUSEMOTION], sizeof(ISpDeltaData), &deltaX);																	
		if (err)
			DoFatalAlert("GetMouseDelta: ISpElement_GetComplexState failed!");


						/* DY */
						
		err = ISpElement_GetComplexState(gVirtualElements[NEED_NUM_MOUSEMOTION+1], sizeof(ISpDeltaData), &deltaY);																	
		if (err)
			DoFatalAlert("GetMouseDelta: ISpElement_GetComplexState failed!");

		dFactor = gGameWindowWidth * ((1.0/1600.0) * .06);

		gMouseDeltaX = deltaX * dFactor;		
		gMouseDeltaY = -deltaY * dFactor; 
	}
#endif
}


/**************** MY MOUSE EVENT HANDLER *************************/
//
// Every time WaitNextEvent() is called this callback will be invoked.
//

#if 0
static pascal OSStatus MyMouseEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata)
{
OSStatus	result = eventNotHandledErr;
OSStatus	theErr = noErr;
Point		qdPoint;
UInt16 		whichButton;

eventhandler; userdata;

	switch (GetEventKind(pEventRef))
	{	 
		case	kEventMouseMoved:
		case	kEventMouseDragged:
				theErr = GetEventParameter(pEventRef, kEventParamMouseDelta, typeQDPoint,
										nil, sizeof(Point), nil, &qdPoint);
			
				gMouseDeltaX = qdPoint.h;
				gMouseDeltaY = qdPoint.v;

				result = noErr;
				break;	
				
				
		case	kEventMouseWheelMoved:										
				theErr = GetEventParameter(pEventRef, kEventParamMouseWheelDelta, typeSInt32, nil, sizeof(gScrollWheelDelta), nil, &gScrollWheelDelta);		// Get the delta
				result = noErr;
				break;
				
		case	kEventMouseDown:
				theErr = GetEventParameter(pEventRef, kEventParamMouseButton, typeMouseButton, nil ,sizeof(whichButton), nil, &whichButton); 	// see if 2nd or 3rd button was pressed
				if (theErr == noErr)
				{
					switch(whichButton)
					{
						case	kEventMouseButtonSecondary:							// right button?
								gMouseRightButtonDown = true;
								break;
								
						case	kEventMouseButtonTertiary:							// middle button?
								gMouseMiddleButtonDown = true;
								break;
					}								
				}
//				result = noErr;	// NOTE:  DO *NOT* SET RESULT TO NOERR BECAUSE WE WANT THESE CLICKS TO GET PASSED TO THE SYSTEM FOR WINDOW MOVING ETC.
				break;


		case	kEventMouseUp:
				theErr = GetEventParameter(pEventRef, kEventParamMouseButton, typeMouseButton, nil ,sizeof(whichButton), nil, &whichButton); 	// see if 2nd or 3rd button was pressed
				if (theErr == noErr)
				{
					switch(whichButton)
					{
						case	kEventMouseButtonSecondary:							// right button?
								gMouseRightButtonDown = false;
								break;
								
						case	kEventMouseButtonTertiary:							// middle button?
								gMouseMiddleButtonDown = false;
								break;
					}								
				}
//				result = noErr;	// NOTE:  DO *NOT* SET RESULT TO NOERR BECAUSE WE WANT THESE CLICKS TO GET PASSED TO THE SYSTEM FOR WINDOW MOVING ETC.
				break;
	}	 
     return(result);
}
#endif

/******************* INSTALL MOUSE EVENT HANDLER ***********************/

void Install_MouseEventHandler(void)
{
#if 0
EventTypeSpec			mouseEvents[5] ={{kEventClassMouse, kEventMouseMoved},
										{kEventClassMouse, kEventMouseDragged},
										{kEventClassMouse, kEventMouseUp},
										{kEventClassMouse, kEventMouseDown},
										{kEventClassMouse, kEventMouseWheelMoved}};

	// Set up the handler to capture mouse movements.



	if (gMouseEventHandlerUPP == nil)
		gMouseEventHandlerUPP = NewEventHandlerUPP(MyMouseEventHandler);

	if (gMouseEventHandlerRef == 0)
	{	
		//	make sure we start the deltas at 0 when we setup the handler
			
		gMouseDeltaX = 0;
		gMouseDeltaY = 0;
		
		//	install the handler

		InstallEventHandler(GetApplicationEventTarget(), gMouseEventHandlerUPP,	5,
							mouseEvents, nil, &gMouseEventHandlerRef);
			
	}
#endif
}


/******************* REMOVE MOUSE EVENT HANDLER *************************/

void Remove_MouseEventHandlers(void)
{
#if 0
	//	if the handler has been installed, remove it

	if (gMouseEventHandlerRef != nil)
	{
		RemoveEventHandler(gMouseEventHandlerRef);
		gMouseEventHandlerRef = nil;
		
		DisposeEventHandlerUPP(gMouseEventHandlerUPP);
		gMouseEventHandlerUPP = nil;
	}
#endif
}





