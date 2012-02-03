#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    files.allowExt("mov");
    files.listDir("/Users/gameover/Desktop/vjMedia/trains01/"); // put a video path here with several video files in a folder
    
    video1.loadMovie(files.getPath(ofRandom(files.numFiles())));
    video2.loadMovie(files.getPath(ofRandom(files.numFiles())));
}

//--------------------------------------------------------------
void testApp::update(){
    video1.update();
    video2.update();
}

//--------------------------------------------------------------
void testApp::draw(){
    ofSetColor(255, 255, 255);
    video1.draw(0, 0, 640, 480);
    video2.draw(640, 0, 640, 480);
    
    ofSetColor(0, 255, 0);
    int duration1 = video1.getVideo().getDuration();
    int duration2 = video2.getVideo().getDuration();
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()) + " duration vid1: " + ofToString(duration1) + " duration vid2: " + ofToString(duration2), 20, ofGetHeight() - 20);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    switch (key) {
        case '1':
            video1.loadMovie(files.getPath(ofRandom(files.numFiles())));
            break;
        case '2':
            video2.loadMovie(files.getPath(ofRandom(files.numFiles())));
            break; 
        case '3':
            video1.loadMovie(files.getPath(ofRandom(files.numFiles())));
            video2.loadMovie(files.getPath(ofRandom(files.numFiles())));
            break; 
        default:
            break;
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    ofVideoPlayer & video = video1.getVideo();
    video1.setFastFrame(video.getTotalNumFrames() * (float)x/ofGetWidth());
    //video2.setFastPosition((float)x/ofGetWidth());
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    //bool bPaused = video1.isFastPaused();
    //video1.setFastPaused(true);
    //video2.setFastPaused(true);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    //bool bPaused = video1.isFastPaused();
    //video1.setFastPaused(false);
    //video2.setFastPaused(false);
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