
#pragma precompile_target "MyPCH_Normal.mch"
#pragma once

		/* MY BUILD OPTIONS */
		
#define	TARGET_API_MAC_CARBON				1
#define	CALL_IN_SPOCKETS_BUT_NOT_IN_CARBON	1

		/* HEADERS */
		
#include <stdlib.h>
//#include <TextUtils.h>
#include <math.h>
#include <SDL.h>
#include <SDL_opengl.h>
//#include <textutils.h>
//#include <Files.h>
//#include <Resources.h>
//#include <Sound.h>

//#include <Power.h>
//#include <Script.h>
//#include <Fonts.h>
//#include <TextUtils.h>
//#include <Gestalt.h>
//#include	<Movies.h>
//#include <Components.h>
//#include <QuicktimeComponents.h>

#include "Pomme.h"
#include "globals.h"
#include "structs.h"

#include "metaobjects.h"
#include "ogl_support.h"
#include "main.h"
#include "player.h"
#include "mobjtypes.h"
#include "objects.h"
#include "misc.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include	"skeletonjoints.h"
#include "sound2.h"
#include "sobjtypes.h"
#include "terrain.h"
#include "sprites.h"
#include "shards.h"
#include "sparkle.h"
#include "bg3d.h"
#include "effects.h"
#include "camera.h"
#include "collision.h"
#include 	"input.h"
#include "file.h"
#include "fences.h"
#include "splineitems.h"
#include "items.h"
#include "window.h"
#include "enemy.h"
#include "water.h"
#include "miscscreens.h"
#include "pick.h"
#include "internet.h"