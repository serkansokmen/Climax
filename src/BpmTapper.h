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
    }
    
    void start(){
        mTimer.stop();
        mTimer.start();
        isOnBeat = false;
    }
    
    void update(){
        
        isOnBeat = false;
        if ( mTimer.isStopped() )
            mTimer.start();
        
        float milisPerBeat = 60000 / bpm;
        //    float beatsPerMilis = mBpm / 60000;
        
        if ( mTimer.getSeconds() > milisPerBeat / 1000 ){
            mTimer.stop();
            isOnBeat = true;
        }
        
        console() << isOnBeat << endl;
    }
    
    void setBpm( const float bpm ){
        this->bpm = bpm;
    }
    
    bool onBeat(){
        return this->isOnBeat;
    };
};
