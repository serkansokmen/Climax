#pragma once

#include "Particle.h"

class ParticleCluster {

    float       targetSeparation;
    float       neighboringDistance;
    
public:
    
    ParticleCluster();
    ParticleCluster(float targetSeparation,
                    float neighboringDistance,
                    const ci::Color & color) {
        this->targetSeparation = targetSeparation;
        this->neighboringDistance = neighboringDistance;
        this->color = color;
    };
    
    void addParticle(Particle * particle){
        particles.push_back(particle);
    };
    void removeParticle(Particle * particle){
        std::vector< Particle *>::const_iterator it = std::find(particles.begin(), particles.end(), particle);
        delete * it;
        particles.erase(it);
    };

    ci::Color color;
    std::vector<Particle *> particles;
};
