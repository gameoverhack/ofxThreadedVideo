/*
 * ofxThreadedVideo.h
 *
 * Copyright 2010-2013 (c) Matthew Gingold http://gingold.com.au
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * If you're using this software for something cool consider sending
 * me an email to let me know about your project: m@gingold.com.au
 *
 */

#ifndef __H_OFXTHREADEDVIDEO
#define __H_OFXTHREADEDVIDEO

#include <set>
#include <deque>

#include "ofMain.h"

#define USE_QUICKTIME_7
#define USE_JACK_AUDIO

enum ofxThreadedVideoEventType{
    VIDEO_EVENT_LOAD_OK = 0,
    VIDEO_EVENT_LOAD_FAIL
};

#ifdef USE_JACK_AUDIO
struct AudioChannelMap{
    int trackIndex;
    int oldChannel;
    int newChannel;
};
#endif

class ofxThreadedVideoFade;
class ofxThreadedVideoEvent;
class ofxThreadedVideoCommand;

static ofMutex                          ofxThreadedVideoGlobalMutex;
static bool                             ofxThreadedVideoGlobalCritical = false;
static int                              ofxThreadedVideoGlobalInstanceID = 0;
//static deque<ofxThreadedVideoCommand>   ofxThreadedVideoCommands;
//static ofxThreadedVideoCommand          ofxThreadedVideoNullCommand; (declared below)

static int ofxThreadedVideoLoadOk;
static int ofxThreadedVideoLoadFail;

class ofxThreadedVideo : public ofThread {

public:

    ofxThreadedVideo();
    ~ofxThreadedVideo();
    
    template <typename T>
    void setPlayer(){
        video[0].setPlayer(ofPtr<T>(new T));
        video[1].setPlayer(ofPtr<T>(new T));
    }

    ofPtr<ofBaseVideoPlayer> getPlayer();

    bool loadMovie(string path);
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

#ifdef USE_QUICKTIME_7
    float getVolume(); // we should implement for QT6
    void setPan(float pan);
    float getPan();
#endif
    
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
    bool isLoading(string path);
    bool isTextureReady();
    
#ifdef USE_JACK_AUDIO
    vector<string>  getAudioDevices();
//    int             getAudioTrackList();
    void            setAudioDevice(int deviceID);
    void            setAudioDevice(string deviceID);
    void            setAudioTrackToChannel(int trackIndex, int oldChannelLabel, int newChannelLabel);
#endif
    
    string getMovieName();
    string getMoviePath();

    double getFrameRate();
    
    void setFade(float fadeTarget);
    void setFade(int frameStart, int durationMillis, float fadeTarget, bool fadeSound = false, bool fadeVideo = true, bool fadeOnce = false);
    
    float getFade();
    void clearFades();
    
    ofEvent<ofxThreadedVideoEvent> threadedVideoEvent;
    string getEventTypeAsString(ofxThreadedVideoEventType eventType);
    
    void setVerbose(bool b);
    
    void flush();
    void finish();
    
    int getQueueSize();
    int getLoadOk();
    int getLoadFail();
    
protected:

    void threadedFunction();

    bool getGlobalSafe();
    void setGlobalSafe();
    void setGlobalUnSafe();
    
    void pushCommand(ofxThreadedVideoCommand& c, bool back = true);
    ofxThreadedVideoCommand getCommand();
    void popCommand();

    static const int VIDEO_NONE = -1;
    static const int VIDEO_FLIP = 0;
    static const int VIDEO_FLOP = 1;

    int getNextLoadID();

    //--------------------------------------------------------------
    
    deque<ofxThreadedVideoCommand>   ofxThreadedVideoCommands;
    
    bool bLoaded;
    bool bCriticalSection;
    int currentVideoID;
    
    ofVideoPlayer video[2];
    ofPixels * pixels;
    ofTexture drawTexture;
    
    bool bUseTexture;
    bool bIsFrameNew;
    bool bForceFrameNew;
    bool bIsPaused;
    bool bIsPlaying;
    bool bIsTextureReady;
    bool bIsLoading;
    bool bIsMovieDone;
    
    float width;
    float height;
    
    float duration;
    float speed;
    float position;
    
    float volume;
    float pan;
    
    ofLoopType loopState;
    
    int frameCurrent;
    int frameTotal;
    
    string movieName;
    string moviePath;

    float fade;
    float _fade;
    deque<ofxThreadedVideoFade> fades;
    
    ofPixelFormat internalPixelFormat;
    
#ifdef USE_JACK_AUDIO
    vector<string> audioDevices;
#endif
    
    int instanceID;
    
    double prevMillis, lastFrameTime, timeNow, timeThen, fps, frameRate;
    
    bool bVerbose;
    
private:
    
    // block copy ctor and assignment operator
    ofxThreadedVideo(const ofxThreadedVideo& other);
    ofxThreadedVideo& operator=(const ofxThreadedVideo&);

};

class ofxThreadedVideoFade {
    
public:
    
    ofxThreadedVideoFade(){
        fadeOriginal = -1.0;
    };
    ofxThreadedVideoFade(int _frameStart, int _frameEnd, float _fadeTarget, bool _fadeSound, bool _fadeVideo, bool _fadeOnce):
    frameStart(_frameStart), frameEnd(_frameEnd), fadeTarget(_fadeTarget), fadeSound(_fadeSound), fadeVideo(_fadeVideo), fadeOnce(_fadeOnce){
        fadeOriginal = -1.0;
        frameDuration = frameEnd - frameStart;
    };
    ~ofxThreadedVideoFade(){};
    
    float getFade(float fadeCurrent, int frameCurrent){
        
        if(fadeOriginal == -1.0f) fadeOriginal = fadeCurrent;
        
        if((frameCurrent - frameStart) <= frameEnd){
            
            return fadeOriginal + (fadeTarget - fadeOriginal) * (float)((float)(frameCurrent - frameStart) / (float)frameDuration);
        }else{
            fadeOriginal = -1.0f;
            return fadeTarget;
        }
        
    }
    
    bool getIsFading(int frameCurrent){
        if(frameCurrent >= frameStart && frameCurrent <= frameEnd){
            return true;
        }else{
            fadeOriginal = -1.0f;
            return false;
        }
    }
    
    bool getFadeDone(int frameCurrent){
        if(frameCurrent > frameEnd){
            return true;
        }else{
            return false;
        }
    }
    
    int frameStart;
    int frameEnd;
    float frameDuration;
    float fadeTarget;
    float fadeOriginal;
    bool fadeSound;
    bool fadeVideo;
    bool fadeOnce;
    
};

class ofxThreadedVideoCommand {
    
public:
    
    ofxThreadedVideoCommand(){
        setCommand("NULL_COMMAND", -1);
    }
    ofxThreadedVideoCommand(string _command, int _instanceID){
        setCommand(_command, _instanceID);
    }
    ~ofxThreadedVideoCommand(){
        instanceID = -1;
        command.clear();
        args.clear();
    }
    
    void setCommand(string _command, int _instanceID){
        instanceID = _instanceID;
        command = _command;
    }
    
    int getInstance(){
        return instanceID;
    }
    
    string getCommand(){
        return command;
    }
    
    template<typename T>
    void setArgument(T arg){
        args.push_back(ofToString(arg));
    }
    
    template<typename T>
    T getArgument(int index){
        if(index > args.size() || index < 0){
            ofLogError() << "Argument index out of range: " << index << " : " << args.size();
            return NULL;
        }
        T t; // template overload hack
        return getArgument(index, t);
    }
    
    string getArgument(int index, string t){
        return args[index];
    }
    
    int getArgument(int index, int t){
        return ofToInt(args[index]);
    }
    
    float getArgument(int index, float t){
        return ofToFloat(args[index]);
    }
    
    bool getArgument(int index, bool t){
        return ofToBool(args[index]);
    }
    
    int getNumArguments(){
        return args.size();
    }
    
    string getCommandAsString(){
        ostringstream os;
        os << command << "(";
        for(int i = 0; i < args.size(); i++){
            os << args[i] << string(i < args.size() - 1 ? "," : "");
        }
        os << ")";
        return os.str();
    }
    
private:
    
    int instanceID;
    string command;
    vector<string> args;
    
};

static ofxThreadedVideoCommand ofxThreadedVideoNullCommand;

class ofxThreadedVideoEvent{

public:

    ofxThreadedVideoEvent(string _path, ofxThreadedVideoEventType _eventType, ofxThreadedVideo * _video)
    : path(_path), eventType(_eventType), video(_video){eventTypeAsString = _video->getEventTypeAsString(_eventType);};

    string path;
    string eventTypeAsString;
    ofxThreadedVideoEventType eventType;
    ofxThreadedVideo * video;

    friend ostream& operator<< (ostream &os, ofxThreadedVideoEvent &e);
    
};

inline ostream& operator<<(ostream& os, const ofxThreadedVideoEvent &e){
    os << e.eventTypeAsString << " " << e.path;
    return os;
};

#endif
