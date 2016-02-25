#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofSetVerticalSync(false);
    ofSetFrameRate(2000);
    ofSetLogLevel(OF_LOG_VERBOSE);

    files.allowExt("mov");
    files.listDir("/Users/gameover/Desktop/media"); // put a video path here with several video files in a folder

    maxVideos = MIN(files.size(), 10);
    
    lastLoadTime = ofGetElapsedTimeMillis();
    loadInterval = 100;
    
    ofBackground(0, 0, 0);
    
    videos.resize(maxVideos);
    
    for(int i = 0; i < maxVideos;i++){
        videos[i] = new ofxThreadedVideo;
        videos[i]->setPixelFormat(OF_PIXELS_BGRA); // accepts RGB, RGBA, BGRA, YUY2 (but you need a shader for that)
        videos[i]->loadMovie(files.getPath(ofRandom(files.size())));
        videos[i]->play();
        //videos[i]->setVerbose(true);
        ofAddListener(videos[i]->threadedVideoEvent, this, &ofApp::threadedVideoEvent);
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    for(int i = 0; i < maxVideos;i++){
        videos[i]->update();
    }
    if(ofGetElapsedTimeMillis() - lastLoadTime >= loadInterval){
        lastLoadTime = ofGetElapsedTimeMillis();
        int i = (int)ofRandom(maxVideos);
        videos[i]->loadMovie(files.getPath(ofRandom(files.size())));
        videos[i]->play();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255, 255, 255);
    
    int xM = 0; int yM = 0;
    int tilesWide = 10;
    for(int i = 0; i < maxVideos; i++){
        
        float width = (ofGetWidth() / tilesWide);
        float height = width * (videos[i]->getHeight() / videos[i]->getWidth());
        
        if(xM == tilesWide - 1) yM++;
        xM = i%tilesWide;

        videos[i]->draw(xM * width, yM * height, width, height);

    }

    ofSetColor(0, 255, 0);
    
    ostringstream os;
    os << "FPS: " << ofGetFrameRate() << " loadInterval = " << loadInterval << " ms" << endl;
    for(int i = 0; i < maxVideos; i++){
        os << i << " " << videos[i]->getFrameRate() << " " << videos[i]->getCurrentFrame() << " / " << videos[i]->getTotalNumFrames() << " " << videos[i]->getQueueSize() << endl;
    }
    ofDrawBitmapString(os.str(), 20, ofGetHeight() - 20 * maxVideos);
}

//--------------------------------------------------------------
void ofApp::threadedVideoEvent(ofxThreadedVideoEvent & event){
    ofLogVerbose() << "VideoEvent: " << event.eventTypeAsString << " for " << event.path;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key) {
        case ' ':
        {
            int i = (int)ofRandom(maxVideos);
            videos[i]->loadMovie(files.getPath(ofRandom(files.numFiles())));
            videos[i]->play();
            break;
        }
        case OF_KEY_UP:
            loadInterval = loadInterval + 10;
            break;
        case OF_KEY_DOWN:
            loadInterval = loadInterval - 10;
            break;
    }
    CLAMP(loadInterval, 0, INFINITY);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
