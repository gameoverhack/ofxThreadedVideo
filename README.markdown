OFXTHREADED VIDEO
=================

ofxThreaded video implements an asynchronous, non-blocking API for ALL critical Quicktime API calls. This results in TOTALLY NON-BLOCKING load, seek, pause, play etc. It also greatly improves video playback efficiency especially if you use OF_PIXEL_BGRA or OF_PIXEL_YUY2 pixel formats with the following codecs: ANIMATION 50-60% (using OF_PIXELS_BGRA on <= 10.8), or JPEG 50-60% and PRORES422 50-60% (using OF_PIXELS_YUY2 on >= 10.8).

I get around 8+ HD 1920 x 1080 movies running smoothly on a late model rMBP.

LATEST VERSION WORKS ON 10.8+ and of009x!! You just need to compile your application as 32bit (OpenFrameworks can remain Universal 32/64bit). AWESOMENESS.

Changes:
* Updated to work with of009x (if you want the legacy version for some reason it's in the 'legacy' branch of this repo)
* Now works in 10.6, 10.7, 10.8 (and presumably 10.9 + 10.10 though needs testing) without modifying openFrameworks or compiling against 10.6 SDK
* Improved stability under heavy loads and setFrame/setPause (tested 1 million+ without crash)
* Can use optimised BGRA and YUY2 pixel formats (with JPEG and ProRes codecs) - including built in YUY2 -> RGB/A shader

Please also note that you will need to comment out:

```
#define USE_QUICKTIME_7
#define USE_JACK_AUDIO
```

if I've forgotten to do so (these options are specific to some of my needs and occasionally I forget to comment them out when pushing to git).

