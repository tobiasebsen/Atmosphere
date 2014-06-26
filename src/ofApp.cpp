#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofBackground(40);
    ofSetFrameRate(30);

    osc.setup(9999);
    
    vector<ofxArtNetInterface> interfaces;
    ofxArtNetInterface::getInterfaces(interfaces);
    artnet.init(interfaces[interfaces.size()-1].ip);
    artnet.setNodeType(ARTNET_TYPE_RAW);
    artnet.start();
    
    inputs = new ofxUISuperCanvas("INPUTS", 20., 20., 200., 200.);
    for (int i=0; i<10; i++)
        inputs->addMinimalSlider("distance" + ofToString(i+1), 0., 10000., 0.);
    inputs->autoSizeToFitWidgets();
    
    settings = new ofxUISuperCanvas("SETTINGS", 240., 20., 200., 200.);
    range = settings->addRangeSlider("range", 0., 10000., 800., 5000.);
    size = settings->addMinimalSlider("size", 1., 60., 60.);
    fade = settings->addMinimalSlider("fade", 0., 255., 16.);
    falloff = settings->addMinimalSlider("falloff", 0., 0.5, 0.1);
    spectrum.loadImage("spectrum.png");
    sampler = settings->addImageSampler("color", &spectrum, 200., 100.);
    settings->autoSizeToFitWidgets();
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
}

//--------------------------------------------------------------
void ofApp::exit() {

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

    distance[0] = (high + 20) * down + distance[0] * (1-down);

    if (osc.hasWaitingMessages()) {
        
        ofxOscMessage msg;
        osc.getNextMessage(&msg);
        
        int d = msg.getArgAsInt32(0);
        if (d < distance[0])
            distance[0] = d;
        
        ((ofxUISlider*)inputs->getWidget("distance1"))->setValue(distance[0]);
    }
    

    fbo.begin();
    ofEnableAlphaBlending();
    ofSetColor(0, 0, 0, fade);
    ofRect(0, 0, fbo.getWidth(), fbo.getHeight());
    
    ofSetColor(sampler->getColor());
    
    for (int i=0; i<1; i++) {

        int n = ofMap(distance[i], low, high, 0, 60);
        n = ofClamp(n, 0, 60);
        ofRect(n, i*4, size, 4);
    }
    
    fbo.readToPixels(pixels);
    fbo.end();
    
    // TODO: send to all
    artnet.sendDmxRaw(0, pixels.getPixels(), fbo.getWidth()*3*2);
    artnet.sendDmxRaw(1, pixels.getPixels()+360, fbo.getWidth()*3*2);
    
    ((ofxUILabel*)server->getWidget("status"))->setLabel(client.isConnected() ? "status: connected" : "status: disconnected");
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofPushMatrix();
    ofRotateZ(-90);
    ofTranslate(-320, 20);
    fbo.getTextureReference().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    fbo.draw(0, 0, -60*4, 40*16);
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::threadedFunction() {


    while (isThreadRunning()) {

        if (client.isConnected()) {

            string msg;
            for (int i=0; i<10; i++) {
                msg += ofToString(i) + ":" + ofToString(distance[i]) + "\n";
            }
            client.send(msg);
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

    if (args.getName() == "connect") {

        if (args.getToggle()->getValue())
            this->startThread();
        else
            this->stopThread();
    }
}
