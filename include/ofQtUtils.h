#pragma once

#include "ofConstants.h"
#define OF_VIDEO_PLAYER_QUICKTIME
#if __LP64__ && (defined(OF_VIDEO_CAPTURE_QUICKTIME) || defined(OF_VIDEO_PLAYER_QUICKTIME))

#error QuickTime support requires 32-bit QuickTime APIs but this target is 64-bit

#elif defined(OF_VIDEO_CAPTURE_QUICKTIME) || defined(OF_VIDEO_PLAYER_QUICKTIME)

#if defined (TARGET_WIN32) || defined (TARGET_OSX)
// these 'hacks' for using Quicktime under 10.7+ taken from the ingenius idea at https://github.com/bangnoise/ofxHapPlayer
#if defined(TARGET_WIN32)
	#include <QTML.h>
	#include <FixMath.h>
	#include <QuickTimeComponents.h>
	#include <TextUtils.h>
	#include <MediaHandlers.h>
#elif defined(TARGET_OSX)
	#include <QuickTime/QuickTime.h>
	#include <CoreServices/CoreServices.h>
	#include <ApplicationServices/ApplicationServices.h>
#endif
/*
 Much of QuickTime is deprecated in recent MacOS but no equivalent functionality exists in modern APIs,
 so we ignore these warnings.
 */
#pragma GCC push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/*
 These functions have been excised from modern MacOS headers but remain available
 */
#if !defined(__QDOFFSCREEN__)
extern "C" {
	void DisposeGWorld(GWorldPtr offscreenGWorld);
	void GetGWorld(CGrafPtr *port, GDHandle *gdh);
	PixMapHandle GetGWorldPixMap(GWorldPtr offscreenGWorld);
	void SetGWorld(CGrafPtr port, GDHandle gdh);
	Boolean LockPixels(PixMapHandle pm);
	void UnlockPixels(PixMapHandle pm);
	
	enum {
		useTempMem = 1L << 2,
		keepLocal = 1L << 3
	};
}
#endif

//p2cstr depreciation fix - thanks pickard!
#ifdef TARGET_OSX
	#define p2cstr(aStr) CFStringGetCStringPtr(CFStringCreateWithPascalString(nullptr, aStr, kCFStringEncodingMacRoman),kCFStringEncodingMacRoman)
#endif

//-------------------------- helpful for rgba->rgb conversion
typedef struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;
} pix24;



//----------------------------------------

void 		initializeQuicktime();
void 		closeQuicktime();
void 		convertPixels(unsigned char * gWorldPixels, unsigned char * rgbPixels, int w, int h);
Boolean 	SeqGrabberModalFilterUPP(DialogPtr theDialog, const EventRecord *theEvent, short *itemHit, long refCon);
OSErr           IsMPEGMediaHandler(MediaHandler inMediaHandler, Boolean *outIsMPEG);
ComponentResult MPEGMediaGetStaticFrameRate(MediaHandler inMPEGMediaHandler, Fixed *outStaticFrameRate);
OSErr           MediaGetStaticFrameRate(Media inMovieMedia, double *outFPS);
void            MovieGetVideoMediaAndMediaHandler(Movie inMovie, Media *outMedia,
				MediaHandler *outMediaHandler);
void            MovieGetStaticFrameRate(Movie inMovie, double *outStaticFrameRate);

#ifdef TARGET_OSX
	OSErr	GetSettingsPreference(CFStringRef inKey, UserData *outUserData);
	OSErr	SaveSettingsPreference(CFStringRef inKey, UserData inUserData);
#endif

#endif

#endif
