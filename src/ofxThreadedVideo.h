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
#include <assert.h>

#include "ofMain.h"

#if __LP64__

#error ofxThreadedVideo support requires 32-bit QuickTime APIs but this target is 64-bit

#else

//#define USE_QUICKTIME_7
//#define USE_JACK_AUDIO

#include "ofQtUtils.h"
#include "ofQuickTimePlayer.h"

// class implimentation overrides how ofQuicktimePlayer implements pixel
// decoding and allows setPixels for RGB, RGBA, BGRA and YUY2 pixel formats
class ofQuickTimePlayerWithFastPixels: public ofQuickTimePlayer{
    
public:
    
void createImgMemAndGWorld(){
    
    Rect movieRect;
    movieRect.top 			= 0;
    movieRect.left 			= 0;
    movieRect.bottom 		= height;
    movieRect.right 		= width;
    
    switch(internalPixelFormat){
            
        case OF_PIXELS_RGB:
        {
            offscreenGWorldPixels = new unsigned char[3 * width * height + 24];
            pixels.allocate(width, height, OF_PIXELS_RGB);
            QTNewGWorldFromPtr (&(offscreenGWorld), k24RGBPixelFormat, &(movieRect), NULL, NULL, 0, (pixels.getPixels()), 3 * width);
            break;
        }
        case OF_PIXELS_RGBA:
        {
            offscreenGWorldPixels = new unsigned char[4 * width * height + 32];
            pixels.allocate(width, height, OF_PIXELS_RGBA);
            QTNewGWorldFromPtr (&(offscreenGWorld), k32RGBAPixelFormat, &(movieRect), NULL, NULL, 0, (pixels.getPixels()), 4 * width);
            break;
        }
        case OF_PIXELS_BGRA:
        {
            offscreenGWorldPixels = new unsigned char[4 * width * height + 32];
            pixels.allocate(width, height, OF_PIXELS_BGRA);
            QTNewGWorldFromPtr (&(offscreenGWorld), k32BGRAPixelFormat, &(movieRect), NULL, NULL, 0, (pixels.getPixels()), 4 * width);
            break;
        }
        case OF_PIXELS_YUY2:
        {
#if !defined (TARGET_OSX) && !defined (GL_APPLE_rgb_422)
            movieRect.right = width*2; // this makes it look correct but we lose some of the performance gains
            SetMovieBox(moviePtr, &(movieRect));
            //width = width / 2; // this makes it go really fast but we only get 'half-resolution'...
            offscreenGWorldPixels = new unsigned char[4 * width * height + 32];
            pixels.allocate(width, height, OF_IMAGE_COLOR_ALPHA);
            QTNewGWorldFromPtr (&(offscreenGWorld), k24RGBPixelFormat, &(movieRect), NULL, NULL, 0, (pixels.getPixels()), 4 * width);
#else
            
            // for some reason doesn't like non-even width's and height's
            if(width % 2 != 0) width++;
            if(height % 2 != 0) height++;
            movieRect.bottom = height;
            movieRect.right = width;
            
            // this works perfectly on Mac platform!
            offscreenGWorldPixels = new unsigned char[2 * width * height + 32];
            pixels.allocate(width, height, OF_IMAGE_COLOR_ALPHA);
            QTNewGWorldFromPtr (&(offscreenGWorld), k2vuyPixelFormat, &(movieRect), NULL, NULL, 0, (pixels.getPixels()), 2 * width);
#endif
            
            break;
        }
    }
    
    LockPixels(GetGWorldPixMap(offscreenGWorld));
    
    // from : https://github.com/openframeworks/openFrameworks/issues/244
    // SetGWorld do not seems to be necessary for offscreen rendering of the movie
    // only SetMovieGWorld should be called
    // if both are called, the app will crash after a few ofVideoPlayer object have been deleted
    
#ifndef TARGET_WIN32
    SetGWorld (offscreenGWorld, NULL);
#endif
    SetMovieGWorld (moviePtr, offscreenGWorld, nil);
    
}
    
//---------------------------------------------------------------------------
bool setPixelFormat(ofPixelFormat pixelFormat){
    //note as we only support RGB we are just confirming that this pixel format is supported
    if( pixelFormat == OF_PIXELS_RGB || pixelFormat == OF_PIXELS_RGBA || pixelFormat == OF_PIXELS_BGRA || pixelFormat == OF_PIXELS_YUY2){
        internalPixelFormat = pixelFormat;
        return true;
    }
    ofLogWarning("ofQuickTimePlayer") << "setPixelFormat(): requested pixel format " << pixelFormat << " not supported";
    return false;
}
    
protected:
    
    int internalPixelFormat;
    
};

// YUV2 shader modified from: http://www.fourcc.org/fccyvrgb.php and http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
const string ofxThreadedVideoVertexShader = "void main(void){\
gl_TexCoord[0] = gl_MultiTexCoord0;\
gl_TexCoord[1] = gl_MultiTexCoord0 + vec4(1,0,0,0);\
gl_Position = ftransform();\
}";

const string ofxThreadedVideoFragmentShader = "uniform sampler2DRect yuvTex;\
uniform int conversionType;\
uniform float fade;\
void main(void){\
float nx, ny, r, g, b, a, y, u, v;\
vec4 txl,ux,vx;\
nx = gl_TexCoord[0].x;\
ny = gl_TexCoord[0].y;\
y = texture2DRect(yuvTex, vec2(nx,ny)).g;\
u = texture2DRect(yuvTex, vec2(nx,ny)).b;\
v = texture2DRect(yuvTex, vec2(nx,ny)).r;\
y = 1.164383561643836 * (y - 0.0625);\
u = u - 0.5;\
v = v - 0.5;\
float kb = 0.114;\
float kr = 0.299;\
float c = 2.276785714285714;\
if(conversionType == 709){\
kb = 0.0722;\
kr = 0.2126;\
}\
r = y + c * (1.0 - kr) * v;\
g = y - c * (1.0 - kb) * (kb / (1.0 - kb - kr)) * u - c * (1.0 - kr) * (kr / (1.0 - kb - kr)) * v;\
b = y + c * (1.0 - kb) * u;\
gl_FragColor = vec4(r * fade, g * fade, b * fade, 1.0 * fade);\
}";

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

class ofxThreadedVideo;
class ofxThreadedVideoEvent;

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
        return getArgument(index, T());
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

static ofMutex                          ofxThreadedVideoGlobalMutex;
static bool                             ofxThreadedVideoGlobalCritical = false;
static int                              ofxThreadedVideoGlobalInstanceID = 0;

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

    void load(const string& path);
    void loadMovie(const string& path);
    void setPixelFormat(ofPixelFormat pixelFormat);
    ofPixelFormat getPixelFormat();
    void closeMovie();
    void close();

    void update();
    void play();
    void stop();

    ofShader& getShader();
    void setUseInternalShader(bool b);
    bool getUseInternalShader();
    
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

	void setUseBlackStop(bool b);
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
    
    ofFbo fboYUY2;
    ofShader shader;
    bool bUseInternalShader;
    
    int textureInternalType;
    int textureFormatType;
    int texturePixelType;
    
	bool bUseBlackStop;
	bool bForceBlack;
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

#endif
