//
//  BpmTapper.h
//  Climax
//
//  Created by Serkan Sökmen on 30.10.2013.
//
//

#pragma once

#include "cinder/Timer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BpmTapper {
    
    Timer   mTimer;
    float   bpm;
    bool    isOnBeat;
    
public:
    
    BpmTapper(){
        this->isOnBeat = false;
        this->isEnabled = false;
    }
    
    void start(){
        if ( ! isEnabled ) return;
        mTimer.start();
        this->isOnBeat = false;
    }
    
    void stop(){
        if ( ! isEnabled ) return;
        mTimer.stop();
        this->isOnBeat = false;
    }
    
    void update(){
        if ( ! isEnabled ) return;
        this->isOnBeat = false;
        if ( mTimer.isStopped() )
            this->mTimer.start();
        else {
            float milisPerBeat = 60000 / bpm;
//            float beatsPerMilis = mBpm / 60000;
            
            if ( mTimer.getSeconds() > milisPerBeat / 1000 ){
                this->mTimer.stop();
                this->isOnBeat = true;
            }
        }
    }
    
    void setBpm( const float bpm ){
        this->bpm = bpm;
    }
    
    bool onBeat(){
        return this->isOnBeat;
    };
    
    bool isEnabled;
};
