#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "cinder/Rect.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Fbo.h"
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
    void randomizeParticleProperties();
    void saveConfig();
    void loadConfig();
    
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
    
    float   mAttrFactor;
    float   mRepulsionFactor;
    float   mRepulsionRadius;
    
    float   mTargetSeparation;
    float   mNeighboringDistance;
    float   mSeparationFactor;
    float   mAlignmentFactor;
    float   mCohesionFactor;
    
    int     mMaxParticles;
    
    bool    mParticlesPullToCenter;
    bool    mUseAttraction;
    bool    mUseRepulsion;
    bool    mUseFlocking;
    bool    mAddParticleWithTouch;
};

void ParticleSystemApp::prepareSettings( Settings * settings )
{
    settings->setWindowSize( 1280, 720 );
    settings->enableMultiTouch( false );
    settings->setFullScreen( false );
    settings->enableHighDensityDisplay();
    settings->setResizable( true );
    settings->setBorderless( false );
    settings->setTitle( "Climax" );
}

void ParticleSystemApp::setup()
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
    
    mMaxParticles = 1200;
    mParticleColor = Color::white();
    mParticleRadiusMin = .6f;
    mParticleRadiusMax = 6.f;
    
    mParticlesPullToCenter = true;
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
    mAddParticleWithTouch = true;
    
    configFilename = "config.xml";
    mParams = params::InterfaceGl( getWindow(), "Settings", toPixels( Vec2i( GUI_WIDTH, getWindowHeight() - PARTICLE_AREA_PADDING * 2.f ) ) );
    mConfig = new config::Config( & mParams );
    
    mParams.addText( "Particles", "label=`Particles`" );
    mConfig->addParam( "Max Particles", & mMaxParticles , "" );
    mConfig->addParam( "Add on Touch", & mAddParticleWithTouch , "key=q" );
    mConfig->addParam( "Minimum Particle Radius", & mParticleRadiusMin, "min=0.01f max=1.f step=.01f keyIncr=Z keyDecr=z" );
    mConfig->addParam( "Maximum Particle Radius", & mParticleRadiusMax, "min=1.f max=20.f step=.1f keyIncr=X keyDecr=x" );
    mConfig->addParam( "Particle Color" , & mParticleColor );
    mParams.addButton( "Randomize Particle Color" , std::bind( & ParticleSystemApp::randomizeParticleProperties, this ), "key=R" );
    mParams.addSeparator();
    
    mParams.addText( "Forces", "label=`Forces`" );
    mConfig->addParam( "Pull Particles to Center", & mParticlesPullToCenter , "key=c" );
    mConfig->addParam( "Attraction Enabled", & mUseAttraction, "key=a" );
    mConfig->addParam( "Attraction Factor", & mAttrFactor, "min=0.f max=10.f step=.001f" );
    mParams.addSeparator();
    mConfig->addParam( "Repulsion Enabled", & mUseRepulsion, "key=r" );
    mConfig->addParam( "Repulsion Factor", & mRepulsionFactor, "min=-10.f max=10.f step=.001f" );
    mConfig->addParam( "Repulsion Radius", & mRepulsionRadius, "min=0.f max=800.f" );
    mParams.addSeparator();
    
    mParams.addText( "Flocking", "label=`Flocking`" );
    mConfig->addParam( "Flocking Enabled", & mUseFlocking, "key=f" );
    mConfig->addParam( "Target Separation", & mTargetSeparation, "min=-500.f max=500.f" );
    mConfig->addParam( "Neighboring Distance", & mNeighboringDistance, "min=0.f max=1000.f" );
    mConfig->addParam( "Separation Factor", & mSeparationFactor, "min=-5.f max=5.f step=0.01" );
    mConfig->addParam( "Alignment Factor", & mAlignmentFactor, "min=-5.f max=5.f" );
    mConfig->addParam( "Cohesion Factor", & mCohesionFactor, "min=-50.f max=50.f" );
    
    mParams.addSeparator();
    mParams.addText( "Settings", "label=`Settings`" );
    mParams.addButton( "Save Settings", bind( & ParticleSystemApp::saveConfig, this ) );
    mParams.addButton( "Reload Settings", bind( & ParticleSystemApp::loadConfig, this ), "key=L" );
    
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
        
        particle->separationEnabled = mUseFlocking;
        particle->separationFactor = mSeparationFactor;
        particle->alignmentEnabled = mUseFlocking;
        particle->alignmentFactor = mAlignmentFactor;
        particle->cohesionEnabled = mUseFlocking;
        particle->cohesionFactor = mCohesionFactor;
        
        if ( mParticlesPullToCenter ) {
            Vec2f force = ( center - particle->position ) * .01f;
            particle->forces += force;
        }
        
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
    mParticleSystem.maxParticles = mMaxParticles;
    mParticleSystem.update();
}

void ParticleSystemApp::addNewParticleAtPosition( const Vec2f &position )
{
    float radius = ci::randFloat( mParticleRadiusMin, mParticleRadiusMax );
    float mass = radius;
    float drag = .95f;
    
    Particle * particle = new Particle( position, radius, mass, drag, mTargetSeparation, mNeighboringDistance, mParticleColor );
    mParticleSystem.addParticle( particle );
}

void ParticleSystemApp::randomizeParticleProperties()
{
    mParticleColor = Color( randFloat(), randFloat(), randFloat() );
    mTargetSeparation = randFloat( 0.f, 100.f );
    mNeighboringDistance = randFloat( 0.f, 400.f );
}

void ParticleSystemApp::mouseDown( MouseEvent event )
{
//    mForceCenter = event.getPos();
    randomizeParticleProperties();
    
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
//    mForceCenter = event.getPos();
}

void ParticleSystemApp::resize()
{
    mParticlesBorder = ci::Rectf(GUI_WIDTH + PARTICLE_AREA_PADDING * 2.f,
                                 PARTICLE_AREA_PADDING,
                                 getWindowWidth() - PARTICLE_AREA_PADDING,
                                 getWindowHeight() - PARTICLE_AREA_PADDING);
    mForceCenter = mParticlesBorder.getCenter();
    mParticleSystem.setBorders( ci::Area( mParticlesBorder ) );
}

void ParticleSystemApp::keyDown( KeyEvent event )
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

void ParticleSystemApp::draw()
{
	gl::clear();
    gl::enableAlphaBlending();
    gl::setViewport( getWindowBounds() );
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    
    gl::color( Color::white() );
    gl::drawStrokedRect( mParticlesBorder );
    
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
