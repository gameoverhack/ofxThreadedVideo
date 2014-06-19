/*
 * ofxThreadedVideo.cpp
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

#include "ofxThreadedVideo.h"

//--------------------------------------------------------------
void ofxThreadedVideo::flush(){
    lock();
    ofxThreadedVideoGlobalMutex.lock();
    ofxThreadedVideoCommands.clear();
    ofxThreadedVideoGlobalMutex.unlock();
    unlock();
}

//--------------------------------------------------------------
void ofxThreadedVideo::finish(){
    while(getQueueSize() > 0){
        update();
    }
    update();
}

//--------------------------------------------------------------
int ofxThreadedVideo::getQueueSize(){
//    ofScopedLock lock(ofxThreadedVideoGlobalMutex);
    return ofxThreadedVideoCommands.size();
}

//--------------------------------------------------------------
int ofxThreadedVideo::getLoadOk(){
    return ofxThreadedVideoLoadOk;
}

//--------------------------------------------------------------
int ofxThreadedVideo::getLoadFail(){
    return ofxThreadedVideoLoadFail;
}

//--------------------------------------------------------------
ofxThreadedVideo::ofxThreadedVideo(){

    instanceID = ofxThreadedVideoGlobalInstanceID;
    ofxThreadedVideoGlobalInstanceID++;
    

#ifdef OF_VIDEO_PLAYER_GSTREAMER
    setPlayer<ofGstVideoPlayer>();
#else
    initializeQuicktime();
    setPlayer<ofQuickTimePlayer>();
#endif
    
    // setup video instances
    video[0].setUseTexture(false);
    video[1].setUseTexture(false);
    
    setPixelFormat(OF_PIXELS_RGB);
    
    drawTexture.allocate(1, 1, GL_RGB);
    ofPixels p;
    p.allocate(1, 1, OF_IMAGE_COLOR);
    p.set(0);
    drawTexture.loadData(p.getPixels(), 1, 1, GL_RGB);
    pixels = &video[0].getPixelsRef();
    
    // set vars to default values
    currentVideoID = VIDEO_FLIP;
    bCriticalSection = false;
    bLoaded = false;
    
    bUseTexture = true;
    bIsFrameNew = false;
    bIsPlaying = false;
    bIsLoading = false;
    bIsMovieDone = false;
    
    width = 0.0f;
    height = 0.0f;
    
    speed = 0.0f;
    position = 0.0f;
    duration = 0.0f;
    
    volume = 0.0f;
    pan = 0.0f;
    
    loopState = OF_LOOP_NORMAL;
    
    frameCurrent = 0;
    frameTotal = 0;
    
    movieName = "";
    moviePath = "";
    
    fade = 1.0f;
    fades.clear();
    
    prevMillis = ofGetElapsedTimeMillis();
    lastFrameTime = timeNow = timeThen = fps = frameRate = 0;
    
    ofxThreadedVideoNullCommand.setCommand("NULL_COMMAND", -1);
    
    bVerbose = false;
    
    // let's go!
    startThread();
}

//--------------------------------------------------------------
ofxThreadedVideo::~ofxThreadedVideo(){

    // stop threading
    waitForThread(true);
    
    // close anything left open
    video[0].close();
    video[1].close();
    
    drawTexture.clear();

}

//--------------------------------------------------------------
void ofxThreadedVideo::update(){
    
    lock();
    
    if(!bCriticalSection && bLoaded){
        bCriticalSection = true;
        int videoID = currentVideoID;
        bool bUpdate = bLoaded;
        unlock();
        
        if(bUpdate){
            
//            lock();
            video[videoID].update();
            
            bIsFrameNew = video[videoID].isFrameNew();
            position = video[videoID].getPosition();
            frameCurrent = video[videoID].getCurrentFrame();
            bIsMovieDone = video[videoID].getIsMovieDone();
//            unlock();
            
            if(bIsFrameNew || bForceFrameNew){
                
                if(bForceFrameNew) lock();
                
                if(!bIsTextureReady) bIsTextureReady = true;
                
                if(drawTexture.getWidth() != width || drawTexture.getHeight() != height){
                    drawTexture.allocate(width, height, ofGetGLTypeFromPixelFormat(video[videoID].getPixelFormat()));
                }
                
                unsigned char * pixels = video[videoID].getPixels();
                if(pixels != NULL && bUseTexture) drawTexture.loadData(pixels, width, height, ofGetGLTypeFromPixelFormat(video[videoID].getPixelFormat()));
                
                if(bForceFrameNew){
                    bForceFrameNew = false;
                    unlock();
                }
                
                // calculate frameRate -> taken from ofAppRunner
                prevMillis = ofGetElapsedTimeMillis();
                timeNow = ofGetElapsedTimef();
                double diff = timeNow-timeThen;
                if( diff  > 0.00001 ){
                    fps			= 1.0 / diff;
                    frameRate	*= 0.9f;
                    frameRate	+= 0.1f*fps;
                }
                lastFrameTime	= diff;
                timeThen		= timeNow;
                
            }
        }
        
        lock();
        bCriticalSection = false;
        unlock();
    }else{
        unlock();
    }
    
    lock();
    ofxThreadedVideoGlobalMutex.lock();
    if(!ofxThreadedVideoGlobalCritical && !bCriticalSection){
        int videoID = currentVideoID;
        ofxThreadedVideoGlobalCritical = true;
        bCriticalSection = true;
        ofxThreadedVideoCommand c = getCommand();
        bool bCanStop = (bLoaded && !bIsLoading) || (!bLoaded && !bIsLoading);
        bool bPopCommand = false;
        unlock();
        ofxThreadedVideoGlobalMutex.unlock();

        if(c.getInstance() == instanceID){
            
            if(c.getCommand() == "stop" && bCanStop){
                if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                if(bIsPlaying) video[videoID].stop();
                lock();
                //fade = 1.0;
                fades.clear();
                bIsPlaying = false;
                bIsPaused = false; // ????
                bIsLoading = false;
                bIsFrameNew = false;
                bIsMovieDone = false;
                bLoaded = false;
                unlock();
                bPopCommand = true;
            }
            
            if(c.getCommand() == "loadMovie" && bCanStop){
                if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString() << " execute in update";
                if(bIsPlaying) video[videoID].stop();
                lock();
                currentVideoID = getNextLoadID();
                bIsPaused = false;
                bLoaded = false;
                bIsLoading = true;
                bIsPlaying = false;
                bIsMovieDone = false;
                unlock();
            }
            
            if(c.getCommand() == "setSpeed"){
                if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                lock();
                speed = c.getArgument<float>(0);
                unlock();
                video[videoID].setSpeed(speed);
                bPopCommand = true;
            }
            
            if(c.getCommand() == "setFrame"){
                if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                lock();
                int frameTarget = c.getArgument<int>(0);
                bForceFrameNew = true;
                frameTarget = CLAMP(frameTarget, 0, frameTotal);
                //cout << "setframe A: " << frameTarget << " " << videoID << " " << bCriticalSection << endl;
                video[videoID].setFrame(frameTarget);
                //cout << "setframe B: " << frameTarget << " " << videoID << " " << bCriticalSection << endl;
                unlock();
                bPopCommand = true;
            }
            
        }
        
        lock();
        ofxThreadedVideoGlobalMutex.lock();
        
        if(bPopCommand) popCommand();
        
        ofxThreadedVideoGlobalCritical = false;
        bCriticalSection = false;
        ofxThreadedVideoGlobalMutex.unlock();
        unlock();
    }else{
        ofxThreadedVideoGlobalMutex.unlock();
        unlock();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::threadedFunction(){

    while (isThreadRunning()){

        lock();
        ofxThreadedVideoGlobalMutex.lock();
        if(!ofxThreadedVideoGlobalCritical && !bCriticalSection){
            ofxThreadedVideoGlobalCritical = true;
            bCriticalSection = true;
            int videoID = currentVideoID;
            ofxThreadedVideoCommand c = getCommand();
            bool bCanLoad = !bLoaded;
            bool bPopCommand = false;
            unlock();
            ofxThreadedVideoGlobalMutex.unlock();
            
            if(c.getInstance() == instanceID){
                
                if(c.getCommand() == "play"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    video[videoID].play();
                    
                    lock();
                    bIsPlaying = true;
                    bIsPaused = false;
                    unlock();
                    
                    bPopCommand = true;
                }
                
                if(c.getCommand() == "setPosition"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    lock();
                    position = c.getArgument<float>(0);
                    unlock();
                    video[videoID].setPosition(position);
                    bPopCommand = true;
                }

                if(c.getCommand() == "setVolume"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    lock();
                    volume = c.getArgument<float>(0);
                    unlock();
                    video[videoID].setVolume(volume);
                    bPopCommand = true;
                }
                
#ifdef USE_QUICKTIME_7
                if(c.getCommand() == "setPan"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    lock();
                    pan = c.getArgument<float>(0);
                    unlock();
                    video[videoID].setPan(pan);
                    bPopCommand = true;
                }
#endif
                
                if(c.getCommand() == "setLoopState"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    lock();
                    loopState = (ofLoopType)c.getArgument<int>(0);
                    unlock();
                    video[videoID].setLoopState(loopState);
                    bPopCommand = true;
                }
                
//                if(c.getCommand() == "setSpeed"){
//                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
//                    lock();
//                    speed = c.getArgument<float>(0);
//                    unlock();
//                    video[videoID].setSpeed(speed);
//                    bPopCommand = true;
//                }
                
//                if(c.getCommand() == "setFrame"){
//                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
//                    int frameTarget = c.getArgument<int>(0);
//                    CLAMP(frameTarget, 0, frameTotal);
//                    video[videoID].setFrame(frameTarget);
//                    bForceFrameNew = true;
//                    bPopCommand = true;
//                }
                
//                if(c.getCommand() == "setFrame"){
//                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
//                    lock();
//                    int frameTarget = c.getArgument<int>(0);
//                    bForceFrameNew = true;
//                    frameTarget = CLAMP(frameTarget, 0, frameTotal);
//                    cout << "setframe A: " << frameTarget << " " << videoID << " " << bCriticalSection << endl;
//                    video[videoID].setFrame(frameTarget);
//                    cout << "setframe B: " << frameTarget << " " << videoID << " " << bCriticalSection << endl;
//                    unlock();
//                    bPopCommand = true;
//                }
                
                if(c.getCommand() == "setPaused"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    lock();
                    bIsPaused = c.getArgument<bool>(0);
                    unlock();
                    video[videoID].setPaused(bIsPaused);
                    bPopCommand = true;
                }
                
                if(c.getCommand() == "setAnchorPercent"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    video[videoID].setAnchorPercent(c.getArgument<float>(0), c.getArgument<float>(0));
                    bPopCommand = true;
                }

                if(c.getCommand() == "setAnchorPoint"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    video[videoID].setAnchorPercent(c.getArgument<float>(0), c.getArgument<float>(0));
                    bPopCommand = true;
                }
                
                if(c.getCommand() == "resetAnchor"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    video[videoID].resetAnchor();
                    bPopCommand = true;
                }
                
                if(c.getCommand() == "setFade"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    
                    int frameEnd;
                    int frameStart = c.getArgument<int>(0);
                    int durationMillis = c.getArgument<int>(1);
                    float fadeTarget = c.getArgument<float>(2);
                    bool fadeSound = c.getArgument<bool>(3);
                    bool fadeVideo = c.getArgument<bool>(4);
                    bool fadeOnce = c.getArgument<bool>(5);
                    
                    CLAMP(fadeTarget, 0.0f, 1.0f);
                    
                    if(frameStart == -1){ // fade is durationMillis from the end
                        frameEnd = frameTotal;
                        frameStart = frameTotal - ((float)durationMillis / 1000.0) * 25.0;
                    }else{
                        frameEnd = frameStart + ((float)durationMillis / 1000.0) * 25.0;
                    }
                    
                    if(frameStart == frameEnd){
                        _fade = fadeTarget;
                        if(fadeVideo) fade = _fade;
                        lock();
                        if(fadeSound) video[videoID].setVolume(_fade);
                        unlock();
                    }else{
                        frameEnd -= 1;
                        
                        // assert(frameStart >= 0);
                        // assert(frameEnd >= frameStart);
                        // assert(frameEnd <= frameTotal);
                        
                        fades.push_back(ofxThreadedVideoFade(frameStart, frameEnd, fadeTarget, fadeSound, fadeVideo, fadeOnce));
                    }
                    

                    
                    bPopCommand = true;
                }
                
#ifdef USE_JACK_AUDIO
                if(c.getCommand() == "setAudioTrackToChannel"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    video[videoID].setAudioTrackToChannel(c.getArgument<int>(0), c.getArgument<int>(1), c.getArgument<int>(2));
                    bPopCommand = true;
                }
                
                if(c.getCommand() == "setAudioDevice"){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    video[videoID].setAudioDevice(c.getArgument<string>(0));
                    bPopCommand = true;
                }
#endif
                
                if(c.getCommand() == "loadMovie" && bCanLoad){
                    if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString();
                    
                    if(video[videoID].loadMovie(c.getArgument<string>(0))){
                        
                        if(bVerbose) ofLogVerbose() << instanceID << " = " << c.getCommandAsString() << " executed in thread";;

//                        lock();
                        
                        fades.clear();
                        width = video[videoID].getWidth();
                        height = video[videoID].getHeight();
                        speed = video[videoID].getSpeed();
                        duration = video[videoID].getDuration();
                        position = video[videoID].getPosition();
                        frameCurrent = video[videoID].getCurrentFrame();
                        frameTotal = video[videoID].getTotalNumFrames();
#ifdef USE_QUICKTIME_7
                        volume = video[videoID].getVolume(); // we should implement for QT6
                        pan = video[videoID].getPan();
#endif
                        loopState = video[videoID].getLoopState();
                        
                        moviePath = c.getArgument<string>(0);
#ifdef TARGET_OSX
                        vector<string> pathParts = ofSplitString(moviePath, "/");
#else
                        vector<string> pathParts = ofSplitString(moviePath, "\\");
#endif
                        movieName = pathParts[pathParts.size() - 1];
                        
                        bIsPaused = true;
                        bIsPlaying = false;
                        bIsTextureReady = false;
                        bIsLoading = false;
                        bIsMovieDone = false;
                        bLoaded = true;
                        
                        pixels = &video[videoID].getPixelsRef();

                        unlock();
                        
                        bPopCommand = true;
                        
                        ofxThreadedVideoEvent e = ofxThreadedVideoEvent(moviePath, VIDEO_EVENT_LOAD_OK, this);
                        ofNotifyEvent(threadedVideoEvent, e, this);
                        
                        ofxThreadedVideoLoadOk++;
                        
                    }else{
                        
                        ofLogError() << "Could not load: " << instanceID << " + " << c.getCommandAsString();
                        
                        ofxThreadedVideoEvent e = ofxThreadedVideoEvent(moviePath, VIDEO_EVENT_LOAD_FAIL, this);
                        ofNotifyEvent(threadedVideoEvent, e, this);
                        
                        ofxThreadedVideoLoadFail++;
                    }
                    
                }
                
            }
            
            if(bPopCommand){
                video[videoID].update();
            }
        
            lock();
            
            if(bIsFrameNew){
                for(unsigned int i = 0; i < fades.size(); i++){
                    
                    ofxThreadedVideoFade& currentFade = fades.at(i);
                    
                    if(currentFade.getIsFading(frameCurrent)){
                        _fade = currentFade.getFade(_fade, frameCurrent);
                        
                        if(currentFade.fadeVideo){
                            if(fade != _fade) fade = _fade;
                        }
#ifdef USE_QUICKTIME_7
                        if(currentFade.fadeSound){ // we should implement for QT6
                            if(video[videoID].getVolume() != _fade) video[videoID].setVolume(_fade);
                        }
#endif
                    }
                    
                    if(currentFade.fadeOnce && currentFade.getFadeDone(frameCurrent)){
                        fades.erase(fades.begin() + i);
                        i--;
                    }

                }
            }
            
            ofxThreadedVideoGlobalMutex.lock();
            
            if(bPopCommand) popCommand();
            
            ofxThreadedVideoGlobalCritical = false;
            bCriticalSection = false;
            ofxThreadedVideoGlobalMutex.unlock();
            unlock();
        }else{
            ofxThreadedVideoGlobalMutex.unlock();
            unlock();
        }
        
        ofSleepMillis(1);

    }
        
}

//--------------------------------------------------------------
void ofxThreadedVideo::pushCommand(ofxThreadedVideoCommand& c, bool back){
    ofLogNotice() << instanceID << " + push " << c.getCommandAsString();
    ofLogNotice() << instanceID << " + lock ";
    lock();
    ofLogNotice() << instanceID << " + ofxThreadedVideoGlobalMutex.lock(); ";
    ofxThreadedVideoGlobalMutex.lock();
    if(bVerbose) ofLogVerbose() << instanceID << " + push " << c.getCommandAsString();
    if(back){
        ofLogNotice() << instanceID << " + back ";
        ofxThreadedVideoCommands.push_back(c);
    }else{
        ofLogNotice() << instanceID << " + not back";
        ofxThreadedVideoCommands.push_front(c);
    }
    ofxThreadedVideoGlobalMutex.unlock();
    unlock();
    ofLogNotice() << instanceID << " + unlock ";
}

//--------------------------------------------------------------
void ofxThreadedVideo::popCommand(){
    ofxThreadedVideoCommand& c = ofxThreadedVideoCommands.front();
    if(bVerbose) ofLogVerbose() << instanceID << " - pop " << c.getCommandAsString();
    ofxThreadedVideoCommands.pop_front();
}

//--------------------------------------------------------------
ofxThreadedVideoCommand ofxThreadedVideo::getCommand(){
    if(ofxThreadedVideoCommands.size() > 0){
        return ofxThreadedVideoCommands.front();
    }else{
        return ofxThreadedVideoNullCommand;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::loadMovie(string path){
    ofLogNotice() << instanceID << " - loadMovie " << path;
    ofxThreadedVideoCommand c("loadMovie", instanceID);
    c.setArgument(path);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::play(){
    ofxThreadedVideoCommand c("play", instanceID);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::stop(){
    ofxThreadedVideoCommand c("stop", instanceID);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::close(){
    closeMovie();
}

//--------------------------------------------------------------
void ofxThreadedVideo::closeMovie(){
    //waitForThread(); ?
    lock();
    ofxThreadedVideoGlobalMutex.lock();
    
    video[0].close();
    video[1].close();
    
    bCriticalSection = false;
    bLoaded = false;
    
    bUseTexture = true;
    bIsFrameNew = false;
    bIsPlaying = false;
    bIsLoading = false;
    bIsMovieDone = false;
    
    width = 0.0f;
    height = 0.0f;
    
    speed = 0.0f;
    position = 0.0f;
    duration = 0.0f;
    
    volume = 0.0f;
    pan = 0.0f;
    
    loopState = OF_LOOP_NORMAL;
    
    frameCurrent = 0;
    frameTotal = 0;
    
    movieName = "";
    moviePath = "";
    
    fade = 1.0f;
    fades.clear();
    
    prevMillis = ofGetElapsedTimeMillis();
    lastFrameTime = timeNow = timeThen = fps = frameRate = 0;
    
    ofxThreadedVideoGlobalMutex.unlock();
    unlock();
}

//--------------------------------------------------------------
void ofxThreadedVideo::setFade(float fadeTarget){
    ofScopedLock lock(mutex);
    _fade = fade = fadeTarget;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setFade(int frameStart, int durationMillis, float fadeTarget, bool fadeSound, bool fadeVideo, bool fadeOnce){
    ofxThreadedVideoCommand c("setFade", instanceID);
    c.setArgument(frameStart);
    c.setArgument(durationMillis);
    c.setArgument(fadeTarget);
    c.setArgument(fadeSound);
    c.setArgument(fadeVideo);
    c.setArgument(fadeOnce);
    pushCommand(c);
}

//--------------------------------------------------------------
float ofxThreadedVideo::getFade(){
    ofScopedLock lock(mutex);
    return fade;
}

//--------------------------------------------------------------
void ofxThreadedVideo::clearFades(){
    ofScopedLock lock(mutex);
    fades.clear();
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isFrameNew(){
    ofScopedLock lock(mutex);
    return bIsFrameNew;
}

//--------------------------------------------------------------
float ofxThreadedVideo::getPosition(){
    ofScopedLock lock(mutex);
    return position;
}

//--------------------------------------------------------------
float ofxThreadedVideo::getSpeed(){
    ofScopedLock lock(mutex);
    return speed;
}

//--------------------------------------------------------------
float ofxThreadedVideo::getDuration(){
    ofScopedLock lock(mutex);
    return duration;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::getIsMovieDone(){
    ofScopedLock lock(mutex);
    return bIsMovieDone;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPosition(float _pct){
    _pct = CLAMP(_pct, 0.0f, 1.0f);
    ofxThreadedVideoCommand c("setPosition", instanceID);
    c.setArgument(_pct);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setVolume(float _volume){
    _volume = CLAMP(_volume, 0.0f, 1.0f);
    ofxThreadedVideoCommand c("setVolume", instanceID);
    c.setArgument(_volume);
    pushCommand(c);
}

#ifdef USE_QUICKTIME_7
//--------------------------------------------------------------
float ofxThreadedVideo::getVolume(){ // we should implement for QT6
    ofScopedLock lock(mutex);
    return volume;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPan(float _pan){
    _pan = CLAMP(_pan, -1.0f, 1.0f);
    ofxThreadedVideoCommand c("setPan", instanceID);
    c.setArgument(_pan);
    pushCommand(c);
}

//--------------------------------------------------------------
float ofxThreadedVideo::getPan(){
    ofScopedLock lock(mutex);
    return pan;
}
#endif

//--------------------------------------------------------------
void ofxThreadedVideo::setLoopState(ofLoopType state){
    ofxThreadedVideoCommand c("setLoopState", instanceID);
    c.setArgument(state);
    pushCommand(c);
}

//--------------------------------------------------------------
int ofxThreadedVideo::getLoopState(){
    ofScopedLock lock(mutex);
    return loopState;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setSpeed(float _speed){
    _speed = CLAMP(_speed, -1.9, 4.0);
    ofxThreadedVideoCommand c("setSpeed", instanceID);
    c.setArgument(_speed);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setFrame(int frame){
    ofxThreadedVideoCommand c("setFrame", instanceID);
    c.setArgument(frame);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setUseTexture(bool b){
    ofScopedLock lock(mutex);
    // this is for ofxThreadedVideo since the ofVideoPlayers
    // intances don't use textures internally
    bUseTexture = b;
}

//--------------------------------------------------------------
ofTexture& ofxThreadedVideo::getTextureReference(){
    return drawTexture;
}

// untested pixel operations!! be careful ;)

//--------------------------------------------------------------
unsigned char * ofxThreadedVideo::getPixels(){
    ofScopedLock lock(mutex);
    return pixels->getPixels();
}

//--------------------------------------------------------------
ofPixelsRef ofxThreadedVideo::getPixelsRef(){
    ofScopedLock lock(mutex);
    return *pixels;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPixelFormat(ofPixelFormat _pixelFormat){
    ofScopedLock lock(mutex);
    internalPixelFormat = _pixelFormat;
    video[0].setPixelFormat(internalPixelFormat);
    video[1].setPixelFormat(internalPixelFormat);
}

//--------------------------------------------------------------
ofPixelFormat ofxThreadedVideo::getPixelFormat(){
    ofScopedLock lock(mutex);
    return internalPixelFormat;
}

//--------------------------------------------------------------
ofPtr<ofBaseVideoPlayer> ofxThreadedVideo::getPlayer(){
    return video[0].getPlayer();
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y, float w, float h){
    ofPushStyle();
    ofSetColor(255 * fade, 255 * fade, 255 * fade, 255 * fade);
    drawTexture.draw(x, y, w, h);
    ofPopStyle();
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y){
    draw(x, y, width, height);
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(const ofPoint & p){
    draw(p.x, p.y, width, height);
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(const ofRectangle & r){
    draw(r.x, r.y, r.width, r.height);
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(){
    draw(0, 0, width, height);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAnchorPercent(float xPct, float yPct){
    ofxThreadedVideoCommand c("setAnchorPercent", instanceID);
    c.setArgument(xPct);
    c.setArgument(yPct);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAnchorPoint(float x, float y){
    ofxThreadedVideoCommand c("setAnchorPoint", instanceID);
    c.setArgument(x);
    c.setArgument(y);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::resetAnchor(){
    ofxThreadedVideoCommand c("resetAnchor", instanceID);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPaused(bool b){
    ofxThreadedVideoCommand c("setPaused", instanceID);
    c.setArgument(b);
    pushCommand(c);
}

//--------------------------------------------------------------
int ofxThreadedVideo::getCurrentFrame(){
    ofScopedLock lock(mutex);
    return frameCurrent;
}

//--------------------------------------------------------------
int ofxThreadedVideo::getTotalNumFrames(){
    ofScopedLock lock(mutex);
    return frameTotal;
}

//--------------------------------------------------------------
void ofxThreadedVideo::firstFrame(){
    setFrame(0);
}

//--------------------------------------------------------------
void ofxThreadedVideo::nextFrame(){
    setFrame(getCurrentFrame() + 1);
}

//--------------------------------------------------------------
void ofxThreadedVideo::previousFrame(){
    setFrame(getCurrentFrame() - 1);
}

//--------------------------------------------------------------
float ofxThreadedVideo::getWidth(){
    ofScopedLock lock(mutex);
    return width;
}

//--------------------------------------------------------------
float ofxThreadedVideo::getHeight(){
    ofScopedLock lock(mutex);
    return height;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isPaused(){
    ofScopedLock lock(mutex);
    return bIsPaused;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isLoading(){
    ofScopedLock lock(mutex);
    return bIsLoading;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isLoading(string path){
    ofScopedLock lock(ofxThreadedVideoGlobalMutex);
    for(unsigned int i = 0; i < ofxThreadedVideoCommands.size(); i++){
        if(ofxThreadedVideoCommands[i].getInstance() == instanceID){
            if(ofxThreadedVideoCommands[i].getCommand() == "loadMovie"){
                if(ofxThreadedVideoCommands[i].getArgument<string>(0) == path){
                    return true;
                }
            }
        }
    }
    return false;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isTextureReady(){
    ofScopedLock lock(mutex);
    return bIsTextureReady;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isLoaded(){
    ofLogNotice() << instanceID << " + isLoaded ";
    ofScopedLock lock(mutex);
    return bLoaded;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isPlaying(){
    ofScopedLock lock(mutex);
    return bIsPlaying;
}

//--------------------------------------------------------------
string ofxThreadedVideo::getMovieName(){
    ofScopedLock lock(mutex);
    return movieName;
}

//--------------------------------------------------------------
string ofxThreadedVideo::getMoviePath(){
    ofScopedLock lock(mutex);
    return moviePath;
}

//--------------------------------------------------------------
double ofxThreadedVideo::getFrameRate(){
    ofScopedLock lock(mutex);
    return frameRate;
}

#ifdef USE_JACK_AUDIO
//--------------------------------------------------------------
vector<string> ofxThreadedVideo::getAudioDevices(){
    lock();
    ofxThreadedVideoGlobalMutex.lock();
    audioDevices = video[currentVideoID].getAudioDevices();
    ofxThreadedVideoGlobalMutex.unlock();
    unlock();
    return audioDevices;
}

////--------------------------------------------------------------
//int ofxThreadedVideo::getAudioTrackList(){
////    if(currentVideoID != VIDEO_NONE){
////        return video[currentVideoID].getAudioTrackList();
////    }
////    return 0;
//}

//--------------------------------------------------------------
void ofxThreadedVideo::setAudioDevice(int deviceID){
    ofxThreadedVideoCommand c("setAudioDevice", instanceID);
    c.setArgument(deviceID);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAudioDevice(string deviceID){
    ofxThreadedVideoCommand c("setAudioDevice", instanceID);
    c.setArgument(deviceID);
    pushCommand(c);
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAudioTrackToChannel(int trackIndex, int oldChannelLabel, int newChannelLabel){
    ofxThreadedVideoCommand c("setAudioTrackToChannel", instanceID);
    c.setArgument(trackIndex);
    c.setArgument(oldChannelLabel);
    c.setArgument(newChannelLabel);
    pushCommand(c);
}
#endif

//--------------------------------------------------------------
void ofxThreadedVideo::setVerbose(bool b){
    bVerbose = b;
}

//--------------------------------------------------------------
int ofxThreadedVideo::getNextLoadID(){
    // get the free slot in our videos array
    switch (currentVideoID) {
        case VIDEO_NONE:
        case VIDEO_FLOP:
            return VIDEO_FLIP;
            break;
        default:
        case VIDEO_FLIP:
            return VIDEO_FLOP;
            break;
    }
}

//--------------------------------------------------------------
string ofxThreadedVideo::getEventTypeAsString(ofxThreadedVideoEventType eventType){
    switch (eventType){
        case VIDEO_EVENT_LOAD_OK:
            return "VIDEO_EVENT_LOAD_OK";
            break;
        default:
        case VIDEO_EVENT_LOAD_FAIL:
            return "VIDEO_EVENT_LOAD_FAIL";
            break;
    }
}
