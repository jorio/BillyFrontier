// Externals
#include "game.h"

inline static Boolean ValidateSerialNumber(unsigned char *regInfo, Boolean eitherBoxedOrShareware);

#ifndef gRegInfo
extern unsigned char	gRegInfo[64];
#endif

#ifndef gSerialFileName
extern Str255  gSerialFileName;
#endif


/****************** DO SERIAL DIALOG *************************/

static void DoSerialDialog(unsigned char *out)
{
DialogPtr 		myDialog;
Boolean			dialogDone = false, isValid;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
int				rezID;

	Enter2D(false);
	
	if (gShareware)
		rezID = 3000 + gGamePrefs.language;
	else
		rezID = 4000 + gGamePrefs.language;
	
	myDialog = GetNewDialog(rezID,nil,MOVE_TO_FRONT);
	if (myDialog == nil)
	{
		DoAlert("\pDoSerialDialog: rez not found");
		ShowSystemErr(rezID);
	}
			
				/* DO IT */
				
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case	1:									        // Register
					GetDialogItem(myDialog,3,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,gRegInfo);
                    BlockMove(&gRegInfo[1], &gRegInfo[0], SERIAL_LENGTH);   // skip length byte from src string

                    isValid = ValidateSerialNumber(gRegInfo, false);    	// validate the number
    
                    if (isValid == true)
                    {
                        gGameIsRegistered = true;
                        dialogDone = true;
                        BlockMove(gRegInfo, out, SERIAL_LENGTH);		// copy to output
                    }
                    else
                    {
                    	switch(gGamePrefs.language)
                    	{
                    		case	LANGUAGE_SPANISH:
			                        DoAlert("\pLo sentimos, el n�mero de serie no es v�lido.  Por favor int�ntelo nuevamente.");
                    				break;
                    				
                    		default:
			                        DoAlert("\pSorry, that serial number is not valid.  Please try again.");
			            }
						InitCursor();
                    }
					break;

			case 	2:									// QUIT
                    ExitToShell();
					break;	

            case    4:                                  // Demo
            		if (gShareware)
            		{
	                    dialogDone = true;
						gGameIsRegistered 	= false;
						gSerialWasVerified 	= true;
	        		}
                    break;
					
            case    5:                                  // URL
            		if (gShareware)
					{
						if (LaunchURL("\phttp://www.pangeasoft.net/billy/buy.html") == noErr)
		                    ExitToShell();
					}
                    break;
		}
	}
	DisposeDialog(myDialog);

	Exit2D();
}






/********************* VALIDATE REGISTRATION NUMBER ******************/
//
// Return true if is valid
//
// INPUT:	eitherBoxedOrShareware = true if the serial number can be either the boxed-faux-code or
//									a legit shareware code (for reading Gamefiles which could have either)
//

inline static Boolean ValidateSerialNumber(unsigned char *regInfo, Boolean eitherBoxedOrShareware)
{
FSSpec	spec;
u_long	customerID;
u_long	seed0,seed1,seed2, validSerial, enteredSerial;
u_char	shift;
int		i,j,c;
Handle	hand;
static unsigned char pirateNumbers[26][SERIAL_LENGTH*2] =
{
	"C%N%O%P%A%K%A%A%J%O%A%K%",		// put "%" in there to confuse pirates scanning for this data
	"H%-%O%-%X%O%Q%H%I%N%C%P%",
	"H%-%O%-%H%O%Q%H%I%N%C%P%",
	"B%Q%U%E%T%S%H%O%Q%Q%H%E%",
	"B%R%K%M%F%J%I%T%H%E%P%R%",	
	"B%Q%U%E%T%S%H%O%Q%Q%H%E%",
	"B%R%K%M%F%J%I%T%H%E%P%R%",
	"B%R%O%D%S%S%J%R%F%T%O%N%",
	"B%R%T%H%R%L%F%M%P%N%K%S%",
	"B%P%L%J%H%M%M%Q%O%H%R%I%",
	"B%T%M%B%J%Q%L%I%E%I%Q%L%",
	"B%T%L%K%I%R%J%T%H%I%R%J%",
	"B%T%T%B%M%M%N%T%I%N%O%N%",
	"B%T%U%E%M%J%H%R%G%F%M%J%",
	"B%T%U%F%N%J%Q%T%N%L%M%T%",
	"B%T%T%K%F%J%K%S%J%J%F%P%",
	"B%T%T%J%T%M%H%M%O%P%J%O%",
	"B%T%T%E%H%M%J%I%Q%H%H%T%",
	"B%T%S%I%L%S%R%H%J%I%G%F%",
	"B%T%U%B%L%H%S%O%F%O%L%K%",
	"B%T%T%M%E%G%I%G%N%E%M%G%",
	"B%T%T%L%F%H%R%O%O%O%F%I%",
	"C%E%F%E%O%M%J%S%L%S%N%R%",
	"C%E%F%F%E%T%F%M%L%F%L%E%",
	"C%E%F%G%H%I%Q%K%R%H%O%E%",
	"B%T%P%H%O%E%M%H%E%E%M%M%",
	
};


			/******************************************************/
			/* THIS IS THE BOXED VERSION, SO VERIFY THE FAUX CODE */
			/******************************************************/

	if ((!gShareware) || eitherBoxedOrShareware)
	{	    
		int	i;
		static unsigned char fauxCode[SERIAL_LENGTH] = "PANG00737126";


		for (i = 0 ; i < SERIAL_LENGTH; i++)
		{		
			if (regInfo[i] != fauxCode[i])						// see if doesn't match
			{
				if (eitherBoxedOrShareware)						// see if we should also try as shareware code
					goto try_shareware_code;
				else
					return(false);
			}
		}
		gSerialWasVerified = true;					// set this to verify that hackers didn't bypass this function
	    return(true);
	}



			/*******************************************************/
			/* THIS IS THE SHAREWARE VERSION, SO VALIDATE THE CODE */
			/*******************************************************/
	else
	{
try_shareware_code:
	
			/* CONVERT TO UPPER CASE */
				    
	    for (i = 0; i < SERIAL_LENGTH; i++)
	    {
			if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))
				regInfo[i] = 'A' + (regInfo[i] - 'a');
		}
	    
	    
	    	/* THE FIRST 4 DIGITS ARE THE CUSTOMER INDEX */
	    
	    customerID  = (regInfo[0] - 'B') * 0x1000;				// convert B,C,D,E  F,G,H,I, J,K,L,M  N,O,P,Q to 0x1000...0xf000
	    customerID += (regInfo[1] - 'E') * 0x0100;				// convert F,G,H,I  J,K,L,M  N,O,P,Q  R,S,T,U to 0x0100...0x0f00
	    customerID += (regInfo[2] - 'F') * 0x0010;				// convert G,H,I,J, K,L,M,N, O,P,Q,R, S,T,U,V to 0x0001...0x00f0
	    customerID += (regInfo[3] - 'A') * 0x0001;				// convert A,B,C,D  E,F,G,H  I,J,K,L  M,N,O,P to 0x0010...0x000f

		if (customerID < 200)									// there are no customers under 200 since we want to confuse pirates
			return(false);
		if (customerID > 20000)									// also assume not over 20,000
			return(false);

			/* NOW SEE WHAT THE SERIAL SHOULD BE */

		seed0 = 0x2a10ce30;										// init random seed
		seed1 = 0xEE0101BA5;
		seed2 = 0x1234FFCC9;
			
		for (i = 0; i < customerID; i++)						// calculate the random serial
  			seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+(seed0 = (seed0+1)*3141592621UL))*2435386481UL;

		validSerial = seed2;


			/* CONVERT ENTERED SERIAL STRING TO NUMBER */
			
		shift = 0;
		enteredSerial = 0;
		for (i = SERIAL_LENGTH-1; i >= 4; i--)						// start @ last digit
		{
			u_long 	digit = regInfo[i] - 'E';					// convert E,F,G,H, I,J,K,L, M,N,O,P, Q,R,S,T to 0x0..0xf		
			enteredSerial += digit << shift;					// shift digit into posiion
			shift += 4;											// set to insert next nibble
		}
			
				/* SEE IF IT MATCHES */
				
		if (enteredSerial != validSerial)
			return(false);
			


				/**********************************/
				/* CHECK FOR KNOWN PIRATE NUMBERS */
				/**********************************/
				
					/* FIRST VERIFY OUR TABLE */
					
		if ((pirateNumbers[0][1] != '%') ||									// we do this to see if priates have cleared out our table
			(pirateNumbers[0][3] != '%') ||
			(pirateNumbers[0][4] != 'O') ||
			(pirateNumbers[0][6] != 'P'))
		{
			DoFatalAlert("\pThis application is corrupt.  You should reinstall a fresh copy of the game.");		
		}
				
				/* THEN SEE IF THIS CODE IS IN THE TABLE */
						
		for (j = 0; j < 5; j++)
		{
			for (i = 0; i < SERIAL_LENGTH; i++)
			{
				if (regInfo[i] != pirateNumbers[j][i*2])					// see if doesn't match
					goto next_code;		
			}
			
					/* THIS CODE IS PIRATED */
					
			return(false);
			
	next_code:;		
		}


				/*******************************/
				/* SECONDARY CHECK IN REZ FILE */
				/*******************************/
				//
				// The serials are stored in the Level 1 terrain file
				//

		if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Audio:Main.sounds", &spec) == noErr)		// open rez fork
		{
			short fRefNum = FSpOpenResFile(&spec,fsRdPerm);
			
			UseResFile(fRefNum);						// set app rez
		
			c = Count1Resources('savs');						// count how many we've got stored
			for (j = 0; j < c; j++)
			{
				hand = Get1Resource('savs',128+j);			// read the #
			
				for (i = 0; i < SERIAL_LENGTH; i++)
				{
					if (regInfo[i] != (*hand)[i])			// see if doesn't match
						goto next2;		
				}
			
						/* THIS CODE IS PIRATED */
						
				UseResFile(gMainAppRezFile);				// set app rez
				return(false);
			
		next2:		
				ReleaseResource(hand);		
			}
			
			UseResFile(gMainAppRezFile);					// set app rez
		}

		gSerialWasVerified = true;					// set this to verify that hackers didn't bypass this function
	    return(true);
	}

}



/********************** CHECK GAME SERIAL NUMBER *************************/

static void CheckGameSerialNumber(Boolean onlyVerify)
{
OSErr   iErr;
FSSpec  spec;
short		fRefNum;
long        	numBytes = SERIAL_LENGTH;

            /* GET SPEC TO REG FILE */

	iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gSerialFileName, &spec);
    if (iErr)
        goto game_not_registered;


            /****************************/
            /* VALIDATE THE SERIAL FILE */
            /****************************/

            /* READ SERIAL DATA */

    if (FSpOpenDF(&spec,fsCurPerm,&fRefNum) != noErr)		// if cant open file then not registered yet
        goto game_not_registered;
    	
	FSRead(fRefNum,&numBytes,gRegInfo);	
    FSClose(fRefNum);

            /* VALIDATE THE DATA */

    if (!ValidateSerialNumber(gRegInfo, false))			// if this serial is bad then we're not registered
        goto game_not_registered;        

    gGameIsRegistered = true;
    
	return;
	

        /* GAME IS NOT REGISTERED YET, SO DO DIALOG */

game_not_registered:

	if (onlyVerify)											// see if let user try to enter it
		gGameIsRegistered = false;	
	else
	    DoSerialDialog(gRegInfo);


    if (gGameIsRegistered)                                  // see if write out reg file
    {
	    FSpDelete(&spec);	                                // delete existing file if any
	    iErr = FSpCreate(&spec,kGameID,'xxxx',-1);
        if (iErr == noErr)
        {
        	numBytes = SERIAL_LENGTH;
			FSpOpenDF(&spec,fsCurPerm,&fRefNum);
			FSWrite(fRefNum,&numBytes,gRegInfo);
		    FSClose(fRefNum);
     	}  
    }
    
}





