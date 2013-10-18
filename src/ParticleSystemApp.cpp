#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "cinder/Rect.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"

#include "CinderConfig.h"

#include "Resources.h"
#include "ParticleSystem.h"

#define GUI_WIDTH               280
#define PARTICLE_AREA_PADDING   20.f

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
    
    Vec2f       mForceCenter;
    ci::Area    mParticlesArea;
    
    Color   mParticleColor;
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
    bool    mUseFlocking;
    bool    mAddParticleWithTouch;
};

void ParticleSystemApp::prepareSettings( Settings * settings )
{
    settings->setWindowSize( 1280, 720 );
    settings->enableMultiTouch( false );
    settings->setFullScreen( true );
    settings->enableHighDensityDisplay();
    settings->setResizable( true );
    settings->setBorderless( false );
}

void ParticleSystemApp::setup()
{
    gl::enableAlphaBlending();
    gl::clear( Color::black() );
    
    mParticleTexture = gl::Texture( loadImage( loadResource( RES_PARTICLE_IMAGE ) ) );
    
    mParticleColor = Color::white();
    mParticleRadiusMin = .6f;
    mParticleRadiusMax = 6.f;
    
    mUseAttraction = false;
    mAttrFactor = .05f;
    
    mUseRepulsion = false;
    mRepulsionFactor = -10.f;
    mRepulsionRadius = 35.f;
    
    mUseFlocking = true;
    mSeperationFactor = 1.2f;
    mAlignmentFactor = .9f;
    mCohesionFactor = .4f;
    mAddParticleWithTouch = true;
    
    mNumParticles = 40;
    
    configFilename = "config.xml";
    mParams = params::InterfaceGl( getWindow(), "Parameters", toPixels( Vec2i( GUI_WIDTH, getWindowHeight() - PARTICLE_AREA_PADDING * 2.f ) ) );
    mConfig = new config::Config( & mParams );
    
    mParams.addText( "Particles", "label=`Particles Settings`" );
    mConfig->addParam( "Add on Touch", & mAddParticleWithTouch , "key=q" );
    mConfig->addParam( "Minimum Particle Radius", & mParticleRadiusMin, "min=0.01f max=1.f step=.01f keyIncr=a keyDecr=A" );
    mConfig->addParam( "Maximum Particle Radius", & mParticleRadiusMax, "min=1.f max=20.f step=.1f" );
    mConfig->addParam( "Particle Color" , & mParticleColor );
    mParams.addSeparator();
    
    mParams.addText( "Forces", "label=`Forces`" );
    mConfig->addParam( "Attraction Enabled", & mUseAttraction, "key=w" );
    mConfig->addParam( "Attraction Factor", & mAttrFactor, "min=0.f max=10.f step=.01f" );
    mParams.addSeparator();
    mConfig->addParam( "Repulsion Enabled", & mUseRepulsion, "key=e" );
    mConfig->addParam( "Repulsion Factor", & mRepulsionFactor, "min=-10.f max=10.f step=.01f" );
    mConfig->addParam( "Repulsion Radius", & mRepulsionRadius, "min=0.f max=800.f" );
    mParams.addSeparator();
    
    mParams.addText( "Flocking", "label=`Flocking`" );
    mConfig->addParam( "Flocking Enabled", & mUseFlocking, "" );
    mConfig->addParam( "Seperation Factor", & mSeperationFactor, "min=-5.f max=5.f step=0.01" );
    mConfig->addParam( "Alignment Factor", & mAlignmentFactor, "min=-5.f max=5.f step=0.01" );
    mConfig->addParam( "Cohesion Factor", & mCohesionFactor, "min=-5.f max=5.f step=0.01" );
    
    mParams.addSeparator();
    mParams.addText( "Settings", "label=`Settings`" );
    mParams.addButton( "Save", bind( & ParticleSystemApp::saveConfig, this) );
    mParams.addButton( "Reload", bind( & ParticleSystemApp::loadConfig, this) );
    
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
        
        particle->seperationEnabled = mUseFlocking;
        particle->seperationFactor = mSeperationFactor;
        particle->alignmentEnabled = mUseFlocking;
        particle->alignmentFactor = mAlignmentFactor;
        particle->cohesionEnabled = mUseFlocking;
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
    
    Particle * particle = new Particle( position, radius, mass, drag, mParticleColor );
    mParticleSystem.addParticle( particle );
}

void ParticleSystemApp::mouseDown( MouseEvent event )
{
    mForceCenter = event.getPos();
    
    if ( mAddParticleWithTouch )
        addNewParticleAtPosition( event.getPos() );
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
    ci::Rectf rect(GUI_WIDTH + PARTICLE_AREA_PADDING * 2.f,
                   PARTICLE_AREA_PADDING,
                   getWindowWidth() - PARTICLE_AREA_PADDING,
                   getWindowHeight() - PARTICLE_AREA_PADDING);
    mParticlesArea = ci::Area( rect );
    mForceCenter = rect.getCenter();
    mParticleSystem.setBorders( mParticlesArea );
}

void ParticleSystemApp::keyDown( KeyEvent event )
{
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

void ParticleSystemApp::draw()
{
	gl::clear();
    gl::enableAlphaBlending();
    gl::setViewport( getWindowBounds() );
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    
    gl::color( Color::white() );
    gl::drawStrokedRect( mParticlesArea );
    mParticleSystem.draw();
    
    if ( mParams.isVisible() ) mParams.draw();
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
