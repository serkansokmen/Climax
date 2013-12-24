#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "cinder/Rect.h"
#include "cinder/CinderMath.h"
#include "cinder/Timer.h"
#include "cinder/params/Params.h"

#include "CinderConfig.h"

#include "Resources.h"
#include "ParticleCluster.h"
#include "ParticleSystem.h"
#include "BpmTapper.h"
#include "TouchPoint.h"

#include <vector>
#include <map>
#include <list>


#define GUI_WIDTH   320


using namespace ci;
using namespace ci::app;
using namespace std;


class ClimaxApp : public AppNative {

public:

    void prepareSettings(Settings * settings);
	void setup();
    void update();
	void draw();

    void touchesBegan(TouchEvent event);
	void touchesMoved(TouchEvent event);
	void touchesEnded(TouchEvent event);

    void keyDown(KeyEvent event);
    void resize();

    void addNewParticleAtPosition(const Vec2f & position);
    void addNewParticlesOnPolyLine(const PolyLine<Vec2f> & line);
    void randomizeParticleProperties();
    void setHighSeperation();
    void setHighNeighboring();
    void randomizeFlockingProperties();

    void saveConfig();
    void loadConfig();

    void shutdown();

    ParticleSystem  mParticleSystem;
    BpmTapper       * bpmTapper;

    map<uint32_t,TouchPoint>	mActivePoints;
	list<TouchPoint>			mDyingPoints;

#ifndef CINDER_COCOA_TOUCH
    params::InterfaceGl mParams;
    config::Config      * mConfig;
#endif

    Vec2f       mForceCenter;
    Vec2f       mAttractionCenter;

    Color       mParticleColor;
    string      mConfigFileName;

    float   mParticleRadiusMin, mParticleRadiusMax;

    float   mTargetSeparation;
    float   mNeighboringDistance;
    float   mSeparationFactor;
    float   mAlignmentFactor;
    float   mCohesionFactor;
    float   mBpm;

    int     mMaxParticles;
    int     mEmitRes;
    int     mNumParticles;
    int     mNumSprings;

    bool    mUseFlocking;
    bool    mAutoRandParticleProperties;
};

void ClimaxApp::prepareSettings(Settings * settings)
{
#ifdef CINDER_COCOA_TOUCH
    settings->enableHighDensityDisplay();
#else
    settings->setWindowSize(1280, 720);
    settings->setFullScreen(true);
    settings->setResizable(false);
    settings->setBorderless(false);
    settings->setTitle("Climax");
#endif
    settings->enableMultiTouch(true);
}

void ClimaxApp::setup()
{
    gl::enableAlphaBlending();
    gl::clear(Color::black());

    // Set up line rendering
    gl::enable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl::color(ColorAf::white());

    mMaxParticles = 1200;
    mEmitRes = 2;
    mParticleColor = Color::white();
    mParticleRadiusMin = .8f;
    mParticleRadiusMax = 1.6f;

    mAutoRandParticleProperties = false;
    mBpm = 124.f;
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

#ifndef CINDER_COCOA_TOUCH
    mConfigFileName = "config.xml";
    mParams = params::InterfaceGl(getWindow(),
                                  "Settings",
                                  toPixels(Vec2i(GUI_WIDTH,
                                                  getWindowHeight() - 40.f)
                                          )
                                 );
    mConfig = new config::Config(& mParams);

    mParams.addText("Particles", "label=`Particles`");

    mParams.addParam("Particle Count", & mNumParticles, "", true);
    mParams.addParam("Spring Count", & mNumSprings, "", true);

    mConfig->addParam("Max Particles", & mMaxParticles , "");
    mConfig->addParam("Emitter Resolution", & mEmitRes , "");

    mConfig->addParam("BPM Tempo" , & mBpm, "min=100 max=255");
    mConfig->addParam("Cluster Particle Color" , & mParticleColor);
//    mConfig->addParam("Target Separation", & mTargetSeparation, "min=0.1f max=100.f");
//    mConfig->addParam("Neighboring Distance", & mNeighboringDistance, "min=0.1f max=100.f");
    mParams.addButton("Randomize Particle Color & Radius" ,
                      std::bind(& ClimaxApp::randomizeParticleProperties,
                                this), "key=1");
    mConfig->addParam("Auto-Randomize Particle Color" ,
                      & mAutoRandParticleProperties,
                      "key=2");
    mParams.addButton("High Seperation" ,
                      std::bind(& ClimaxApp::setHighSeperation, this),
                      "key=3");
    mParams.addButton("High Neighboring" ,
                      std::bind(& ClimaxApp::setHighNeighboring, this),
                      "key=4");

    mConfig->addParam("Flocking Enabled", & mUseFlocking, "key=f");
    mConfig->addParam("Separation Factor", & mSeparationFactor,
                      "min=-5.f max=5.f step=0.01");
    mConfig->addParam("Alignment Factor", & mAlignmentFactor,
                      "min=-5.f max=5.f");
    mConfig->addParam("Cohesion Factor", & mCohesionFactor,
                      "min=-50.f max=50.f");
    mParams.addButton("Randomize Flocking Parameters" ,
                      std::bind(& ClimaxApp::randomizeFlockingProperties,
                                this), "key=4");
    mParams.addSeparator();

    mParams.addText("Settings", "label=`Settings`");
    mParams.addButton("Save Settings", bind(& ClimaxApp::saveConfig, this));
    mParams.addButton("Reload Settings", bind(& ClimaxApp::loadConfig, this),
                      "key=L");

    // Try to restore last saved parameters configuration
    try {
        loadConfig();
    } catch (Exception e) {
        console() << e.what() << std::endl;
    }
#endif
}

void ClimaxApp::update()
{
    mNumParticles = mParticleSystem.particles.size();
    mNumSprings = mParticleSystem.springs.size();

    bpmTapper->setBpm(mBpm);
    bpmTapper->update();

    if (mAutoRandParticleProperties && bpmTapper->onBeat()){
        bpmTapper->start();
        randomizeParticleProperties();
    }

    for (auto it : mParticleSystem.particles){

        it->separationEnabled = mUseFlocking;
        it->separationFactor = mSeparationFactor;
        it->alignmentEnabled = mUseFlocking;
        it->alignmentFactor = mAlignmentFactor;
        it->cohesionEnabled = mUseFlocking;
        it->cohesionFactor = mCohesionFactor;

//        if (mPullParticles){
//            Vec2f force = (getWindowCenter() - it->position) * .01f;
//            it->forces += force;
//        }
//
//        if (mUseAttraction){
//            Vec2f attrForce = mAttractionCenter - it->position;
//            attrForce.normalize();
//            attrForce *= math<float>::max(0.f, mAttrFactor - attrForce.length());
//            it->forces += attrForce;
//        }
//
//        if (mUseRepulsion && it->position.distance(mAttractionCenter) > mRepulsionRadius){
//            Vec2f repForce = it->position - mAttractionCenter;
//            repForce = repForce.normalized() * math<float>::max(0.f, mRepulsionRadius - repForce.length());
//            it->forces += repForce;
//        }
    }
    mParticleSystem.maxParticles = mMaxParticles;
    mParticleSystem.update();

    mForceCenter = getWindowCenter();
}

void ClimaxApp::addNewParticleAtPosition(const Vec2f & position)
{
    if (getElapsedFrames() % mEmitRes == 0) {
        float radius = ci::randFloat(mParticleRadiusMin, mParticleRadiusMax);
        float mass = radius * radius;
        float drag = .95f;

        Particle * particle = new Particle(position, radius, mass, drag, mTargetSeparation, mNeighboringDistance, mParticleColor);
        mParticleSystem.addParticle(particle);
    }
}

void ClimaxApp::addNewParticlesOnPolyLine(const PolyLine<Vec2f> & line)
{
    if (line.size() == 0) return;
    if (bpmTapper->onBeat())
        for (auto it : line)
            addNewParticleAtPosition(it);
}

void ClimaxApp::setHighSeperation()
{
    mTargetSeparation = randFloat(50.f, 100.f);
    mNeighboringDistance = randFloat(10.f, 50.f);
}

void ClimaxApp::setHighNeighboring()
{
    mTargetSeparation = randFloat(10.f, 50.f);
    mNeighboringDistance = randFloat(50.f, 100.f);
}

void ClimaxApp::randomizeParticleProperties()
{
    mParticleColor = Color(randFloat(), randFloat(), randFloat());
    mParticleRadiusMin = ci::randFloat();
    mParticleRadiusMax = ci::randFloat() * .8f;
}

void ClimaxApp::randomizeFlockingProperties()
{
    mSeparationFactor = randFloat();
    mAlignmentFactor = randFloat();
    mCohesionFactor = randFloat();
}

void ClimaxApp::touchesBegan(TouchEvent event)
{
    for(vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt) {
        Color newColor(CM_HSV, Rand::randFloat(), 1, 1);
        mActivePoints.insert(make_pair(touchIt->getId(), TouchPoint(touchIt->getPos(), newColor)));
    }
}

void ClimaxApp::touchesMoved(TouchEvent event)
{
    for(vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt){
        mActivePoints[touchIt->getId()].addPoint(touchIt->getPos());
    }

    switch (event.getTouches().size()) {
        case 2:
        {
            Vec2f avrg = Vec2f::zero();
            for (auto touch : event.getTouches())
                avrg += touch.getPos();
            avrg /= 2;
            addNewParticleAtPosition(avrg);
        }
            break;
        default:
            break;
    }
}

void ClimaxApp::touchesEnded(TouchEvent event)
{
    for(vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt) {
        mActivePoints[touchIt->getId()].startDying();
        mDyingPoints.push_back(mActivePoints[touchIt->getId()]);
        mActivePoints.erase(touchIt->getId());
    }
}

void ClimaxApp::resize()
{
    mForceCenter = getWindowCenter();
    mAttractionCenter = getWindowCenter();
}

void ClimaxApp::keyDown(KeyEvent event)
{
//    if (event.getChar() == 's' && event.isMetaDown()) {
//        saveConfig();
//        return;
//    }
//    if (event.getChar() == 'l' && event.isMetaDown()) {
//        loadConfig();
//        return;
//    }

    if (event.getChar() == ' ')
    {
        mParticleSystem.clear();
    }
    if (event.getChar() == 'S') {
        if (mParams.isMaximized()) {
            mParams.minimize();
            hideCursor();
        } else
            mParams.maximize();
            showCursor();
    }
}

void ClimaxApp::draw()
{
	gl::clear( Color::black() );
    gl::enableAlphaBlending();
    gl::setViewport(getWindowBounds());
    gl::setMatricesWindow(getWindowWidth(), getWindowHeight());
    gl::lineWidth( .2f );

    gl::color(Color::white());

    mParticleSystem.draw();

//  for(map<uint32_t,TouchPoint>::const_iterator activeIt = mActivePoints.begin(); activeIt != mActivePoints.end(); ++activeIt) {
//		activeIt->second.draw();
//	}
//
//	for(list<TouchPoint>::iterator dyingIt = mDyingPoints.begin(); dyingIt != mDyingPoints.end();) {
//		dyingIt->draw();
//		if(dyingIt->isDead())
//			dyingIt = mDyingPoints.erase(dyingIt);
//		else
//			++dyingIt;
//	}

#ifndef CINDER_COCOA_TOUCH
    if (mParams.isVisible()) {
        // draw yellow circles at the active touch points
//        gl::color(ColorA(.4f, 1.f, .8f, .7f));
//        gl::drawStrokedCircle(mTouchesAvrgVec3, 20.0f);
        mParams.draw();
    }
#endif
}

#ifndef CINDER_COCOA_TOUCH
void ClimaxApp::saveConfig()
{
    mConfig->save(getAppPath() / fs::path(mConfigFileName));
    console() << "Saved configuration to: " << getAppPath() / fs::path(mConfigFileName) << std::endl;
}

void ClimaxApp::loadConfig()
{
    mConfig->load(getAppPath() / fs::path(mConfigFileName));
    console() << "Loaded configuration from: " << getAppPath() / fs::path(mConfigFileName) << std::endl;
}
#endif

void ClimaxApp::shutdown()
{
}

CINDER_APP_NATIVE(ClimaxApp, RendererGl)
