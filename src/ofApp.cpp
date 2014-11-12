#include "ofApp.h"
#include <stdlib.h>

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofBackground(40);
    ofSetFrameRate(60);

    osc.setup(9999);
    
    vector<ofxArtNetInterface> interfaces;
    ofxArtNetInterface::getInterfaces(interfaces);
    if (interfaces.size() > 0)
        artnet.init(interfaces[interfaces.size()-1].ip, false);
    artnet.setNodeType(ARTNET_TYPE_RAW);
    artnet.setBroadcastLimit(30);
    ofAddListener(artnet.pollReply, this, &ofApp::artnetPollReply);
    artnet.start();
    artnet.sendPoll();
    
    inputs = new ofxUISuperCanvas("INPUTS", 20., 20., 200., 200.);
    for (int i=0; i<10; i++) {
        inputs->addMinimalSlider("distance" + ofToString(i+1), 0., 20000., 0., 100., 8.);
        inputs->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
        ofxUILabel* label = inputs->addLabel("fps" + ofToString(i+1), "0");
        ofxUIRectangle* rect = label->getRect();
        label->setFont(inputs->getFontSmall());
        label->setPadding(8);
        inputs->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    }
    filter = inputs->addMinimalSlider("filter", 0., 20000., 500.);
    range = inputs->addRangeSlider("range", 0., 20000., 800., 5000.);
    inputs->addLabelButton("calibrate", false);
    inputs->addLabelButton("poll", false);
    inputs->autoSizeToFitWidgets();
    ofAddListener(inputs->newGUIEvent, this, &ofApp::uiEvent);
    inputs->loadSettings("inputs.xml");
    
    for (int i=0; i<10; i++) {
        distance[i] = 80000;
        value[i] = 0;
        calibration[0] = 0;
    }

    settings = new ofxUISuperCanvas("SETTINGS", 240., 20., 200., 200.);
    size = settings->addMinimalSlider("size", 1., 60., 60.);
    fade = settings->addMinimalSlider("fade", 0., 255., 16.);
    falloff = settings->addMinimalSlider("falloff", 0., 0.99, 0.8);
    spectrum.loadImage("spectrum.png");
    sampler = settings->addImageSampler("color", &spectrum, 200., 100.);
    saturation = settings->addMinimalSlider("saturation", 0.2, 1., 0.5);
    brightness = settings->addMinimalSlider("brightness", 0., 0.6, 0.5);
    rainbow = settings->addToggle("rainbow", true);
    rbspeed = settings->addMinimalSlider("rbspeed", 0.0001, 0.01, 0.03);
    spiral = settings->addToggle("spiral", true);
    sspeed = settings->addMinimalSlider("sspeed", 0.01, 0.8, 0.03);
    baseline = settings->addToggle("baseline", false);
    bonus = settings->addToggle("bonus", false);
    settings->autoSizeToFitWidgets();
    ofAddListener(settings->newGUIEvent, this, &ofApp::uiEvent);
    settings->loadSettings("settings.xml");
    
    server = new ofxUISuperCanvas("SERVER", 480., 20., 200., 200.);
    server->addTextInput("ip", "192.168.1.1");
    server->addNumberDialer("port", 1., 0xFFFF, 9999., 0.);
    server->addLabelToggle("connect", false);
    server->addLabel("status")->setFont(server->getFontSmall());
    vector<string> dir;
    dir.push_back("send");
    dir.push_back("receive");
    server->addRadio("direction", dir);
    server->autoSizeToFitWidgets();
    server->loadSettings("server.xml");
    
    ofAddListener(server->newGUIEvent, this, &ofApp::uiEvent);
    
    fbo.allocate(60, 40, GL_RGB);
    fbo.begin();
    ofClear(0);
    fbo.end();
    pixels.allocate(60, 40, OF_PIXELS_RGB);
    
    midi.listPorts();
    midi.openPort(0);
    midi.addListener(this);
    midi.setVerbose(true);
}

//--------------------------------------------------------------
void ofApp::exit() {

    inputs->saveSettings("inputs.xml");
    settings->saveSettings("settings.xml");
    server->saveSettings("server.xml");
}

//--------------------------------------------------------------
void ofApp::update(){

    float down = falloff->getValue();
    float low = range->getValueLow();
    float high = range->getValueHigh();
    int size = this->size->getValue();
    int fade = this->fade->getValue();
    int filter = this->filter->getValue();
    unsigned long long now = ofGetElapsedTimeMillis();

    for (int i=0; i<10; i++)
        ((ofxUILabel*)inputs->getWidget("fps" + ofToString(i+1)))->setLabel(ofToString(now - time[i],2));
    
    int i = 0;
    while (osc.hasWaitingMessages()) {
        
        ofxOscMessage msg;
        osc.getNextMessage(&msg);
        
        int b = msg.getArgAsInt32(0) / 2;
        int d = msg.getArgAsInt32(1);
        
        if (abs(d - history[b]) < filter) {
            distance[b] = d;
            time[b] = now;
            history[b] = d;
        }
        else
            history[b] = history[b] * 0.2 + d * 0.8;
        
        ((ofxUISlider*)inputs->getWidget("distance" + ofToString(b+1)))->setValue(distance[b]);
        i++;
    }
    
    bool bonus = false;
    for (int i=0; i<10; i++) {
        
        float n = ofMap(distance[i] - calibration[i], low, high, 0, 1.);
        if (n < 0) bonus = true;
        n = ofClamp(n, 0, 1.);

        if (n < value[i])
            value[i] = n;
        else
            value[i] = value[i] * down + (1-down) * n;
    }
    
    if (bonus && this->bonus->getValue()) {
        for (int i=0; i<10; i++) {
            value[i] = 0;
        }
    }
    

    fbo.begin();
    ofEnableAlphaBlending();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(0, 0, 0, fade);
    ofRect(0, 0, fbo.getWidth(), fbo.getHeight());
    
    ofColor color = sampler->getColor();
    color.setSaturation(saturation->getValue()*255);
    color.setBrightness(brightness->getValue()*255);
    ofSetColor(color);
    
    if (spiral->getValue()) {
        int n = (int)(ofGetFrameNum() * sspeed->getValue()) % 4;
        for (int i=0; i<10; i++) {
            ofRect(value[i]*59, i*4+n, size, 1);
        }
    }
    else {
        int b = baseline->getValue() ? 59 : 60;
        for (int i=0; i<10; i++) {
            ofRect(value[i]*b, i*4, size, 4);
        }
    }
    
    fbo.readToPixels(pixels);
    fbo.end();
    
    // TODO: send to all
    for (int i=9; i>=0; i--) {
        artnet.sendDmxRaw(i*2+0, pixels.getPixels()+i*720, fbo.getWidth()*3*2);
        artnet.sendDmxRaw(i*2+1, pixels.getPixels()+i*720+360, fbo.getWidth()*3*2);
    }
    
    if (rainbow->getValue()) {
        ofPoint p = sampler->getValue();
        p.x += rbspeed->getValue();
        if (p.x > 1) p.x -= 1;
        sampler->setValue(p);
    }
    
    ((ofxUILabel*)server->getWidget("status"))->setLabel(client.isConnected() ? "status: connected" : "status: disconnected");
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofPushMatrix();
    ofRotateZ(-90);
    ofTranslate(-420, 20);
    fbo.getTextureReference().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    fbo.draw(0, 0, -60*4, 40*16);
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::threadedFunction() {


    while (isThreadRunning()) {

        if (client.isConnected()) {

            if (((ofxUIRadio*)server->getWidget("direction"))->getActiveName() == "send") {
                string msg;
                for (int i=0; i<10; i++) {
                    msg += ofToString(i) + ":" + ofToString(distance[i]) + "\n";
                }
                client.send(msg);
            }
            if (((ofxUIRadio*)server->getWidget("direction"))->getActiveName() == "receive") {
                
                string msg = client.receive();
                
            }            
            
        }
        else {
            
            string ip = ((ofxUITextArea*)server->getWidget("ip"))->getTextString();
            int port = ((ofxUINumberDialer*)server->getWidget("port"))->getValue();
            client.setup(ip, port, false);
        }
    }
    
    client.close();
}

//--------------------------------------------------------------
void ofApp::uiEvent(ofxUIEventArgs &args) {

    /*if (args.getName() == "calibrate" && args.getButton()->getValue()) {
        int min = INT32_MAX;
        int max = 0;
        for (int i=0; i<10; i++) {
            if (distance[i] < min)
                min = distance[i];
            if (distance[i] > max)
                max = distance[i];
        }
        for (int i=0; i<10; i++) {
            calibration[i] = distance[i] - min;
        }
    }*/
    if (args.getName() == "poll") {
        artnet.sendPoll();
    }
    if (args.getName() == "connect") {

        if (args.getToggle()->getValue())
            this->startThread();
        else
            this->stopThread();
    }
}

void ofApp::artnetPollReply(ofxArtNetNodeEntry *&node) {

    ofLogNotice() << node->getIp();
}

void ofApp::newMidiMessage(ofxMidiMessage& msg) {
    ofLogNotice() << msg.toString();
    
    if (msg.status == MIDI_CONTROL_CHANGE) {
        switch (msg.control) {
            case 1:
                range->setValueLow(20000.*msg.value/127.);break;
            case 2:
                range->setValueHigh(20000.*msg.value/127.);break;
            case 3:
                size->setValue(1+59*msg.value/127.);break;
            case 4:
                fade->setValue(1+msg.value*2);break;
            case 5:
                falloff->setValue(ofLerp(falloff->getMin(), falloff->getMax(), msg.value/127.));break;
            case 6:
                sampler->setValue(ofPoint(msg.value/127.,0.));break;
            case 7:
                saturation->setValue(msg.value/127.);break;
            case 8:
                brightness->setValue(msg.value/127.);break;
            case 9:
                rbspeed->setValue(ofLerp(rbspeed->getMin(), rbspeed->getMax(), msg.value/127.));break;
            case 10:
                sspeed->setValue(ofLerp(sspeed->getMin(), sspeed->getMax(), msg.value/127.));break;
        }
    }
    if (msg.status == MIDI_NOTE_ON) {
        switch (msg.pitch) {
            case 24:
                rainbow->toggleValue();break;
            case 25:
                spiral->toggleValue();break;
            case 26:
                baseline->toggleValue();break;
            case 27:
                bonus->toggleValue();break;
        }
    }
}