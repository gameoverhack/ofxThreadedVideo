/*
 *  ofxThreadedVideo.cpp
 *  emptyExample
 *
 *  Created by gameover on 2/02/12.
 *  Copyright 2012 trace media. All rights reserved.
 *
 */

#include "ofxThreadedVideo.h"

static ofMutex ofxThreadedVideoMutex;

//--------------------------------------------------------------
ofxThreadedVideo::ofxThreadedVideo(){

    // setup video instances
    videos[0].setUseTexture(false);
    videos[1].setUseTexture(false);
    setPixelFormat(OF_PIXELS_RGB);
    bFrameNew[0] = bFrameNew[1] = false;
    paths[0] = paths[1] = names[0] = names[1] = "";

    // set vars to default values
    loadVideoID = VIDEO_NONE;
    currentVideoID = VIDEO_NONE;
    loadPath = "";

    newPosition[0] = newPosition[1] = -1.0f;
    newFrame[0] = newFrame[1] = -1;
    bPaused[0] = bPaused[1] = false;
    bUseTexture = true;
    volume[0] = volume[1] = 255;
    balance[0] = balance[1] = .0f;
    newSpeed[0] = newSpeed[1] = 1.0f;
    newLoopType[0] = newLoopType[1] = -1;
    frame[0] = frame[1] = 0;

    bUseAutoPlay = true;
    bUseQueue = false;

    prevMillis = ofGetElapsedTimeMillis();
    lastFrameTime = timeNow = timeThen = fps = frameRate = 0;

    // let's go!
    startThread(false, false);
}

//--------------------------------------------------------------
ofxThreadedVideo::~ofxThreadedVideo(){

    // stop threading
    waitForThread(true);

    // close anything left open
    videos[0].close();
    videos[1].close();

}

//--------------------------------------------------------------
void ofxThreadedVideo::setPlayer(ofPtr<ofBaseVideoPlayer> newPlayer){
    videos[0].setPlayer(newPlayer);
    videos[1].setPlayer(newPlayer);
}

//--------------------------------------------------------------
ofPtr<ofBaseVideoPlayer> ofxThreadedVideo::getPlayer(){
    return videos[0].getPlayer();
}

//--------------------------------------------------------------
void ofxThreadedVideo::setUseAutoPlay(bool b){
    bUseAutoPlay = b;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::getUseAutoPlay(){
    return bUseAutoPlay;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setUseQueue(bool b){
    bUseQueue = b;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::getUseQueue(){
    return bUseQueue;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::loadMovie(string fileName){

    // check if we're using a queue or only allowing one file to load at a time
    if (!bUseQueue && pathsToLoad.size() > 0){
        ofLogWarning() << "Ignoring loadMovie(" << fileName << ") as we're not using a queue and a movie is already loading. Returning false. You can change this behaviour with setUseQueue(true)";

        // send event notification
        ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(loadPath, VIDEO_EVENT_LOAD_BLOCKED, this);
        ofNotifyEvent(threadedVideoEvent, videoEvent, this);

        return false;
    }

    // put the movie path in a queue
    pathsToLoad.push(ofToDataPath(fileName));
    return true;

}

//--------------------------------------------------------------
void ofxThreadedVideo::setPixelFormat(ofPixelFormat _pixelFormat){
    internalPixelFormat = _pixelFormat;
    videos[0].setPixelFormat(internalPixelFormat);
    videos[1].setPixelFormat(internalPixelFormat);
}

//--------------------------------------------------------------
void ofxThreadedVideo::closeMovie(){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].closeMovie();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::close(){
    closeMovie();
}

//--------------------------------------------------------------
void ofxThreadedVideo::update(){

    if(lock()){

        // check if we're loading a video
        if(loadVideoID != VIDEO_NONE){

            float w = videos[loadVideoID].getWidth();
            float h = videos[loadVideoID].getHeight();

            // allocate a texture if the one we have is a different size
            if(bUseTexture && (textures[loadVideoID].getWidth() != w || textures[loadVideoID].getHeight() != h)){
                ofLogVerbose() << "Allocating texture" << loadVideoID << w << h;
                textures[loadVideoID].allocate(w, h, ofGetGLTypeFromPixelFormat(internalPixelFormat));
            }

            // check for a new frame before loading video
            if(bFrameNew[loadVideoID]){

                // switch the current movie ID to the one we just loaded
                int lastVideoID = currentVideoID;
                currentVideoID = loadVideoID;
                loadVideoID = VIDEO_NONE;

                // close the last movie - we do this here because
                // ofQuicktimeVideo chokes if you try to close in a thread
                if(lastVideoID != VIDEO_NONE){
                    ofLogVerbose() << "Closing last video" << lastVideoID;
                    paths[lastVideoID] = names[lastVideoID] = "";
                    videos[lastVideoID].stop();

                    // reset properties to defaults
                    newPosition[lastVideoID] = -1.0f;
                    newFrame[lastVideoID] = -1;
                    newSpeed[lastVideoID] = 1.0f;
                    newLoopType[lastVideoID] = -1;
                    frame[lastVideoID] = 0;

                    bFrameNew[lastVideoID] = false;
                    bPaused[lastVideoID] = false;
                    volume[lastVideoID] = 255;
                    balance[lastVideoID] = .0f;
                }

                // send event notification
                ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(paths[currentVideoID], VIDEO_EVENT_LOAD_OK, this);
                ofNotifyEvent(threadedVideoEvent, videoEvent, this);
            }
        }

        // check for a new frame for current video
        updateTexture(currentVideoID);

        // if there's a movie in the queue
        if(pathsToLoad.size() > 0 && loadPath == "" && loadVideoID == VIDEO_NONE){
            // ...let's start trying to load it!
            loadPath = pathsToLoad.front();
            pathsToLoad.pop();
        };

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

        unlock();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::updatePixels(int videoID){
    videos[videoID].update();
    if (videos[videoID].isFrameNew()){
        bFrameNew[videoID] = true;
        frame[videoID] = videos[videoID].getCurrentFrame();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::updateTexture(int videoID){
    if(videoID != VIDEO_NONE){
        if(bUseTexture){
            // make sure we don't have NULL pixels
            if(pixels[videoID]->getPixels() != NULL && textures[videoID].isAllocated()){
                float w = videos[videoID].getWidth();
                float h = videos[videoID].getHeight();
                textures[videoID].loadData(pixels[videoID]->getPixels(), w, h, ofGetGLTypeFromPixelFormat(internalPixelFormat));
            }
        }
        bFrameNew[videoID] = false;
    }
}

void ofxThreadedVideo::updateVideo(int videoID){

    if(videoID != VIDEO_NONE){

        // set loop type
        if(newLoopType[videoID] != -1){
            videos[videoID].setLoopState((ofLoopType)newLoopType[videoID]);
            newLoopType[videoID] = -1;
        }

        // set speed
        if(newSpeed[videoID] != videos[videoID].getSpeed()){
            videos[videoID].setSpeed(newSpeed[videoID]);
        }

        // do pause, or...
        if (bPaused[videoID] && !videos[videoID].isPaused()){
            videos[videoID].setPaused(true);
        }

        // ...do unpause
        if (!bPaused[videoID] && videos[videoID].isPaused()){
            videos[videoID].setPaused(false);
        }

        // do non blocking seek to position
        if(newPosition[videoID] != -1.0f){
            if(!bPaused[videoID]) videos[videoID].setPaused(true);
            videos[videoID].setPosition(newPosition[videoID]);
        }

        // do non blocking seek to frame
        if(newFrame[videoID] != -1){
            CLAMP(newFrame[videoID], 0, videos[videoID].getTotalNumFrames());
            videos[videoID].setFrame(newFrame[videoID]);
        }

        // update current video
        updatePixels(videoID);

        // unpause if doing a non blocking seek to position
        if(newPosition[videoID] != -1.0f && !bPaused[videoID]) videos[videoID].setPaused(false);

        newPosition[videoID] = -1.0f;
        newFrame[videoID] = -1;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::threadedFunction(){

    while (isThreadRunning()){

        if(lock()){

            // if there's a movie to load...
            if(loadPath != ""){

                loadVideoID = getNextLoadID();

                paths[loadVideoID] = loadPath;
                loadPath = "";
#ifdef TARGET_OSX
                vector<string> pathParts = ofSplitString(paths[loadVideoID], "/");
#else
                vector<string> pathParts = ofSplitString(paths[loadVideoID], "\\");
#endif
                names[loadVideoID] = pathParts[pathParts.size() - 1];

                // using a static mutex blocks all threads (including the main app) until we've loaded
                ofxThreadedVideoMutex.lock();

                // load that movie!
                if(videos[loadVideoID].loadMovie(paths[loadVideoID])){

                    ofLogVerbose() << "Loaded" << names[loadVideoID] << " " << loadVideoID;

                    // start rolling if AutoPlay is true
                    if (bUseAutoPlay) videos[loadVideoID].play();

                    // set pixel refs
                    pixels[loadVideoID] = &videos[loadVideoID].getPixelsRef();

                }else{

                    ofLogVerbose() << "Could not load video";
                    loadVideoID = VIDEO_NONE;

                    // send event notification
                    ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(paths[loadVideoID], VIDEO_EVENT_LOAD_FAIL, this);
                    ofNotifyEvent(threadedVideoEvent, videoEvent, this);
                }


                ofxThreadedVideoMutex.unlock();

            }

            // do threaded update of videos
            updateVideo(currentVideoID);
            updateVideo(loadVideoID);

            unlock();

            // sleep a bit so we don't thrash the cores!!
            ofSleepMillis(1000/30); // TODO: implement target frame rate? US might need 30 here?

        }
    }
}

//--------------------------------------------------------------
int ofxThreadedVideo::getNextLoadID(){
    // get the free slot in our videos array
    switch (currentVideoID) {
        case VIDEO_NONE:
        case VIDEO_FLOP:
            return VIDEO_FLIP;
            break;
        case VIDEO_FLIP:
            return VIDEO_FLOP;
            break;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::play(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].play();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::stop(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].stop();
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isFrameNew(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return bFrameNew[currentVideoID];
    }else{
        return false;
    }
}

//--------------------------------------------------------------
unsigned char * ofxThreadedVideo::getPixels(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return pixels[currentVideoID]->getPixels();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
ofPixelsRef ofxThreadedVideo::getPixelsRef(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getPixelsRef();
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getPosition(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getPosition();
    }else{
        return 0;
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getSpeed(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getSpeed();
    }else{
        return 0;
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getDuration(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getDuration();
    }else{
        return 0;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::getIsMovieDone(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getIsMovieDone();
    }else{
        return false;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPosition(float pct){
    Poco::ScopedLock<ofMutex> lock();
    CLAMP(pct, 0.0f, 1.0f);
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        newPosition[currentVideoID] = pct;
    }
    newPosition[getNextLoadID()] = pct;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setVolume(int _volume){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        volume[currentVideoID] = _volume;
        videos[currentVideoID].setVolume(volume[currentVideoID]);
    }
    volume[getNextLoadID()] = _volume;
    videos[getNextLoadID()].setVolume(volume[getNextLoadID()]);
}

//--------------------------------------------------------------
int ofxThreadedVideo::getVolume(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return volume[currentVideoID]; // videos[currentVideoID].getVolume(); this should be implemented in OF!
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setBalance(float _balance){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        balance[currentVideoID] = _balance;
        videos[currentVideoID].setBalance(balance[currentVideoID]);
    }
    balance[getNextLoadID()] = _balance;
    videos[getNextLoadID()].setBalance(balance[getNextLoadID()]);
}

//--------------------------------------------------------------
float ofxThreadedVideo::getBalance(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return balance[currentVideoID];
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setLoopState(ofLoopType state){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        newLoopType[currentVideoID] = state;
    }
    newLoopType[getNextLoadID()] = state;
}

//--------------------------------------------------------------
int ofxThreadedVideo::getLoopState(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getLoopState();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setSpeed(float speed){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        newSpeed[currentVideoID] = speed;
    }
    newSpeed[getNextLoadID()] = speed;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setFrame(int frame){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        newFrame[currentVideoID] = frame;
    }
    newFrame[getNextLoadID()] = frame;
}

//--------------------------------------------------------------
void ofxThreadedVideo::setUseTexture(bool b){
    Poco::ScopedLock<ofMutex> lock();
    // this is for ofxThreadedVideo since the ofVideoPlayers
    // intances don't use textures internally
    bUseTexture = b;
}

//--------------------------------------------------------------
ofTexture & ofxThreadedVideo::getTextureReference(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return textures[currentVideoID];
    }else{
        //ofLogVerbose() << "No video loaded. Returning garbage";
        return textures[0];
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y, float w, float h){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(x, y, w, h);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(x, y, videos[currentVideoID].getWidth(), videos[currentVideoID].getHeight());
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(const ofPoint & p){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(p);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(const ofRectangle & r){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(r);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(0, 0, videos[currentVideoID].getWidth(), videos[currentVideoID].getHeight());
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAnchorPercent(float xPct, float yPct){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].setAnchorPoint(xPct, yPct);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAnchorPoint(float x, float y){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].setAnchorPoint(x, y);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::resetAnchor(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].resetAnchor();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPaused(bool b){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        bPaused[currentVideoID] = b;
    }
    bPaused[getNextLoadID()] = b;
}

//--------------------------------------------------------------
int ofxThreadedVideo::getCurrentFrame(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return frame[currentVideoID];
    }else{
        return 0;
    }
}

//--------------------------------------------------------------
int ofxThreadedVideo::getTotalNumFrames(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getTotalNumFrames();
    }else{
        return 0;
    }
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
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getWidth();
    }else{
        return 0;
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getHeight(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getHeight();
    }else{
        return 0;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isPaused(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].isPaused();
        //return bPaused;
    }else{
        return false;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isLoaded(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        return videos[currentVideoID].isLoaded();
        //return bPaused;
    }else{
        return false;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isPlaying(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].isPlaying();
        //return bPaused;
    }else{
        return false;
    }
}

//--------------------------------------------------------------
string ofxThreadedVideo::getName(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return names[currentVideoID];
    }else{
        return "";
    }
}

//--------------------------------------------------------------
string ofxThreadedVideo::getPath(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID != VIDEO_NONE){
        return paths[currentVideoID];
    }else{
        return "";
    }
}

//--------------------------------------------------------------
double ofxThreadedVideo::getFrameRate(){
    Poco::ScopedLock<ofMutex> lock();
    return frameRate;
}

//--------------------------------------------------------------
string ofxThreadedVideo::getEventTypeAsString(ofxThreadedVideoEventType eventType){
    switch (eventType){
        case VIDEO_EVENT_LOAD_OK:
            return "VIDEO_EVENT_LOAD_OK";
            break;
        case VIDEO_EVENT_LOAD_FAIL:
            return "VIDEO_EVENT_LOAD_FAIL";
            break;
        case VIDEO_EVENT_LOAD_BLOCKED:
            return "VIDEO_EVENT_LOAD_BLOCKED";
        case VIDEO_EVENT_LOAD_THREADBLOCKED:
            return "VIDEO_EVENT_LOAD_THREADBLOCKED";
            break;
    }
}
