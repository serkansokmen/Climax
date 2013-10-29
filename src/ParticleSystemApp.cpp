#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "cinder/Rect.h"
#include "cinder/params/Params.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/FftProcessor.h"
#include "cinder/audio/PcmBuffer.h"
#include "cinder/CinderMath.h"

#include "CinderConfig.h"

#include "Resources.h"
#include "ParticleSystem.h"


#define GUI_WIDTH   320

using namespace ci;
using namespace ci::app;
using namespace std;


class ClimaxApp : public AppNative {

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
    
    string              configFilename;
    params::InterfaceGl mParams;
    config::Config*     mConfig;
    
    audio::TrackRef         mTrack;
    audio::PcmBuffer32fRef  mPcmBuffer;
    float                   mBeatForce;
    float                   mBeatSensitivity;
    float                   mAverageLevelOld;
    float                   mRandAngle;
    
    ParticleSystem  mParticleSystem;
    
    Vec2f       mForceCenter;
    Vec2f       mAttractionCenter;
    Color       mParticleColor;
    Perlin      mPerlin;
    
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
    
    vector<float>       mOutput;
};

void ClimaxApp::prepareSettings( Settings * settings )
{
    settings->setWindowSize( 1280, 720 );
    settings->enableMultiTouch( false );
    settings->setFullScreen( true );
    settings->enableHighDensityDisplay();
    settings->setResizable( true );
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
    
    mTrack = audio::Output::addTrack( audio::load( getAssetPath( "vordhosbn.mp3" ).c_str() ) );
    mTrack->enablePcmBuffering( true );
    mTrack->setLooping( true );
    toggleSoundPlaying();
    
    mPerlinFrequency = .01f;
    mPerlin = Perlin();
    
    mMaxParticles = 1200;
    mNumParticlesOnBeat = 1;
    mParticleColor = Color::white();
    mParticleRadiusMin = .2f;
    mParticleRadiusMax = 1.6f;
    mParticlesPullToCenter = true;
    mParticlesPullFactor = 0.01f;
    mUseAttraction = false;
    mAttrFactor = .05f;
    
    mUseRepulsion = false;
    mRepulsionFactor = -10.f;
    mRepulsionRadius = 35.f;
    
    mAutoRandomizeColor = true;
    
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
    
    configFilename = "config.xml";
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
    
    float beatValue = 0.f;
    mPcmBuffer = mTrack->getPcmBuffer();
    
    if ( mPcmBuffer ){
        
        int bandCount = 32;
        std::shared_ptr<float> fftRef = audio::calculateFft( mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_LEFT ), bandCount );
        
        if( fftRef ) {
            float * fftBuffer = fftRef.get();
            float avgLvl= 0.f;
            for( int i= 0; i<bandCount; i++ ) {
                avgLvl += fftBuffer[i] / (float)bandCount;
            }
            avgLvl /= (float)bandCount;
            if( avgLvl > mAverageLevelOld + mBeatSensitivity) {
                beatValue = avgLvl - mBeatSensitivity;
            }
            mAverageLevelOld = avgLvl;
        }
    }
    
    if ( mAutoRandomizeColor && getElapsedFrames() % 100 == 0 )
        randomizeParticleProperties();
    
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
    
    // Add New Particle on Beat
    float beatForce = beatValue * randFloat( mBeatForce * .5f, mBeatForce );
    Vec2f particlePos = getWindowCenter();
    
    particlePos.x = math<float>::sin( getElapsedSeconds() * 8 ) * getWindowWidth() + getWindowWidth() / 2;
    particlePos.y = math<float>::cos( getElapsedSeconds() * 4 ) * getWindowHeight() + getWindowHeight() / 2;
    mPerlinFrequency = beatValue;
    particlePos += mPerlin.fBm( mPerlinFrequency );
    mNumParticlesOnBeat = (int)( beatValue * 40.f );
    
    if ( beatForce > mMinimumBeatForce ){
        
        if ( beatForce > mMinimumBeatForce * 2.f ) {
            randomizeParticleProperties();
            randomizeFlockingProperties();
        }
        
        if ( mForceCenter.x > 0 && mForceCenter.x < getWindowWidth() && mForceCenter.y > 0 && mForceCenter.y < getWindowHeight() ) {
            float radius = ci::randFloat( mParticleRadiusMin, mParticleRadiusMax ) * beatForce * 40.f;
            float mass = radius * radius;
            float drag = .95f;
            
            for ( int i=0; i<mNumParticlesOnBeat; i++) {
                Vec2f pos = particlePos + randVec2f() * 10.f;
                Particle * particle = new Particle( pos, radius, mass, drag, mTargetSeparation, mNeighboringDistance, mParticleColor );
                
                for ( auto second : mParticleSystem.particles ){
                    float d = particle->position.distance( second->position );
                    float d2 = ( particle->radius + second->radius ) * 100.f;
                    
                    if ( d > 0.f && d <= d2 && d2 < 500.f ) {
                        Spring * spring = new Spring( particle, second, d * 1.2f, .001f );
                        mParticleSystem.addSpring( spring );
                    }
                }
                mParticleSystem.addParticle( particle );
            }
        }
    }
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
    mParticleRadiusMin = randFloat( .4f, .8f );
    mParticleRadiusMax = randFloat( .8f, 1.6f );
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
    
    if ( mParams.isVisible() ) mParams.draw();
}

void ClimaxApp::saveConfig()
{
    mConfig->save( getAppPath() / fs::path( configFilename ) );
    console() << "Saved configuration to: " << getAppPath() / fs::path( configFilename ) << std::endl;
}

void ClimaxApp::loadConfig()
{
    mConfig->load( getAppPath() / fs::path( configFilename ) );
    console() << "Loaded configuration from: " << getAppPath() / fs::path( configFilename ) << std::endl;
}

CINDER_APP_NATIVE( ClimaxApp, RendererGl )
