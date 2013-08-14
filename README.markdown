OFXTHREADED VIDEO
=================

ofxThreaded video does two things let's you load video's of any size without blocking the main application thread. As a side product this also tends to improve performance for some codecs. The best ones are JPEG 50-60% or ANIMATION 50-60%. It's been tested on OSX (10.6,7,8) and Windows 7.

I get around 8 HD 1920 x 1080 movies running smoothly on a late model rMBP.

Please note that this version is based on Quicktime 6 or 7 (ie., the 10.6 SDK) so you will need to install a copy of that and compile your projects (and oF) against 10.6 if you are using 10.7 or 10.8 on your dev machine (I'm working on a QTKit version at the moment…).

Since it is non-blocking, or asynchronous ofxThreadedVideo cannot always be used as a direct drop-in for an ofVideoPlayer. Previously you had to wait until a video was fully loaded before issuing any other commands that manipulated or queried the video. This version implements a new queueing system that allows all 'set' commands to be issued as though the video were a normal 'blocking' ofVideoPlayer. However any 'get' method must (for obvious reasons) wait until the video is loaded (this can be achieved by watching the isLoaded() method, or using the built in event model).

Please also note that the queue is designed to handle an UNREASONABLE user request scenario without rejecting any requests. Eg., if you ask an instance (or several instances) of ofxThreadedVideo to load, let's say, 100,000 videos it will swallow the request and then proceed to execute it as fast as possible (roughly 1 load per 80-150ms depending on the system). Less 'expensive' calls such as setVolume, setFrame, play, stop, pause etc execute faster than this (but importantly not immediately). The thing is designed for efficiency and non-blockingness, not 'accuracy' or 'immediacy' per se.

This is because the overall design is actually a serial queue because Quicktime is by (unfortunate) design non-thread safe for many operations - specifically any playing, pausing, seeking, etc parts of the API. ofxThreadedVideo gets around this by using a series of local and global mutexes to ensure that both single and multiple threaded instances never (EVER) call a Quicktime command at exactly the same time. Instead it schedules every call to occur on the next 'tick' so to speak.

This new method is NEW - so expect some bugs. BUT it is both much more efficient and truly block free compared to the previous incarnation.

