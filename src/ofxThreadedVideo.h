/*
 *  ofxThreadedVideo.h
 *  emptyExample
 *
 *  Created by gameover on 2/02/12.
 *  Copyright 2012 trace media. All rights reserved.
 *
 */

#ifndef _H_OFXTHREADEDVIDEO
#define _H_OFXTHREADEDVIDEO

#include <set>
#include <deque>

#include "ofLog.h"
#include "ofConstants.h"
#include "ofPixels.h"
#include "ofTexture.h"
#include "ofThread.h"
#include "ofEvents.h"
#include "ofVideoPlayer.h"
#include "ofAppRunner.h"

enum ofxThreadedVideoEventType{
    VIDEO_EVENT_LOAD_OK = 0,
    VIDEO_EVENT_LOAD_FAIL,
    VIDEO_EVENT_LOAD_BLOCKED,
    VIDEO_EVENT_LOAD_THREADBLOCKED
};

class ofxThreadedVideoEvent;
class ofxThreadedVideo : public ofThread {

public:

    ofxThreadedVideo();
    ~ofxThreadedVideo();

    void setPlayer(ofPtr<ofBaseVideoPlayer> newPlayer);
    ofPtr<ofBaseVideoPlayer> getPlayer();

    void setUseAutoPlay(bool b);
    bool getUseAutoPlay();

    void setUseQueue(bool b);
    bool getUseQueue();

    bool loadMovie(string name);
    void setPixelFormat(ofPixelFormat pixelFormat);
    ofPixelFormat getPixelFormat();
    void closeMovie();
    void close();

    void update();
    void play();
    void stop();

    bool isFrameNew();
    unsigned char * getPixels();
    ofPixelsRef	getPixelsRef();
    float getPosition();
    float getSpeed();
    float getDuration();
    bool getIsMovieDone();

    void setPosition(float pct);
    void setVolume(float volume);
    float getVolume();
    void setPan(float pan);
    float getPan();
    void setLoopState(ofLoopType state);
    int getLoopState();
    void setSpeed(float speed);
    void setFrame(int frame);

    void setUseTexture(bool bUse);
    ofTexture &	getTextureReference();
    void draw(float x, float y, float w, float h);
    void draw(float x, float y);
    void draw(const ofPoint & p);
    void draw(const ofRectangle & r);
    void draw();

    void setAnchorPercent(float xPct, float yPct);
    void setAnchorPoint(float x, float y);
    void resetAnchor();

    void setPaused(bool bPause);

    int getCurrentFrame();
    int getTotalNumFrames();

    void firstFrame();
    void nextFrame();
    void previousFrame();

    float getHeight();
    float getWidth();

    bool isPaused();
    bool isLoaded();
    bool isPlaying();
    bool isLoading();
    bool isQueued(string path);
    
    //float width, height;

    string getMovieName();
    string getMoviePath();

    double getFrameRate();

    ofEvent<ofxThreadedVideoEvent> threadedVideoEvent;
    string getEventTypeAsString(ofxThreadedVideoEventType eventType);

protected:

    void threadedFunction();

private:

    static const int VIDEO_NONE = -1;
    static const int VIDEO_FLIP = 0;
    static const int VIDEO_FLOP = 1;

    void updatePixels(int videoID);
    void updateTexture(int videoID);
    void updateVideo(int videoID);

    int getNextLoadID();

    int loadVideoID;
    int currentVideoID;

    string loadPath;

    float newPosition[2];
    int newFrame[2];
    float volume[2];
    float pan[2];
    bool bPaused[2];
    float newSpeed[2];
    int newLoopType[2];
    int frame[2];
    int totalframes[2];

    bool bUseAutoPlay;
    bool bUseQueue;
    bool bUseTexture;

    deque<string> pathsToLoad;
    string paths[2];
    string names[2];
    bool bFrameNew[2];
    ofPixels * pixels[2];
    ofTexture textures[2];
    ofVideoPlayer videos[2];

    ofPixelFormat internalPixelFormat;

    double prevMillis, lastFrameTime, timeNow, timeThen, fps, frameRate;

    // block copy ctor and assignment operator
    ofxThreadedVideo(const ofxThreadedVideo& other);
    ofxThreadedVideo& operator=(const ofxThreadedVideo&);

};

class ofxThreadedVideoEvent{

public:

    ofxThreadedVideoEvent(string _path, ofxThreadedVideoEventType _eventType, ofxThreadedVideo * _video)
    : path(_path), eventType(_eventType), video(_video){eventTypeAsString = _video->getEventTypeAsString(_eventType);};

    string path;
    string eventTypeAsString;
    ofxThreadedVideoEventType eventType;
    ofxThreadedVideo * video;

};

#endif
