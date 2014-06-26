#pragma once

#include "ofMain.h"
#include "ofxOSC.h"
#include "ofxUI.h"
#include "ofxArtNet.h"
#include "ofxTCPClient.h"

class ofApp : public ofBaseApp, ofThread {
public:

    void setup();
    void exit();
    void update();
    void draw();
    
    void threadedFunction();
    
    void uiEvent(ofxUIEventArgs & args);
	
	ofxOscReceiver osc;
    ofxArtNet artnet;
    ofxTCPClient client;

    ofxUISuperCanvas *inputs;
    ofxUISuperCanvas *settings;
    ofxUIRangeSlider *range;
    ofxUISlider *size;
    ofxUISlider *fade;
    ofxUISlider *falloff;
    ofxUIImageSampler *sampler;
    ofImage spectrum;
    ofxUISuperCanvas *server;
    
    float distance[10];
    ofFbo fbo;
    ofPixels pixels;
};
