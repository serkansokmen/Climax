#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "cinder/Rect.h"
#include "cinder/params/Params.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/FftProcessor.h"
#include "cinder/audio/PcmBuffer.h"
#include "cinder/audio/Callback.h"
#include "cinder/CinderMath.h"
#include "cinder/Timer.h"

#include "KissFFT.h"
#include "CinderConfig.h"

#include "Resources.h"
#include "ParticleSystem.h"
#include "BpmTapper.h"


#define GUI_WIDTH   320


using namespace ci;
using namespace ci::app;
using namespace std;


class ClimaxApp : public AppNative {
    
private:
    
    params::InterfaceGl mParams;
    config::Config      * mConfig;
    
    Vec2f       mForceCenter;
    Vec2f       mAttractionCenter;
    Color       mParticleColor;
    Perlin      mPerlin;
    
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
    bool    mDrawForceCenter;
    bool    mAutoRandomizeColor;

public:
    
    void prepareSettings( Settings * settings );
	void setup();
    void mouseDown( MouseEvent event );
	void mouseMove( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void mouseUp( MouseEvent event );
    void keyDown( KeyEvent event );
    void resize();
	void update();
	void draw();
    void addNewParticleAtPosition( const Vec2f & position );
    void randomizeParticleProperties();
    void setHighSeperation();
    void setHighNeighboring();
    void randomizeFlockingProperties();
    void saveConfig();
    void loadConfig();
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
    settings->setWindowSize( 1280, 720 );
    settings->enableMultiTouch( false );
    settings->setFullScreen( true );
    settings->enableHighDensityDisplay();
    settings->setResizable( false );
    settings->setBorderless( false );
    settings->setTitle( "Climax" );
}

void ClimaxApp::setup()
{
    gl::enableAlphaBlending();
    gl::clear( Color::black() );
    
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
    
//	mAudioSource = audio::load( getAssetPath( "vordhosbn.mp3" ).c_str() );
    mAudioSource = audio::createCallback( this, & ClimaxApp::sineWave );
    // Play sine
//	audio::Output::play( mAudioSource );
    
	mTrack = audio::Output::addTrack( mAudioSource, false );
	mTrack->enablePcmBuffering( true );
    mTrack->setLooping( true );
	mTrack->play();
//    toggleSoundPlaying();
    
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
    bpmTapper->isEnabled = false;
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
    mDrawForceCenter = true;
    
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
    mConfig->addParam( "Auto-Randomize Tempo" , & mBpm, "min=100 max=255" );
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
    mConfig->addParam( "Show Force Center", & mDrawForceCenter, "" );
    mParams.addSeparator();
    
    mParams.addText( "Audio", "label=`Audio`" );
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
}

void ClimaxApp::update()
{
    mNumParticles = mParticleSystem.particles.size();
    mNumSprings = mParticleSystem.springs.size();
    
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
    }
    mParticleSystem.maxParticles = mMaxParticles;
    mParticleSystem.update();
    
    mForceCenter = getWindowCenter();
        
//    if ( mForceCenter.x > 0 && mForceCenter.x < getWindowWidth() && mForceCenter.y > 0 && mForceCenter.y < getWindowHeight() ) {
//        addNewParticleAtPosition( mForceCenter );
//    }
}

void ClimaxApp::addNewParticleAtPosition( const Vec2f & position )
{
    float radius = ci::randFloat( mParticleRadiusMin, mParticleRadiusMax );
    float mass = radius * radius;
    float drag = .95f;
    
    Particle * particle = new Particle( position, radius, mass, drag, mTargetSeparation, mNeighboringDistance, mParticleColor );
    mParticleSystem.addParticle( particle );
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
    if ( mTrack->isPlaying() )
        mTrack->stop();
    else
        mTrack->play();
}

void ClimaxApp::mouseDown( MouseEvent event )
{
    randomizeParticleProperties();
}

void ClimaxApp::mouseMove( MouseEvent event )
{
    mAttractionCenter = event.getPos();
    
    // Change frequency and amplitude based on mouse position
	// Scale everything logarithmically to get a better feel and sound
	mAmplitude = 1.0f - event.getY() / (float)getWindowHeight();
	double width = (double)getWindowWidth();
	double x = width - (double)event.getX();
	float mPosition = (float)( ( log( width ) - log( x ) ) / log( width ) );
	mFreqTarget = math<float>::clamp( mMaxFreq * mPosition, mMinFreq, mMaxFreq );
	mAmplitude = math<float>::clamp( mAmplitude * ( 1.0f - mPosition ), 0.05f, 1.0f );
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
    if ( event.getChar() == 's' && event.isMetaDown() ) {
        saveConfig();
        return;
    }
    if ( event.getChar() == 'l' && event.isMetaDown() ) {
        loadConfig();
        return;
    }
    
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
}

void ClimaxApp::draw()
{
	gl::clear();
    gl::enableAlphaBlending();
    gl::setViewport( getWindowBounds() );
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    
    gl::color( Color::white() );
    
    mParticleSystem.draw();
    
    if ( mDrawForceCenter ){
        gl::color( Color( 0.f, 1.f, 0.f ) );
        gl::drawSolidCircle( mForceCenter, mForceCenterAnimRadius * 10 );
        gl::color( Color( 1.f, 0.f, 1.f ) );
        gl::drawSolidCircle( mAttractionCenter , mAttrFactor * 10 );
    }
    
    // Check init flag
	if ( mFft ) {
        
		// Get data in the frequency (transformed) and time domains
		float * freqData = mFft->getAmplitude();
		float * timeData = mFft->getData();
		int32_t dataSize = mFft->getBinSize();
        
		// Cast data size to float
		float dataSizef = (float)dataSize;
        
		// Get dimensions
		float scale = ( (float)getWindowWidth() - 20.0f ) / dataSizef;
		float windowHeight = (float)getWindowHeight();
        
		// Use polylines to depict time and frequency domains
		PolyLine<Vec2f> freqLine;
		PolyLine<Vec2f> timeLine;
        
		// Iterate through data
		for ( int32_t i = 0; i < dataSize; i++ ) {
            
			// Do logarithmic plotting for frequency domain
			float logSize = math<float>::log( dataSizef );
			float x = (float)( (math<float>::log( (float)i) / logSize ) * dataSizef );
			float y = math<float>::clamp( freqData[i] * ( x / dataSizef ) * ( math<float>::log( ( dataSizef - (float)i ) ) ), 0.0f, 2.0f );
            
			// Plot points on lines for tme domain
			freqLine.push_back( Vec2f(        x * scale + 10.0f,          -y   * ( windowHeight - 20.0f ) * 0.25f + ( windowHeight - 10.0f ) ) );
			timeLine.push_back( Vec2f( (float)i * scale + 10.0f, timeData[ i ] * ( windowHeight - 20.0f ) * 0.25f + ( windowHeight * 0.25f + 10.0f ) ) );
            
		}
        
		// Draw signals
		gl::draw( freqLine );
		gl::draw( timeLine );
        
	}
    
    if ( mParams.isVisible() ) mParams.draw();
}

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

void ClimaxApp::sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, ci::audio::Buffer32f * ioBuffer )
{
    // Fill buffer with sine wave
	mPhaseAdjust = mPhaseAdjust * 0.95f + ( mFreqTarget / 44100.0f ) * 0.05f;
	for ( uint32_t i = 0; i < ioSampleCount; i++ ) {
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
    if ( mFft ) {
		mFft->stop();
	}
}

CINDER_APP_NATIVE( ClimaxApp, RendererGl )
