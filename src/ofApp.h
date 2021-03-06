#pragma once

#include "ofMain.h"
#include "ofxOSC.h"
#include "ofxUI.h"
#include "ofxArtNet.h"
#include "ofxTCPClient.h"
#include "ofxMidi.h"

class ofApp : public ofBaseApp, ofThread, ofxMidiListener {
public:

    void setup();
    void exit();
    void update();
    void draw();
    
    void threadedFunction();
    
    void uiEvent(ofxUIEventArgs & args);
    void artnetPollReply(ofxArtNetNodeEntry* & node);
    
    void newMidiMessage(ofxMidiMessage& msg);
	
	ofxOscReceiver osc;
    ofxArtNet artnet;
    ofxTCPClient client;

    ofxUISuperCanvas *inputs;
    
    ofxUISuperCanvas *settings;
    ofxUIRangeSlider *range;
    ofxUISlider *size;
    ofxUISlider *fade;
    ofxUISlider *falloff;
    ofxUISlider *filter;
    ofxUIImageSampler *sampler;
    ofImage spectrum;
    ofxUISlider *saturation;
    ofxUISlider *brightness;
    ofxUIToggle *rainbow;
    ofxUISlider *rbspeed;
    ofxUIToggle *spiral;
    ofxUISlider *sspeed;
    ofxUIToggle *baseline;
    ofxUIToggle *bonus;
    
    ofxUISuperCanvas *server;
    
    int history[10];
    int distance[10];
    int calibration[10];
    float value[10];
    unsigned long long time[10];
    ofFbo fbo;
    ofPixels pixels;
    
    ofxMidiIn midi;
};
