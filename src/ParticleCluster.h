#pragma once

#include "Particle.h"

class ParticleCluster {

public:

    void addParticle(Particle * particle);
    void removeParticle(Particle * particle);

    ci::Color color;
    std::vector<Particle *> particles;
};
