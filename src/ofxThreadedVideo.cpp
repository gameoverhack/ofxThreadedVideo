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

    newPosition = -1.0f;
    newFrame = -1;
    bPaused = false;
    bUseTexture = true;

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
    if (!bUseQueue && pathsToLoad.size() > 0 || !lock()) {
        ofLogWarning() << "Ignoring loadMovie(" << fileName << ") as we're not using a queue and a movie is already loading. Returning false. You can change this behaviour with setUseQueue(true)";

        // send event notification
        ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(loadPath, VIDEO_EVENT_LOAD_BLOCKED, this);
        ofNotifyEvent(threadedVideoEvent, videoEvent, this);

        return false;
    }

    // put the movie path in a queue
    pathsToLoad.push(ofToDataPath(fileName));
    unlock();
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
        return videos[currentVideoID].closeMovie();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::close(){
    closeMovie();
}

//--------------------------------------------------------------
void ofxThreadedVideo::update(){

    if(lock()){

        // check for a new frame for current video
        updateTexture(currentVideoID);

        // check if we're loading a video
        if(loadVideoID != VIDEO_NONE){

            float w = videos[loadVideoID].getWidth();
            float h = videos[loadVideoID].getHeight();

            // allocate a texture if the one we have is a different size
            if(bUseTexture && (textures[loadVideoID].getWidth() != w || textures[loadVideoID].getHeight() != h)){
                ofLogVerbose() << "Allocating texture" << loadVideoID << w << h;
                textures[loadVideoID].allocate(w, h, ofGetGLTypeFromPixelFormat(internalPixelFormat));
            }

            // check for a new frame for loading video
            if(bFrameNew[loadVideoID]){
                updateTexture(loadVideoID);

                // switch the current movie ID to the one we just loaded
                int lastVideoID = currentVideoID;
                currentVideoID = loadVideoID;
                loadVideoID = VIDEO_NONE;

                // close the last movie - we do this here because
                // ofQuicktimeVideo chokes if you try to close in a thread
                if(lastVideoID != VIDEO_NONE){
                    ofLogVerbose() << "Closing last video" << lastVideoID;
                    paths[lastVideoID] = names[lastVideoID] = "";
                    videos[lastVideoID].close();
                    bFrameNew[lastVideoID] = false;
                    // reset properties ??
                    newPosition = -1.0f;
                    newFrame = -1;
                    bPaused = false;
                    //bUseTexture = true;
                }

                // send event notification
                ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(paths[currentVideoID], VIDEO_EVENT_LOAD_OK, this);
                ofNotifyEvent(threadedVideoEvent, videoEvent, this);
            }
        }

        // if there's a movie in the queue
        if(pathsToLoad.size() > 0){
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
        // get the pixels
        pixels[videoID] = &videos[videoID].getPixelsRef();
        // make sure we don't have NULL pixels
        if(pixels[videoID]->getPixels() != NULL){
            bFrameNew[videoID] = true;
        }
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::updateTexture(int videoID){
    if(videoID != VIDEO_NONE){
        if(bUseTexture){
            float w = videos[videoID].getWidth();
            float h = videos[videoID].getHeight();
            textures[videoID].loadData(pixels[videoID]->getPixels(), w, h, ofGetGLTypeFromPixelFormat(internalPixelFormat));
        }

        bFrameNew[videoID] = false;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::threadedFunction(){

    while (isThreadRunning()){

        if(lock()){

            // if there's a movie to load...
            if(loadPath != ""){

                // get the free slot in our videos array
                switch (currentVideoID) {
                    case VIDEO_NONE:
                    case VIDEO_FLOP:
                        loadVideoID = VIDEO_FLIP;
                        break;
                    case VIDEO_FLIP:
                        loadVideoID = VIDEO_FLOP;
                        break;
                }

                // using a static mutex blocks all threads (including the main app) until we've loaded
                ofxThreadedVideoMutex.lock();

                // load that movie!
                if(videos[loadVideoID].loadMovie(loadPath)){

                    ofLogVerbose() << "Loaded" << loadPath;

                    // start rolling if AutoPlay is true
                    if (bUseAutoPlay) videos[loadVideoID].play();

                    paths[loadVideoID] = loadPath;
                    vector<string> pathParts = ofSplitString(paths[loadVideoID], "/");
                    names[loadVideoID] = pathParts[pathParts.size() - 1];
                    bFrameNew[loadVideoID] = false;

                }else{

                    ofLogVerbose() << "Could not load video";
                    loadVideoID = VIDEO_NONE;

                    // send event notification
                    ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(loadPath, VIDEO_EVENT_LOAD_FAIL, this);
                    ofNotifyEvent(threadedVideoEvent, videoEvent, this);
                }

                ofxThreadedVideoMutex.unlock();
                loadPath = "";
            }

            if(loadVideoID != VIDEO_NONE){
                updatePixels(loadVideoID);
            }

            // if we have a movie let's update it
            if(currentVideoID != VIDEO_NONE){

                if (bPaused && !videos[currentVideoID].isPaused()) {
                    videos[currentVideoID].setPaused(true);
                }

                if (!bPaused && videos[currentVideoID].isPaused()) {
                    videos[currentVideoID].setPaused(false);
                }

                // do non blocking seek to position
                if(newPosition != -1.0f){
                    if(!bPaused) videos[currentVideoID].setPaused(true);
                    videos[currentVideoID].setPosition(newPosition);
                }

                // do non blocking seek to frame
                if(newFrame != -1) videos[currentVideoID].setFrame(newFrame);

                // update current video
                updatePixels(currentVideoID);

                // unpause if doing a non blocking seek to position
                if(newPosition != -1.0f && !bPaused) videos[currentVideoID].setPaused(false);

                newPosition = -1.0f;
                newFrame = -1;
            }

            unlock();

            // sleep a bit so we don't thrash the cores!!
            ofSleepMillis(1000/25); // TODO: implement target frame rate? US might need 30 here?

        }
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::play(){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].play();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::stop(){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].stop();
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isFrameNew(){
    if(currentVideoID != VIDEO_NONE){
        //return videos[currentVideoID].isFrameNew();
        return bFrameNew[currentVideoID];
    }else{
        return false;
    }
}

//--------------------------------------------------------------
unsigned char * ofxThreadedVideo::getPixels(){
    if(currentVideoID != VIDEO_NONE){
        //return videos[currentVideoID].getPixels();
        return pixels[currentVideoID]->getPixels();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
ofPixelsRef ofxThreadedVideo::getPixelsRef(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getPixelsRef();
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getPosition(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getPosition();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getSpeed(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getSpeed();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getDuration(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getDuration();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::getIsMovieDone(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getIsMovieDone();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPosition(float pct){
    if(currentVideoID != VIDEO_NONE){
        CLAMP(pct, 0.0f, 1.0f);
        newPosition = pct;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setVolume(int volume){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].setVolume(volume);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setLoopState(ofLoopType state){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].setLoopState(state);
    }
}

//--------------------------------------------------------------
int ofxThreadedVideo::getLoopState(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getLoopState();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setSpeed(float speed){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].setSpeed(speed);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setFrame(int frame){
    if(currentVideoID != VIDEO_NONE){
        CLAMP(frame, 0, videos[currentVideoID].getTotalNumFrames());
        newFrame = frame;
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setUseTexture(bool b){
    bUseTexture = b;
}

//--------------------------------------------------------------
ofTexture & ofxThreadedVideo::getTextureReference(){
    if(currentVideoID != VIDEO_NONE){
        return textures[currentVideoID];
    }else{
        //ofLogVerbose() << "No video loaded. Returning garbage";
        return textures[0];
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y, float w, float h){
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(x, y, w, h);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y){
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(x, y, videos[currentVideoID].getWidth(), videos[currentVideoID].getHeight());
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(const ofPoint & p){
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(p);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(const ofRectangle & r){
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(r);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(){
    if(currentVideoID != VIDEO_NONE && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(0, 0, videos[currentVideoID].getWidth(), videos[currentVideoID].getHeight());
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAnchorPercent(float xPct, float yPct){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].setAnchorPoint(xPct, yPct);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setAnchorPoint(float x, float y){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].setAnchorPoint(x, y);
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::resetAnchor(){
    if(currentVideoID != VIDEO_NONE){
        videos[currentVideoID].resetAnchor();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::setPaused(bool b){
    bPaused = b;
}

//--------------------------------------------------------------
int ofxThreadedVideo::getCurrentFrame(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getCurrentFrame();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
int ofxThreadedVideo::getTotalNumFrames(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getTotalNumFrames();
    }else{
        return NULL;
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
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getWidth();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
float ofxThreadedVideo::getHeight(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].getHeight();
    }else{
        return NULL;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isPaused(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].isPaused();
        //return bPaused;
    }else{
        return false;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isLoaded(){
    if(currentVideoID != VIDEO_NONE && loadVideoID == VIDEO_NONE){
        return videos[currentVideoID].isLoaded();
        //return bPaused;
    }else{
        return false;
    }
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isPlaying(){
    if(currentVideoID != VIDEO_NONE){
        return videos[currentVideoID].isPlaying();
        //return bPaused;
    }else{
        return false;
    }
}

//--------------------------------------------------------------
string ofxThreadedVideo::getName(){
    if(currentVideoID != VIDEO_NONE){
        return names[currentVideoID];
    }else{
        return "";
    }
}

//--------------------------------------------------------------
string ofxThreadedVideo::getPath(){
    if(currentVideoID != VIDEO_NONE){
        return paths[currentVideoID];
    }else{
        return "";
    }
}

//--------------------------------------------------------------
double ofxThreadedVideo::getFrameRate(){
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
            break;
    }
}
