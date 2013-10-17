#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"

#include "CinderConfig.h"

#include "Resources.h"
#include "ParticleSystem.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class ParticleSystemApp : public AppNative {

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
    void saveConfig();
    void loadConfig();
    
    string              configFilename;
    params::InterfaceGl mParams;
    config::Config*     mConfig;
    
    ParticleSystem  mParticleSystem;
    gl::Texture     mParticleTexture;
    
    Vec2f   mForceCenter;
    
    float   mParticleRadiusMin, mParticleRadiusMax;
    
    float   mAttrFactor;
    float   mRepulsionFactor;
    float   mRepulsionRadius;
    float   mSeperationFactor;
    float   mAlignmentFactor;
    float   mCohesionFactor;
    
    int     mNumParticles;
    
    bool    mUseAttraction;
    bool    mUseRepulsion;
    bool    mUseSeperation;
    bool    mUseAlignment;
    bool    mUseCohesion;
    bool    mShowParams;
};

void ParticleSystemApp::prepareSettings( Settings * settings )
{
    settings->setWindowSize( 1280, 720 );
    settings->enableMultiTouch( false );
    settings->enableHighDensityDisplay();
    settings->setResizable( true );
    settings->setBorderless( false );
}

void ParticleSystemApp::setup()
{
    gl::enableAlphaBlending();
    gl::clear( Color::black() );
    
    configFilename = "config.xml";
    
    mParticleTexture = gl::Texture( loadImage( loadResource( RES_PARTICLE_IMAGE ) ) );
    
    mParticleRadiusMin = .6f;
    mParticleRadiusMax = 6.f;
    
    mUseAttraction = false;
    mForceCenter = getWindowCenter();
    mAttrFactor = .05f;
    
    mUseRepulsion = false;
    mRepulsionFactor = -10.f;
    mRepulsionRadius = 35.f;
    
    mUseSeperation = true;
    mSeperationFactor = 1.2f;
    mUseAlignment = true;
    mAlignmentFactor = .9f;
    mUseCohesion = true;
    mCohesionFactor = .4f;
    
    mNumParticles = 40;
    
    mShowParams = true;
    
    mParams = params::InterfaceGl( getWindow(), "Parameters", toPixels( Vec2i( 280, 260 ) ) );
    mConfig = new config::Config( & mParams );
    
    mParams.addParam( "Configuration file name", & configFilename );
    mParams.addButton( "Save config", bind( & ParticleSystemApp::saveConfig, this) );
    mParams.addButton( "Load config", bind( & ParticleSystemApp::loadConfig, this) );
    mParams.addSeparator();
    
    mParams.addText( "text", "label=`Particle Radius Randomness`" );
    mConfig->addParam( "Minimum", & mParticleRadiusMin, "min=0.01f max=1.f step=.01f keyIncr=z keyDecr=Z" );
    mConfig->addParam( "Maximum", & mParticleRadiusMax, "min=1.f max=20.f step=.1f keyIncr=x keyDecr=X" );
    mParams.addSeparator();
    
    mConfig->addParam( "Use Attraction", & mUseAttraction, "" );
    mConfig->addParam( "Attraction Factor", & mAttrFactor, "min=0.f max=10.f step=.01f" );
    mParams.addSeparator();
    mConfig->addParam( "Use Repulsion", & mUseRepulsion, "" );
    mConfig->addParam( "Repulsion Factor", & mRepulsionFactor, "min=-10.f max=10.f step=.01f" );
    mConfig->addParam( "Repulsion Radius", & mRepulsionRadius, "min=0.f max=800.f" );
    mParams.addSeparator();
    mConfig->addParam( "Use Seperation", & mUseSeperation, "" );
    mConfig->addParam( "Seperation", & mSeperationFactor, "min=-5.f max=5.f step=0.01" );
    mConfig->addParam( "Use Alignment", & mUseAlignment, "" );
    mConfig->addParam( "Alignment", & mAlignmentFactor, "min=-5.f max=5.f step=0.01" );
    mConfig->addParam( "Use Cohesion", & mUseCohesion, "" );
    mConfig->addParam( "Cohesion", & mCohesionFactor, "min=-5.f max=5.f step=0.01" );
    
    // Try to restore last saved parameters configuration
    try {
        loadConfig();
    } catch ( Exception e ) {
        console() << e.what() << std::endl;
    }
}

void ParticleSystemApp::update()
{
    Vec2f center = getWindowCenter();
    for (vector<Particle*>::iterator it = mParticleSystem.particles.begin(); it != mParticleSystem.particles.end(); ++it)
    {
        Particle * particle =  * it;
        
        particle->seperationEnabled = mUseSeperation;
        particle->seperationFactor = mSeperationFactor;
        particle->alignmentEnabled = mUseAlignment;
        particle->alignmentFactor = mAlignmentFactor;
        particle->cohesionEnabled = mUseCohesion;
        particle->cohesionFactor = mCohesionFactor;
        
        Vec2f force = ( center - particle->position ) * .01f;
        particle->forces += force;
        
        if ( mUseAttraction )
        {
            Vec2f attrForce = mForceCenter - particle->position;
            attrForce *= mAttrFactor;
            particle->forces += attrForce;
        }
        
        if ( mUseRepulsion )
        {
            Vec2f repForce = particle->position - mForceCenter;
            repForce = repForce.normalized() * math<float>::max(0.f, mRepulsionRadius - repForce.length() );
            particle->forces += repForce;
        }
    }
    mParticleSystem.update();
}

void ParticleSystemApp::addNewParticleAtPosition( const Vec2f &position )
{
    float radius = ci::randFloat( mParticleRadiusMin, mParticleRadiusMax );
    float mass = radius;
    float drag = .95f;
    
    Particle * particle = new Particle( position, radius, mass, drag );
    mParticleSystem.addParticle( particle );
}

void ParticleSystemApp::mouseDown( MouseEvent event )
{
    mForceCenter = event.getPos();
}

void ParticleSystemApp::mouseMove( MouseEvent event )
{
    if ( event.isMetaDown() )
    {
        addNewParticleAtPosition( event.getPos() );
    }
}

void ParticleSystemApp::mouseUp( MouseEvent event )
{
}

void ParticleSystemApp::mouseDrag( MouseEvent event )
{
    mForceCenter = event.getPos();
}

void ParticleSystemApp::resize()
{
    mParticleSystem.setBorders( getWindowBounds() );
}

void ParticleSystemApp::keyDown( KeyEvent event )
{
    if ( event.getChar() == ' ' )
    {
        mParticleSystem.clear();
    }
    if ( event.getChar() == 's' )
        mShowParams = !mShowParams;
}

void ParticleSystemApp::draw()
{
//    gl::enableDepthRead();
//	gl::enableDepthWrite();
	gl::clear();
    gl::enableAlphaBlending();
    gl::setViewport( getWindowBounds() );
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    
    mParticleSystem.draw();
    
    if ( mShowParams ) mParams.draw();
}

void ParticleSystemApp::saveConfig()
{
    mConfig->save( getAppPath() / fs::path( configFilename ) );
    console() << "Saved to: " << getAppPath() / fs::path( configFilename ) << std::endl;
}

void ParticleSystemApp::loadConfig()
{
    mConfig->load( getAppPath() / fs::path( configFilename ) );
}

CINDER_APP_NATIVE( ParticleSystemApp, RendererGl )
