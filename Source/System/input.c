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
static void Install_MouseEventHandler(void);
static void Remove_MouseEventHandlers(void);


/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

//Boolean		gMouseButtonState = false, gMouseNewButtonState = false;
//Boolean		gMouseRightButtonDown = false, gMouseMiddleButtonDown = false;
//SInt32		gScrollWheelDelta = 0;

long					gMouseDeltaX = 0;
long					gMouseDeltaY = 0;

float			gMouseX = 640/2, gMouseY = 480/2;		// only used with ISp

static	float					gReadMouseDeltasTimer = 0;

Boolean	gISpActive 				= false;							
Boolean	gISPInitialized			= false;


		/* CONTORL NEEDS */
		
#define	NEED_NUM_MOUSEMOTION	0



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
		if ((!gGamePrefs.hasConfiguredISpControls) || (GetKeyState(SDL_SCANCODE_APPLE)))					// see if need to do initial control setup
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
	DoSDLMaintenance();

#if 0
			/* SEE IF DO SAFE PAUSE FOR SCREEN SHOTS */
			
	uint16_t pauseKey = KEY_F12;

	if (GetNewKeyState_Real(pauseKey))
	{
		Boolean o = gISpActive;
		TurnOffISp();

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
#endif
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


#if 0
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
#endif



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


#if 0
/***************** ARE ANY NEW KEYS PRESSED ****************/

Boolean AreAnyNewKeysPressed(void)
{
	if (gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3])
		return(true);
	else
		return(false);
}
#endif


#pragma mark -

/***************** GET MOUSE COORD *****************/

void GetMouseCoord(Point *point)
{
	int x;
	int y;
	SDL_GetMouseState(&x, &y);
	point->h = x;
	point->v = y;
#if 0
	// OS 9 code
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





