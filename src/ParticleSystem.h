#pragma once

#include "Particle.h"
#include "Spring.h"
#include "cinder/Area.h"

#include <vector>

#define MAX_PARTICLES   200

class ParticleSystem {
    
    ci::Area        borders;
    ci::BSpline2f   spline;
    
public:
    
    ~ParticleSystem();
    
    void update();
    void draw();
    
    void addParticle( Particle * particle );
    void destroyParticle( Particle * particle );
    void clear();
    
    void addSpring( Spring * spring );
    void destroySpring( Spring * spring );
    
    void computeBspline();
    
    int  maxParticles;
    
    std::vector< Particle * >   particles;
    std::vector< Spring * >     springs;
};