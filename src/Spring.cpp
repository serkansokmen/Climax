#include "Spring.h"


Spring::Spring(Particle *particleA, Particle *particleB, float rest, float strength)
{
    this->particleA = particleA;
    this->particleB = particleB;
    this->rest = rest;
    this->strength = strength;
}

void Spring::update()
{
    ci::Vec2f delta = particleA->position - particleB->position;
    float length = delta.length();
    float invMassA = 1.0f / particleA->mass;
    float invMassB = 1.0f / particleB->mass;
    float normDist = (length - rest) / (length * (invMassA +
                                                     invMassB)) * strength;
    particleA->position -= delta * normDist * invMassA;
    particleB->position += delta * normDist * invMassB;
}

void Spring::draw()
{
    float distBetweenParticles = particleA->position.distance(particleB->position);
    float distancePercent = 1.f - (distBetweenParticles / 100.f);

    if (distancePercent > 0.f){
        ci::Color colorFirst = ci::lerp(particleA->color, particleB->color, distancePercent);
        ci::gl::color(ci::ColorA( colorFirst, distancePercent * .8f));
        ci::Vec2f conVec = particleB->position - particleA->position;
        conVec.normalize();
        ci::gl::drawLine(particleA->position + conVec * (particleA->radius + .5f),
                          particleB->position - conVec * (particleB->radius + .5f));
    }
//
//
//    ci::gl::color(ci::ColorA(1.f, 1.f, 1.f, 1.f));
//    ci::gl::drawLine(particleA->position, particleB->position);
}
