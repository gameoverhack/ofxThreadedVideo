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
#include <queue>

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
    VIDEO_EVENT_LOAD_BLOCKED
};

class ofxThreadedVideoEvent;
class ofxThreadedVideo : public ofThread {

public:
    
    ofxThreadedVideo();
    ~ofxThreadedVideo();
    
    void update();
    
    void draw();
    void draw(float x, float y);
    void draw(float x, float y, float w, float h);
    
    bool loadMovie(string fileName);
    
    void setUseAutoPlay(bool b);
    bool getUseAutoPlay();
    
    void setUseQueue(bool b);
    bool getUseQueue();
    
    void setFastPaused(bool b);
    bool isFastPaused();
    
    void setFastPosition(float pct);
    void setFastFrame(int frame);
    
    ofVideoPlayer & getVideo();
    ofTexture & getTextureReference();
    
    string getPath();
    
    ofEvent<ofxThreadedVideoEvent> threadedVideoEvent;
    inline string getEventTypeAsString(ofxThreadedVideoEventType eventType);
    
protected:
    
    void threadedFunction();
    
private:
    
    int loadVideoID;
    int currentVideoID;
    int videoIDCounter;
    
    string loadPath;
    
    float fFastPosition;
    int iFastFrame;
    
    bool bUseAutoPlay;
    bool bUseQueue;
    bool bFastPaused;
    bool bNewFrame;
    
    queue<string> pathsToLoad;
    string paths[2];
    ofTexture textures[2];
    ofVideoPlayer videos[2];
    
    int instanceID;
    
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