#pragma once

#include "maximilian.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Callback.h"
#include "cinder/Rand.h"

using namespace std;

extern int channels;
extern int samplerate;
extern int buffersize;

class MaxiPlayer{
public:
	
	MaxiPlayer(){
		ci::audio::Output::play( ci::audio::createCallback( this, &MaxiPlayer::getData, false) );
		channels = 2;
		samplerate = 44100;
		buffersize = 512;
		
		envelopeData = new double[4];
		envelopeData[0] = 1;
		envelopeData[1] = 0;
		envelopeData[2] = 0;
		envelopeData[3] = 1000;
		
		envelope.amplitude = envelopeData[0];
		envelope.valindex = 0;
	}
	
	void trigger(){
		envelope.trigger(0, envelopeData[0]);
	}
	
	~MaxiPlayer(){
		delete[] envelopeData;
	}
	
protected:
	maxiFilter filter;
	maxiOsc osc;
	maxiOsc phasor;
	maxiEnvelope envelope;
	maxiDelayline delay;
	double *envelopeData;
	int counter;
	double envAmplitude;
	
	
	void getData( uint64_t inSampleOffset, uint32_t inSampleCount, ci::audio::Buffer32f *ioBuffer ){
		int numChannels = ioBuffer->mNumberChannels;
		float *_buffer = ioBuffer->mData;
		double sample = 0.0;
		double *samplePtr = &sample;
		for(int i=0; i<inSampleCount; ++i){
			play( samplePtr );
			for(int j=0; j<numChannels; ++j){
				*_buffer++ = sample;
			}
		}
	}
	
	void play(double* output){
		counter = (int)phasor.phasor(0.5, 1, 9);
		envAmplitude = envelope.line(4, envelopeData);
		/*if(counter == 1){
			envelope.trigger(0, envelopeData[0]);
		}*/
		*output =  filter.lores( osc.sinewave( counter * 100 + 100 ), 400, 100) * envAmplitude;
	}
};