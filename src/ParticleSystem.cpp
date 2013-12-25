#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"
#include "ParticleSystem.h"


ParticleSystem::~ParticleSystem()
{
    clear();
}

void ParticleSystem::clear()
{
    for (auto it : particles){
        delete it;
    }
    particles.clear();

    for(auto it : springs){
        delete it;
    }
    springs.clear();
}

void ParticleSystem::update()
{
    if (particles.size() > maxParticles)
        destroyParticle(* particles.begin());

    for (auto particle : particles){
        particle->borders(true);
        particle->update();
        particle->flock(particles);
    }

    for (auto spring : springs)
        spring->update();
}

void ParticleSystem::draw()
{
    for(auto particle : particles)    particle->draw();
    for(auto spring : springs)        spring->draw();
}

void ParticleSystem::addParticle(Particle *particle)
{
    particles.push_back(particle);

    for (auto second : particles){
        if (particle != second && particle->color == second->color){
            float d = particle->position.distance(second->position);
            float d2 = (particle->radius + second->radius) * 50.f;
            
            if (d > 0.f && d2 < 200.f){
                Spring * spring = new Spring(particle, second,
                                             d * ci::randFloat(.4f, 1.8f),
                                             ci::randFloat(.0001f, .005f));
                addSpring(spring);
            }
        }
    }
}

void ParticleSystem::destroyParticle(Particle *particle)
{
    std::vector< Particle *>::const_iterator it = std::find(particles.begin(), particles.end(), particle);
    delete * it;
    particles.erase(it);

    for (auto spring : springs)
        if (spring->particleA == * it || spring->particleB == * it)
            destroySpring(spring);
}

void ParticleSystem::addSpring(Spring *spring)
{
    springs.push_back(spring);
}

void ParticleSystem::destroySpring(Spring *spring)
{
    std::vector<Spring*>::iterator it = std::find(springs.begin(), springs.end(), spring);
    delete *it;
    springs.erase(it);
}
