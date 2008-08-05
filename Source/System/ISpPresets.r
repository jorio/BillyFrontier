//
//
// ISpPresets.r
//
//

#include "InputSprocket.r"

/* the InputSprocket application resource tells utility programs how the */
/* application uses InputSprocket */

resource 'isap' (128)
{
	callsISpInit,
	usesInputSprocket
};


/* the set list resource contains the list of all the saved sets for devices */
/* that are provided in the application's resource fork */


resource 'setl' (1000)
{
	currentVersion,
	{
		"Default (Keyboard)", 0, kISpDeviceClass_Keyboard,
		kISpKeyboardID_Apple, notApplSet, isDefaultSet,
		128,
	};
};


/* Default keyboard set */

resource 'tset' (128, "Default (Keyboard)")
{
	supportedVersion,
	{	
				/* ANALOG MOVEMENT */
				
		 leftKey,				// Left
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 rightKey,				// Right
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 downKey,				// Reverse
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 upKey,					// Forward
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
								
				
				/* COMMANDS */

		spaceKey,				// Jump
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		commandKey,				// Kick
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		shiftKey,				// AutoWalk
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		optionKey,				// Pickup
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,

		tabKey,				// buddy bug
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,


				/* MISC */

		lessThanKey,				// Cam Right
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,

		greaterThanKey,				// Cam Left
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,

		returnKey,					// Cam Mode
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
	};
};




















