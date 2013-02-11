#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

    ofSetLogLevel(OF_LOG_VERBOSE);

    files.allowExt("mov");
    files.listDir("D:/vJMedia/trains01/train1"); // put a video path here with several video files in a folder

    for(int i = 0; i < MAX_VIDEOS;i++){
        videos[i].loadMovie(files.getPath(ofRandom(files.numFiles())));
        ofAddListener(videos[i].threadedVideoEvent, this, &testApp::threadedVideoEvent);
    }
}

//--------------------------------------------------------------
void testApp::update(){
    for(int i = 0; i < MAX_VIDEOS;i++){
        videos[i].update();
    }
    if(ofGetFrameNum() % 25 == 0) videos[(int)ofRandom(MAX_VIDEOS)].loadMovie(files.getPath(ofRandom(files.numFiles())));
}

//--------------------------------------------------------------
void testApp::draw(){
    ofSetColor(255, 255, 255);
    for(int i = 0; i < MAX_VIDEOS;i++){

        videos[i].draw(i*videos[i].getWidth()/MAX_VIDEOS*2,0,
                           videos[i].getWidth()/MAX_VIDEOS*2,
                           videos[i].getHeight()/MAX_VIDEOS*2);

    }

    ofSetColor(0, 255, 0);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), 20, ofGetHeight() - 20);
}

//--------------------------------------------------------------
void testApp::threadedVideoEvent(ofxThreadedVideoEvent & event){
    ofLogVerbose() << "VideoEvent:" << event.eventTypeAsString << "for" << event.path;
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    videos[(int)ofRandom(MAX_VIDEOS)].loadMovie(files.getPath(ofRandom(files.numFiles())));
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){

}
