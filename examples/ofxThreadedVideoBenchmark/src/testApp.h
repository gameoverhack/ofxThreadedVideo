#pragma once

#include "ofMain.h"
#include "ofxThreadedVideo.h"

#define MAX_VIDEOS 8

class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

        int lastLoadTime, loadInterval;
        ofDirectory files;

        ofxThreadedVideo videos[MAX_VIDEOS];

        void threadedVideoEvent(ofxThreadedVideoEvent & event);

};
