#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "cinder/Rect.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/FftProcessor.h"
#include "cinder/audio/PcmBuffer.h"
#include "cinder/audio/Callback.h"
#include "cinder/CinderMath.h"
#include "cinder/Timer.h"
#include "cinder/params/Params.h"

#include "KissFFT.h"
#include "CinderConfig.h"
#include "OscListener.h"

#include "Resources.h"
#include "ParticleSystem.h"
#include "BpmTapper.h"

#define GUI_WIDTH   320


using namespace ci;
using namespace ci::app;
using namespace std;


class ClimaxApp : public AppNative {
    
private:
    
#ifndef CINDER_COCOA_TOUCH
    params::InterfaceGl mParams;
    config::Config      * mConfig;
#endif
    
    Vec2f       mForceCenter;
    Vec2f       mAttractionCenter;
    Color       mParticleColor;
    Perlin      mPerlin;
    osc::Listener 	listener;
    
	// Analyzer
	KissRef mFft;
    
    // Audio generation settings
	float	mAmplitude;
	float	mFreqTarget;
	float	mPhase;
	float	mPhaseAdjust;
	float	mMaxFreq;
	float	mMinFreq;
    
    float   mPerlinFrequency;
    float   mMinimumBeatForce;
    float   mParticleRadiusMin, mParticleRadiusMax;
    float   mParticlesPullFactor;
    float   mAttrFactor;
    float   mRepulsionFactor;
    float   mRepulsionRadius;
    
    float   mTargetSeparation;
    float   mNeighboringDistance;
    float   mSeparationFactor;
    float   mAlignmentFactor;
    float   mCohesionFactor;
    float   mForceCenterAnimRadius;
    float   mBpm;
    
    int     mMaxParticles;
    int     mNumParticlesOnBeat;
    int     mNumParticles;
    int     mNumSprings;
    
    bool    mParticlesPullToCenter;
    bool    mUseAttraction;
    bool    mUseRepulsion;
    bool    mUseFlocking;
    bool    mAutoRandomizeColor;

public:
    
    void prepareSettings( Settings * settings );
	void setup();
    void update();
	void draw();
    
#ifdef CINDER_COCOA_TOUCH
    void touchesBegan( TouchEvent event );
	void touchesMoved( TouchEvent event );
	void touchesEnded( TouchEvent event );
#else
    void mouseDown( MouseEvent event );
	void mouseMove( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void mouseUp( MouseEvent event );
    void keyDown( KeyEvent event );
    void resize();
#endif
	
    void addNewParticleAtPosition( const Vec2f & position );
    void addNewParticlesOnPolyLine( const PolyLine<Vec2f> & line );
    void randomizeParticleProperties();
    void setHighSeperation();
    void setHighNeighboring();
    void randomizeFlockingProperties();
#ifndef CINDER_COCOA_TOUCH
    void saveConfig();
    void loadConfig();
#endif
    void toggleSoundPlaying();
    void shutdown();
    
    void sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, ci::audio::Buffer32f * ioBuffer );
    
    ci::audio::SourceRef		mAudioSource;
	ci::audio::PcmBuffer32fRef	mBuffer;
	ci::audio::TrackRef			mTrack;
    
    ParticleSystem  mParticleSystem;
    BpmTapper       * bpmTapper;
    
    string          mConfigFileName;
    
    float           mBeatForce;
    float           mBeatSensitivity;
    float           mAverageLevelOld;
    float           mRandAngle;
    
    PolyLine<Vec2f> mFreqLine;
    vector<float>   mOutput;
};

void ClimaxApp::prepareSettings( Settings * settings )
{
#ifdef CINDER_COCOA_TOUCH
    settings->enableMultiTouch( true );
    settings->enableHighDensityDisplay();
#else
    settings->setWindowSize( 1280, 720 );
    settings->setFullScreen( true );
    settings->setResizable( false );
    settings->setBorderless( false );
    settings->setTitle( "Climax" );
#endif
}

void ClimaxApp::setup()
{
    gl::enableAlphaBlending();
    gl::clear( Color::black() );
    
    listener.setup( 9001 );
    
    // Audio Setup
    mMinimumBeatForce = 2.f;
    mBeatForce = 150.f;
    mBeatSensitivity = .03f;
    mAverageLevelOld = 0.f;
    mRandAngle = 15.f;
    
    // Set up line rendering
    gl::enable( GL_LINE_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    gl::color( ColorAf::white() );
    
    // Set synth properties
	mAmplitude = 0.5f;
	mMaxFreq = 20000.0f;
	mMinFreq = 1.0f;
	mFreqTarget = 0.0f;
	mPhase = 0.0f;
	mPhaseAdjust = 0.0f;
    
//    mAudioSource = audio::createCallback( this, & ClimaxApp::sineWave );
    mAudioSource = audio::load( loadResource( RES_SAMPLE ) );
	mTrack = audio::Output::addTrack( mAudioSource, false );
	mTrack->enablePcmBuffering( true );
    mTrack->setLooping( true );
	mTrack->play();
    mTrack->stop();
    
    
    mPerlinFrequency = .01f;
    mPerlin = Perlin();
    
    mMaxParticles = 1200;
    mNumParticlesOnBeat = 1;
    mParticleColor = Color::white();
    mParticleRadiusMin = .8f;
    mParticleRadiusMax = 1.6f;
    mParticlesPullToCenter = true;
    mParticlesPullFactor = 0.01f;
    mUseAttraction = false;
    mAttrFactor = .05f;
    
    mUseRepulsion = false;
    mRepulsionFactor = -10.f;
    mRepulsionRadius = 35.f;
    
    mAutoRandomizeColor = true;
    mBpm = 192.f;
    bpmTapper = new BpmTapper();
    bpmTapper->isEnabled = true;
    bpmTapper->start();
    
    mUseFlocking = true;
    mTargetSeparation = 20.f;
    mNeighboringDistance = 50.f;
    mSeparationFactor = 1.2f;
    mAlignmentFactor = .9f;
    mCohesionFactor = .4f;
    
    mForceCenter = getWindowCenter();
    mAttractionCenter = getWindowCenter();
    mForceCenterAnimRadius = 2.f;
    
#ifndef CINDER_COCOA_TOUCH
    mConfigFileName = "config.xml";
    mParams = params::InterfaceGl( getWindow(), "Settings", toPixels( Vec2i( GUI_WIDTH, getWindowHeight() - 40.f ) ) );
    mConfig = new config::Config( & mParams );
    
    mParams.addText( "Particles", "label=`Particles`" );
    
    mParams.addParam( "Particle Count", & mNumParticles, "", true );
    mParams.addParam( "Spring Count", & mNumSprings, "", true );
    
    mConfig->addParam( "Max Particles", & mMaxParticles , "" );
    mConfig->addParam( "Particles on Beat", & mNumParticlesOnBeat , "" );
    
    mConfig->addParam( "Particle Color" , & mParticleColor );
    mConfig->addParam( "Target Separation", & mTargetSeparation, "min=0.1f max=100.f" );
    mConfig->addParam( "Neighboring Distance", & mNeighboringDistance, "min=0.1f max=100.f" );
    mParams.addButton( "Randomize Particle Color" , std::bind( & ClimaxApp::randomizeParticleProperties, this ), "key=1" );
    mConfig->addParam( "Auto-Randomize Particle Color" , & mAutoRandomizeColor, "key=1" );
    mParams.addButton( "High Seperation" , std::bind( & ClimaxApp::setHighSeperation, this ), "key=2" );
    mParams.addButton( "High Neighboring" , std::bind( & ClimaxApp::setHighNeighboring, this ), "key=3" );
    
    mConfig->addParam( "Flocking Enabled", & mUseFlocking, "key=f" );
    mConfig->addParam( "Separation Factor", & mSeparationFactor, "min=-5.f max=5.f step=0.01" );
    mConfig->addParam( "Alignment Factor", & mAlignmentFactor, "min=-5.f max=5.f" );
    mConfig->addParam( "Cohesion Factor", & mCohesionFactor, "min=-50.f max=50.f" );
    mParams.addButton( "Randomize Flocking Parameters" , std::bind( & ClimaxApp::randomizeFlockingProperties, this ), "key=4" );
    mParams.addSeparator();
    
    mParams.addText( "Forces", "label=`Forces`" );
    mConfig->addParam( "Pull Particles to Center", & mParticlesPullToCenter , "key=5" );
    mConfig->addParam( "Pull Factor", & mParticlesPullFactor, "min=0.f max=1.f step=.0001f" );
    mParams.addSeparator();
    mConfig->addParam( "Attraction Enabled", & mUseAttraction, "key=a" );
    mConfig->addParam( "Attraction Factor", & mAttrFactor, "min=0.f max=10.f step=.001f" );
    mParams.addSeparator();
    mConfig->addParam( "Repulsion Enabled", & mUseRepulsion, "key=6" );
    mConfig->addParam( "Repulsion Factor", & mRepulsionFactor, "min=-10.f max=10.f step=.001f" );
    mConfig->addParam( "Repulsion Radius", & mRepulsionRadius, "min=0.f max=800.f" );
    mConfig->addParam( "Force Area Radius", & mForceCenterAnimRadius, "min=0.1f max=800.f" );
    mParams.addSeparator();
    
    mParams.addText( "Audio", "label=`Audio`" );
    mConfig->addParam( "BPM Tempo" , & mBpm, "min=100 max=255" );
    mParams.addButton( "Play / Pause" , std::bind( & ClimaxApp::toggleSoundPlaying, this ) );
    mConfig->addParam( "Minimum Beat Force", & mMinimumBeatForce, "min=0.1f max=20.f" );
    mConfig->addParam( "Beat Force", & mBeatForce, "min=0.f max=1000.f" );
    mConfig->addParam( "Beat Sensitivity", & mBeatSensitivity, "min=0.f max=1.f" );
    mParams.addSeparator();
    
    mParams.addText( "Settings", "label=`Settings`" );
    mParams.addButton( "Save Settings", bind( & ClimaxApp::saveConfig, this ) );
    mParams.addButton( "Reload Settings", bind( & ClimaxApp::loadConfig, this ), "key=L" );
    
    // Try to restore last saved parameters configuration
    try {
        loadConfig();
    } catch ( Exception e ) {
        console() << e.what() << std::endl;
    }
#endif
}

void ClimaxApp::update()
{
    mNumParticles = mParticleSystem.particles.size();
    mNumSprings = mParticleSystem.springs.size();
    
    while( listener.hasWaitingMessages() ) {
		osc::Message message;
		listener.getNextMessage( &message );
		
		console() << "New message received" << std::endl;
		console() << "Address: " << message.getAddress() << std::endl;
		console() << "Num Arg: " << message.getNumArgs() << std::endl;
		
//        for (int i = 0; i < message.getNumArgs(); i++) {
//			console() << "-- Argument " << i << std::endl;
//			console() << "---- type: " << message.getArgTypeName(i) << std::endl;
//			if( message.getArgType(i) == osc::TYPE_INT32 ) {
//				try {
//					console() << "------ value: "<< message.getArgAsInt32(i) << std::endl;
//				}
//				catch (...) {
//					console() << "Exception reading argument as int32" << std::endl;
//				}
//			}
//			else if( message.getArgType(i) == osc::TYPE_FLOAT ) {
//				try {
//					console() << "------ value: " << message.getArgAsFloat(i) << std::endl;
//				}
//				catch (...) {
//					console() << "Exception reading argument as float" << std::endl;
//				}
//			}
//			else if( message.getArgType(i) == osc::TYPE_STRING) {
//				try {
//					console() << "------ value: " << message.getArgAsString(i).c_str() << std::endl;
//				}
//				catch (...) {
//					console() << "Exception reading argument as string" << std::endl;
//				}
//			}
//		}
        
        Vec2f pos = Vec2f::zero() + getWindowCenter();
        if( message.getNumArgs() != 0 && message.getArgType( 0 ) == osc::TYPE_FLOAT )
            pos.x = message.getArgAsFloat(0);
	}
    
    Vec2f center = getWindowCenter();
    
    bpmTapper->setBpm( mBpm );
    bpmTapper->update();
    
    if ( mAutoRandomizeColor && bpmTapper->onBeat() ){
        bpmTapper->start();
        randomizeParticleProperties();
    }
    
    for ( auto it : mParticleSystem.particles ){
        
        it->separationEnabled = mUseFlocking;
        it->separationFactor = mSeparationFactor;
        it->alignmentEnabled = mUseFlocking;
        it->alignmentFactor = mAlignmentFactor;
        it->cohesionEnabled = mUseFlocking;
        it->cohesionFactor = mCohesionFactor;
        
        if ( mParticlesPullToCenter ){
            Vec2f force = ( center - it->position ) * .01f;
            it->forces += force;
        }
        
        if ( mUseAttraction ){
            Vec2f attrForce = mAttractionCenter - it->position;
            attrForce.normalize();
            attrForce *= math<float>::max( 0.f, mAttrFactor - attrForce.length() );
            it->forces += attrForce;
        }
        
        if ( mUseRepulsion && it->position.distance( mAttractionCenter ) > mRepulsionRadius ){
            Vec2f repForce = it->position - mAttractionCenter;
            repForce = repForce.normalized() * math<float>::max( 0.f, mRepulsionRadius - repForce.length() );
            it->forces += repForce;
        }
        // Change frequency and amplitude based on mouse position
        // Scale everything logarithmically to get a better feel and sound
        mAmplitude = 1.0f - it->position.normalized().length();
        double width = (double)getWindowWidth();
        double x = width - (double)it->position.y;
        float mPosition = (float)( ( log( width ) - log( x ) ) / log( width ) );
        mFreqTarget = math<float>::clamp( mMaxFreq * mPosition, mMinFreq, mMaxFreq );
        mAmplitude = math<float>::clamp( mAmplitude * ( 1.0f - mPosition ), 0.05f, 1.0f );
    }
    mParticleSystem.maxParticles = mMaxParticles;
    mParticleSystem.update();
    
    mForceCenter = getWindowCenter();
}

void ClimaxApp::addNewParticleAtPosition( const Vec2f & position )
{
    float radius = ci::randFloat( mParticleRadiusMin, mParticleRadiusMax );
    float mass = radius * radius;
    float drag = .95f;
    
    Particle * particle = new Particle( position, radius, mass, drag, mTargetSeparation, mNeighboringDistance, mParticleColor );
    mParticleSystem.addParticle( particle );
}

void ClimaxApp::addNewParticlesOnPolyLine( const PolyLine<Vec2f> & line )
{
    if ( line.size() == 0 ) return;
    if ( bpmTapper->onBeat() )
        for ( auto it : line )
            addNewParticleAtPosition( it );
}

void ClimaxApp::setHighSeperation()
{
    mTargetSeparation = randFloat( 50.f, 100.f );
    mNeighboringDistance = randFloat( 10.f, 50.f );
}

void ClimaxApp::setHighNeighboring()
{
    mTargetSeparation = randFloat( 10.f, 50.f );
    mNeighboringDistance = randFloat( 50.f, 100.f );
}

void ClimaxApp::randomizeParticleProperties()
{
    mParticleColor = Color( randFloat(), randFloat(), randFloat() );
    mParticleRadiusMin = .8f;
    mParticleRadiusMax = 1.6f;
}

void ClimaxApp::randomizeFlockingProperties()
{
    mSeparationFactor = randFloat();
    mAlignmentFactor = randFloat();
    mCohesionFactor = randFloat();
}

void ClimaxApp::toggleSoundPlaying()
{
    if ( mTrack->isPlaying() ){
        mTrack->stop();
        bpmTapper->isEnabled = false;
        bpmTapper->stop();
    } else {
        mTrack->play();
        bpmTapper->isEnabled = true;
        bpmTapper->start();
    }
}

#ifdef CINDER_COCOA_TOUCH
void ClimaxApp::touchesBegan( TouchEvent event )
{
    randomizeParticleProperties();
	for ( auto touch : event.getTouches() ) {
        addNewParticleAtPosition( touch.getPos() );
	}
}

void ClimaxApp::touchesMoved( TouchEvent event )
{
//	for( vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt )
//		mActivePoints[touchIt->getId()].addPoint( touchIt->getPos() );
}

void ClimaxApp::touchesEnded( TouchEvent event )
{
//	for( vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt ) {
//		mActivePoints[touchIt->getId()].startDying();
//		mDyingPoints.push_back( mActivePoints[touchIt->getId()] );
//		mActivePoints.erase( touchIt->getId() );
//	}
}

#else

void ClimaxApp::mouseDown( MouseEvent event )
{
    randomizeParticleProperties();
}

void ClimaxApp::mouseMove( MouseEvent event )
{
    mAttractionCenter = event.getPos();
}

void ClimaxApp::mouseUp( MouseEvent event )
{
}

void ClimaxApp::mouseDrag( MouseEvent event )
{
    addNewParticleAtPosition( event.getPos() );
}

void ClimaxApp::resize()
{
    mForceCenter = getWindowCenter();
    mAttractionCenter = getWindowCenter();
}

void ClimaxApp::keyDown( KeyEvent event )
{
//    if ( event.getChar() == 's' && event.isMetaDown() ) {
//        saveConfig();
//        return;
//    }
//    if ( event.getChar() == 'l' && event.isMetaDown() ) {
//        loadConfig();
//        return;
//    }
    
    if ( event.getChar() == ' ' )
    {
        mParticleSystem.clear();
    }
    if ( event.getChar() == 's' ) {
        if ( mParams.isVisible() ) {
            mParams.hide();
        } else
            mParams.show();
    }
    
    if ( event.getChar() == 'p' )
        addNewParticlesOnPolyLine( mFreqLine );
}

#endif

void ClimaxApp::draw()
{
	gl::clear();
    gl::enableAlphaBlending();
    gl::setViewport( getWindowBounds() );
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    
    gl::color( Color::white() );
    
    mParticleSystem.draw();
    
#ifndef CINDER_COCOA_TOUCH
    if ( mParams.isVisible() ) mParams.draw();
#endif
}

#ifndef CINDER_COCOA_TOUCH
void ClimaxApp::saveConfig()
{
    mConfig->save( getAppPath() / fs::path( mConfigFileName ) );
    console() << "Saved configuration to: " << getAppPath() / fs::path( mConfigFileName ) << std::endl;
}

void ClimaxApp::loadConfig()
{
    mConfig->load( getAppPath() / fs::path( mConfigFileName ) );
    console() << "Loaded configuration from: " << getAppPath() / fs::path( mConfigFileName ) << std::endl;
}
#endif

void ClimaxApp::sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, ci::audio::Buffer32f * ioBuffer )
{
    // Fill buffer with sine wave
	mPhaseAdjust = mPhaseAdjust * 0.95f + ( mFreqTarget / 44100.0f ) * 0.05f;
    
    mAmplitude = 1.f;
    for ( auto particle_it : mParticleSystem.particles ) {
        mAmplitude -= particle_it->color.get( CM_RGB ).normalized().x;
    }
    
	for ( uint32_t i = 0; i < ioSampleCount; i++ ){
        
		mPhase += mPhaseAdjust;
		mPhase = mPhase - math<float>::floor( mPhase );
        
        float val = math<float>::sin( mPhase * 2.0f * (float)M_PI ) * mAmplitude;
        
        ioBuffer->mData[ i * ioBuffer->mNumberChannels ] = val;
		ioBuffer->mData[ i * ioBuffer->mNumberChannels + 1 ] = val;
	}
    
	// Initialize analyzer
	if ( !mFft ) {
		mFft = Kiss::create( ioSampleCount );
	}
    
	// Analyze data
	mFft->setData( ioBuffer->mData );
}

void ClimaxApp::shutdown()
{
    mTrack->stop();
    mTrack.reset();
    if ( mFft ) {
		mFft->stop();
	}
    mAudioSource.reset();
}

CINDER_APP_NATIVE( ClimaxApp, RendererGl )
