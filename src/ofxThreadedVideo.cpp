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

    videos[0].setUseTexture(false);
    videos[0].setPixelFormat(OF_PIXELS_RGB);
    videos[1].setUseTexture(false);
    videos[1].setPixelFormat(OF_PIXELS_RGB);
    paths[0] = paths[1] = "";
    
    loadVideoID = -1;
    currentVideoID = -1;
    videoIDCounter = -1;
    loadPath = "";
    
    bUseAutoPlay = true;
    bUseQueue = false;
    bFastPaused = false;
    bNewFrame = false;
    
    startThread(false, false);
}

//--------------------------------------------------------------
ofxThreadedVideo::~ofxThreadedVideo(){
    waitForThread(true);
    videos[0].close();
    videos[1].close();
}

//--------------------------------------------------------------
bool ofxThreadedVideo::loadMovie(string fileName){
    
    // check if we're using a queue or only allowing one file to load at a time
    if (!bUseQueue && pathsToLoad.size() > 0) {
        ofLogWarning() << "Ignoring loadMovie(" << fileName << ") as we're not using a queue and a movie is already loading. Returning false. You can change this behaviour with setUseQueue(true)";
        
        // send event notification
        ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(loadPath, VIDEO_EVENT_LOAD_BLOCKED, NULL);
        ofNotifyEvent(threadedVideoEvent, videoEvent, this);
        
        return false;
    }
    
    // put the movie path in a queue
    pathsToLoad.push(ofToDataPath(fileName));
    return true;
}

//--------------------------------------------------------------
void ofxThreadedVideo::update(){
    
    if(lock()){
        
        // check for a new frame
        if(currentVideoID != -1 && bNewFrame){
            
            float w = videos[currentVideoID].getWidth();
            float h = videos[currentVideoID].getHeight();
            
            // get the pixels
            ofPixels & pixels = videos[currentVideoID].getPixelsRef();
            
            // make sure we don't have NULL pixels (happens sometimes!)
            if(pixels.getPixels() != NULL){
                textures[currentVideoID].loadData(pixels.getPixels(), w, h, GL_RGB);
            }
            
            bNewFrame = false;
        }
        
        // check if we're loading a video
        if(loadVideoID != -1){
            
            float w = videos[loadVideoID].getWidth();
            float h = videos[loadVideoID].getHeight();
            
            // allocate a texture if the one we have is a different size
            if(textures[loadVideoID].getWidth() != w || textures[loadVideoID].getHeight() != h){
                ofLogVerbose() << "Allocating texture" << loadVideoID << w << h;
                textures[loadVideoID].allocate(w, h, GL_RGB);
            }
            
            // close the last movie - we do this here because
            // ofQuicktimeVideo chokes if you try to close in a thread
            if(currentVideoID != -1){
                paths[currentVideoID] = "";
                videos[currentVideoID].close();
            }
            
            // switch the current movie ID to the one we just loaded
            currentVideoID = loadVideoID;
            loadVideoID = -1;
            
            // send event notification
            ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(paths[currentVideoID], VIDEO_EVENT_LOAD_OK, &videos[currentVideoID]);
            ofNotifyEvent(threadedVideoEvent, videoEvent, this);
        }
        
        // if there's a movie in the queue
        if(pathsToLoad.size() > 0){
            // ...let's start trying to load it!
            loadPath = pathsToLoad.front();
            pathsToLoad.pop();
        }
        
        unlock();
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::threadedFunction(){
    
    while (isThreadRunning()){
        
        if(lock()){
            
            // if there's a movie to load...
            if(loadPath != ""){
                
                // get the free slot in our videos array
                // by using mod 2 of a load counter (flip-flop)
                videoIDCounter++;
                loadVideoID = videoIDCounter % 2;
                
                // lock a static mutex shared between all instances
                // we do this in case 2+ instances are trying to load
                // simultaneously; if we don't then Quicktime will choke
                // as it can only really handle 1 request at time reliably
                ofxThreadedVideoMutex.lock();
                
                // load that movie!
                if(videos[loadVideoID].loadMovie(loadPath)){

                    ofLogVerbose() << "Loaded" << loadPath;

                    // start rolling if AutoPlay is true
                    if (bUseAutoPlay) videos[loadVideoID].play();
                    
                    ofxThreadedVideoMutex.unlock();
                    paths[loadVideoID] = loadPath;
                    
                }else{
                    
                    ofLogVerbose() << "Could not load video";
                    loadVideoID = -1;
                    
                    // send event notification
                    ofxThreadedVideoEvent videoEvent = ofxThreadedVideoEvent(loadPath, VIDEO_EVENT_LOAD_FAIL, NULL);
                    ofNotifyEvent(threadedVideoEvent, videoEvent, this);
                }
                
                loadPath = "";
            }
            
            // if we have a movie let's update it
            if(currentVideoID != -1){
                
                // FastPause is a hack to avoid the glitchiness of
                // calling setPaused(true/false) when using Quicktime
                // - basically intead of using SetMovieRate(0)
                // as it is in ofQuicktimePlayer we just don't
                // update the movie...it stays in one place since
                // we're not calling MoviesTask(moviePtr,0);
                // not sure what happens with gStreamer...
                
                if (!bFastPaused) videos[currentVideoID].update();
                if (videos[currentVideoID].isFrameNew()) bNewFrame = true;
            }
            
            unlock();
        }
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(){
    if(currentVideoID != -1 && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(0, 0, videos[currentVideoID].getWidth(), videos[currentVideoID].getHeight());
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y){
    if(currentVideoID != -1 && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(x, y, videos[currentVideoID].getWidth(), videos[currentVideoID].getHeight());
    }
}

//--------------------------------------------------------------
void ofxThreadedVideo::draw(float x, float y, float w, float h){
    if(currentVideoID != -1 && textures[currentVideoID].isAllocated()){
        textures[currentVideoID].draw(x, y, w, h);
    }
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
void ofxThreadedVideo::setFastPaused(bool b){
    bFastPaused = b;
}

//--------------------------------------------------------------
bool ofxThreadedVideo::isFastPaused(){
    return bFastPaused;
}

//--------------------------------------------------------------
ofTexture & ofxThreadedVideo::getTextureReference(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID == -1){
        ofLogVerbose() << "No video loaded. Returning garbage";
        return textures[0];
    }
    return textures[currentVideoID];
}

//--------------------------------------------------------------
ofVideoPlayer & ofxThreadedVideo::getVideo(){
    Poco::ScopedLock<ofMutex> lock();
    if(currentVideoID == -1){
        ofLogVerbose() << "No video loaded. Returning garbage";
        return videos[0];
    }
    return videos[currentVideoID];
}

//--------------------------------------------------------------
string ofxThreadedVideo::getPath(){
    Poco::ScopedLock<ofMutex> lock();
    return paths[currentVideoID];
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