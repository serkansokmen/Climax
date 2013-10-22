#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "cinder/Rect.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Fbo.h"
#include "cinder/params/Params.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Callback.h"
#include "cinder/CinderMath.h"

#include "CinderConfig.h"

#include "Resources.h"
#include "ParticleSystem.h"

#define GUI_WIDTH               280
#define PARTICLE_AREA_PADDING   20.f

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
    void randomizeFlockingProperties();
    void saveConfig();
    void loadConfig();
    
    void audioCallback( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *buffer );
    
    string              configFilename;
    params::InterfaceGl mParams;
    config::Config*     mConfig;
    
    ParticleSystem  mParticleSystem;
    gl::Texture     mParticleTexture;
    gl::Fbo         mParticlesFbo;
    
    Vec2f       mForceCenter;
    ci::Rectf   mParticlesBorder;
    
    Color   mParticleColor;
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
    
    float   mSoundFrequency;
    float   mPhase, mPhaseAdd;
    float   mModFrequency;
    float   mModPhase, mModPhaseAdd;
    
    int     mMaxParticles;
    
    bool    mParticlesPullToCenter;
    bool    mUseAttraction;
    bool    mUseRepulsion;
    bool    mUseFlocking;
    
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
    
    mParticleTexture = gl::Texture( loadImage( loadResource( RES_PARTICLE_IMAGE ) ) );
    gl::Fbo::Format format;
    format.enableMipmapping( true );
    format.setColorInternalFormat( GL_RGBA );
    format.setSamples( 4 );
    format.setCoverageSamples( 8 );
    mParticlesFbo = gl::Fbo( getWindowWidth(), getWindowHeight(), format );
    
    mParticlesFbo.bindFramebuffer();
    gl::clear( Color::black() );
    mParticlesFbo.unbindFramebuffer();
    
    // Audio Setup
    mSoundFrequency = 0.f;
    mPhase = 0.f;
    mPhaseAdd = 0.f;
    mModFrequency = 0.0f;
    mModPhase = 0.0f;
    mModPhaseAdd = 0.0f;
    audio::Output::play( audio::createCallback( this, &ClimaxApp::audioCallback ) );
    
    mMaxParticles = 1200;
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
    
    mUseFlocking = true;
    mTargetSeparation = 20.f;
    mNeighboringDistance = 50.f;
    mSeparationFactor = 1.2f;
    mAlignmentFactor = .9f;
    mCohesionFactor = .4f;
    
    configFilename = "config.xml";
    mParams = params::InterfaceGl( getWindow(), "Settings", toPixels( Vec2i( GUI_WIDTH, getWindowHeight() - PARTICLE_AREA_PADDING * 2.f ) ) );
    mConfig = new config::Config( & mParams );
    
    mParams.addText( "Particles", "label=`Particles`" );
    mConfig->addParam( "Max Particles", & mMaxParticles , "" );
    
    mConfig->addParam( "Particle Color" , & mParticleColor );
    mConfig->addParam( "Target Separation", & mTargetSeparation, "min=-500.f max=500.f" );
    mConfig->addParam( "Neighboring Distance", & mNeighboringDistance, "min=0.f max=1000.f" );
    mParams.addButton( "Randomize Particle Properties" , std::bind( & ClimaxApp::randomizeParticleProperties, this ), "key=R" );
    
    mConfig->addParam( "Flocking Enabled", & mUseFlocking, "key=f" );
    mConfig->addParam( "Separation Factor", & mSeparationFactor, "min=-5.f max=5.f step=0.01" );
    mConfig->addParam( "Alignment Factor", & mAlignmentFactor, "min=-5.f max=5.f" );
    mConfig->addParam( "Cohesion Factor", & mCohesionFactor, "min=-50.f max=50.f" );
    mParams.addButton( "Randomize Flocking Parameters" , std::bind( & ClimaxApp::randomizeFlockingProperties, this ), "key=F" );
    mParams.addSeparator();
    
    mParams.addText( "Forces", "label=`Forces`" );
    mConfig->addParam( "Pull Particles to Center", & mParticlesPullToCenter , "key=c" );
    mConfig->addParam( "Pull Factor", & mParticlesPullFactor, "min=0.f max=1.f step=.0001f" );
    mConfig->addParam( "Attraction Enabled", & mUseAttraction, "key=a" );
    mConfig->addParam( "Attraction Factor", & mAttrFactor, "min=0.f max=10.f step=.001f" );
    mParams.addSeparator();
    mConfig->addParam( "Repulsion Enabled", & mUseRepulsion, "key=r" );
    mConfig->addParam( "Repulsion Factor", & mRepulsionFactor, "min=-10.f max=10.f step=.001f" );
    mConfig->addParam( "Repulsion Radius", & mRepulsionRadius, "min=0.f max=800.f" );
    mParams.addSeparator();
    
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
    Vec2f center = getWindowCenter();
    
    float maxFrequency = 15000.f;
    float targetFrequency = ( getMousePos().y / (float)getWindowHeight() ) * maxFrequency;
    targetFrequency = mSoundFrequency - 10000.f;
    mSoundFrequency = math<float>::clamp( targetFrequency, 0.f, maxFrequency );
    
    float maxModFrequency = 30.0f;
    float targetModFrequency = ( getMousePos().x / (float)getWindowWidth() ) * maxModFrequency;
    mModFrequency = math<float>::clamp( targetModFrequency, 0.0f, maxModFrequency );
    
    mParticleSystem.maxParticles = mMaxParticles;
    
    for ( auto it : mParticleSystem.particles ){
        
        it->separationEnabled = mUseFlocking;
        it->separationFactor = mSeparationFactor;
        it->alignmentEnabled = mUseFlocking;
        it->alignmentFactor = mAlignmentFactor;
        it->cohesionEnabled = mUseFlocking;
        it->cohesionFactor = mCohesionFactor;
        
        for( auto second : mParticleSystem.particles ){
            float d = it->position.distance( second->position );
            float d2 = ( it->radius + second->radius ) * 100.f;
            if( d > 0.f && d <= d2 && d < 500.f ) {
                mSoundFrequency = 1000.f;
                mSoundFrequency += 500.f * ( 1.f - ( it->radius * it->color.get( CM_RGB ).length() / 4000.f ) );
                mSoundFrequency += 500.f * ( 1.f - ( second->radius * second->color.get( CM_RGB ).length() / 4000.f ) );
            }
        }

        
        if ( mParticlesPullToCenter ){
            Vec2f force = ( center - it->position ) * .01f;
            it->forces += force;
        }
        
        if ( mUseAttraction ){
            Vec2f attrForce = mForceCenter - it->position;
            attrForce *= mAttrFactor;
            it->forces += attrForce;
        }
        
        if ( mUseRepulsion ){
            Vec2f repForce = it->position - mForceCenter;
            repForce = repForce.normalized() * math<float>::max( 0.f, mRepulsionRadius - repForce.length() );
            it->forces += repForce;
        }
    }
    
    mSoundFrequency = math<float>::clamp( mSoundFrequency, 0.0f, maxFrequency );
    
    mParticleSystem.update();
}

void ClimaxApp::addNewParticleAtPosition( const Vec2f &position )
{
    float radius = ci::randFloat( mParticleRadiusMin, mParticleRadiusMax );
    float mass = radius;
    float drag = .95f;
    
    Particle * particle = new Particle( position, radius, mass, drag, mTargetSeparation, mNeighboringDistance, mParticleColor );
    mParticleSystem.addParticle( particle );
}

void ClimaxApp::randomizeParticleProperties()
{
    mParticleColor = Color( randFloat(), randFloat(), randFloat() );
    mParticleRadiusMin = randFloat( .4f, .8f );
    mParticleRadiusMax = randFloat( 1.2f, 2.4f );
    mTargetSeparation = randFloat( 5.f, 100.f );
    mNeighboringDistance = randFloat( 5.f, 120.f );
}

void ClimaxApp::randomizeFlockingProperties()
{
    mSeparationFactor = randFloat();
    mAlignmentFactor = randFloat();
    mCohesionFactor = randFloat();
}

void ClimaxApp::mouseDown( MouseEvent event )
{
    randomizeParticleProperties();
}

void ClimaxApp::mouseMove( MouseEvent event )
{
    mForceCenter = event.getPos();
    
    bool repulsionUsed = mUseRepulsion;
    
    if ( event.isMetaDown() )
    {
        mUseRepulsion = false;
        addNewParticleAtPosition( event.getPos() );
    }
    
    mUseRepulsion = repulsionUsed;
}

void ClimaxApp::mouseUp( MouseEvent event )
{
}

void ClimaxApp::mouseDrag( MouseEvent event )
{
}

void ClimaxApp::resize()
{
    mParticlesBorder = getWindowBounds();
    mForceCenter = mParticlesBorder.getCenter();
    mParticleSystem.setBorders( ci::Area( mParticlesBorder ) );
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

void ClimaxApp::audioCallback( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *buffer )
{
    if ( mOutput.size() != ioSampleCount ) {
        mOutput.resize( ioSampleCount );
    }
    mPhaseAdd += ( ( mSoundFrequency / 44100.0f ) - mPhaseAdd ) * 0.1f;
    mModPhaseAdd += ( ( mModFrequency / 44100.0f ) - mModPhaseAdd ) * 0.1f;
    int numChannels = buffer->mNumberChannels;
    for( int i=0; i<ioSampleCount; i++ ){
        mPhase += mPhaseAdd;
        mModPhase += mModPhaseAdd;
        float output = math<float>::sin( mPhase * 2.0f * M_PI ) * math<float>::sin( mModPhase * 2.0f * M_PI );
        for( int j=0; j<numChannels; j++ ){
            buffer->mData[ i*numChannels + j ] = output;
        }
        mOutput[i] = output;
    }
}


void ClimaxApp::draw()
{
	gl::clear();
    gl::enableAlphaBlending();
    gl::setViewport( getWindowBounds() );
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    
    gl::color( Color::white() );
    
    mParticlesFbo.bindFramebuffer();
    gl::clear();
    gl::color( Color::black() );
    gl::drawSolidRect( getWindowBounds() );
    mParticlesFbo.unbindFramebuffer();
    
//    gl::Texture text( mParticlesFbo.getTexture() );
//    text.setFlipped(true);
//    gl::draw( text );
    
    mParticleSystem.draw();
    
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
