#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofSetVerticalSync(false);
    ofSetFrameRate(2000);
    ofSetLogLevel(OF_LOG_VERBOSE);

    // uncomment to see the command queue logging
    //video1.setVerbose(true);
    //video2.setVerbose(true);
    
    files.allowExt("mov");
    files.listDir("/Users/gameover/Desktop/media"); // put a video path here with several video files in a folder

    // ofxThreadedVideo is asynchronous - ie., it executes all commands
    // on a background thread. However it now implements a FIFO queue for
    // commands - this means you can call any 'setting' command before you
    // you know the movie has loaded, as ofxThreadedVideo guarantees these
    // will be executed in order. But be careful with this!! For instance
    // you can't call video1.setFrame(ofRandom(video1.getNumTotalFrames())
    // unless you are sure the movie is loaded.
    
    video1.loadMovie(files.getPath(ofRandom(files.numFiles())));
    video1.play();
    
    video2.loadMovie(files.getPath(ofRandom(files.numFiles())));
    video2.play();
    
    ofBackground(0, 0, 0);
    
    ofAddListener(video1.threadedVideoEvent, this, &ofApp::threadedVideoEvent);
    ofAddListener(video2.threadedVideoEvent, this, &ofApp::threadedVideoEvent);
}

//--------------------------------------------------------------
void ofApp::update(){
    video1.update();
    video2.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255, 255, 255);
    video1.draw(0, 0, video1.getWidth(), video1.getHeight());
    video2.draw(video1.getWidth(), 0, video2.getWidth(), video2.getHeight());
    
    ostringstream os;
    os << "FPS: " << ofGetFrameRate() << endl;
    os << "use keys '<' '>' and  ' ' to load movies, 'p' to toggle pause," << endl;
    os << "L/R arrow keys to jump frames and U/D arrow keys to change speed" << endl;
    os << video1.getCurrentFrame() << "/" << video1.getTotalNumFrames() << " " << video1.getSpeed() << endl;
    os << video2.getCurrentFrame() << "/" << video2.getTotalNumFrames() << " " << video2.getSpeed() << endl;
    ofDrawBitmapString(os.str(), 20, ofGetHeight() - 100);
}

//--------------------------------------------------------------
void ofApp::threadedVideoEvent(ofxThreadedVideoEvent & event){
    ofLogVerbose() << "VideoEvent: " << event.eventTypeAsString << " for " << event.path;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

    switch (key) {
        case '<':
        case ',':
            video1.loadMovie(files.getPath(ofRandom(files.numFiles())));
            video1.play();
            break;
        case '>':
        case '.':
            video2.loadMovie(files.getPath(ofRandom(files.numFiles())));
            video2.play();
            break;
        case ' ':
            video1.loadMovie(files.getPath(ofRandom(files.numFiles())));
            video1.play();
            video2.loadMovie(files.getPath(ofRandom(files.numFiles())));
            video2.play();
            break;
        case 'p':
            video1.setPaused(!video1.isPaused());
            video2.setPaused(!video2.isPaused());
            break;
        case OF_KEY_LEFT:
            video1.firstFrame();
            video2.firstFrame();
            break;
        case OF_KEY_RIGHT:
            video1.setFrame(ofRandom(video1.getTotalNumFrames()));
            video2.setFrame(ofRandom(video2.getTotalNumFrames()));
            break;
        case OF_KEY_UP:
            video1.setSpeed(video1.getSpeed() + 0.2);
            video2.setSpeed(video2.getSpeed() + 0.2);
            break;
        case OF_KEY_DOWN:
            video1.setSpeed(video1.getSpeed() - 0.2);
            video2.setSpeed(video2.getSpeed() - 0.2);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    video1.setFrame(video1.getTotalNumFrames() * (float)x/ofGetWidth());
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
