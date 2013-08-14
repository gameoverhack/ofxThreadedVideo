#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

    ofSetLogLevel(OF_LOG_VERBOSE);

    files.allowExt("mov");
    files.listDir("/Users/gameover/Desktop/DeepDataTest/ANIME60"); // put a video path here with several video files in a folder

    lastLoadTime = ofGetElapsedTimeMillis();
    loadInterval = 100;
    
    ofBackground(0, 0, 0);
    
    for(int i = 0; i < MAX_VIDEOS;i++){
        videos[i].loadMovie(files.getPath(ofRandom(files.numFiles())));
        videos[i].play();
        //videos[i].setVerbose(true);
        ofAddListener(videos[i].threadedVideoEvent, this, &testApp::threadedVideoEvent);
    }
}

//--------------------------------------------------------------
void testApp::update(){
    for(int i = 0; i < MAX_VIDEOS;i++){
        videos[i].update();
    }
    if(ofGetElapsedTimeMillis() - lastLoadTime >= loadInterval){
        lastLoadTime = ofGetElapsedTimeMillis();
        int i = (int)ofRandom(MAX_VIDEOS);
        videos[i].loadMovie(files.getPath(ofRandom(files.numFiles())));
        videos[i].play();
    }
}

//--------------------------------------------------------------
void testApp::draw(){
    ofSetColor(255, 255, 255);
    
    int xM = 0; int yM = 0;
    int tilesWide = 4;
    for(int i = 0; i < MAX_VIDEOS; i++){
        
        float width = (ofGetWidth() / tilesWide);
        float height = width * (videos[i].getHeight() / videos[i].getWidth());
        
        if(xM == tilesWide - 1) yM++;
        xM = i%tilesWide;

        videos[i].draw(xM * width, yM * height, width, height);

    }

    ofSetColor(0, 255, 0);
    
    ostringstream os;
    os << "FPS: " << ofGetFrameRate() << " loadInterval = " << loadInterval << " ms" << endl;
    for(int i = 0; i < MAX_VIDEOS;i++){
        os << i << " " << videos[i].getFrameRate() << " " << videos[i].getCurrentFrame() << " / " << videos[i].getTotalNumFrames() << " " << videos[i].getQueueSize() << endl;
    }
    ofDrawBitmapString(os.str(), 20, ofGetHeight() - 20 * MAX_VIDEOS);
}

//--------------------------------------------------------------
void testApp::threadedVideoEvent(ofxThreadedVideoEvent & event){
    ofLogVerbose() << "VideoEvent: " << event.eventTypeAsString << " for " << event.path;
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    switch (key) {
        case ' ':
        {
            int i = (int)ofRandom(MAX_VIDEOS);
            videos[i].loadMovie(files.getPath(ofRandom(files.numFiles())));
            videos[i].play();
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
