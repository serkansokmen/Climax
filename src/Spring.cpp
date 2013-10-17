#include "Spring.h"


Spring::Spring( Particle *particleA, Particle *particleB, float rest, float strength )
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
    float normDist = ( length - rest ) / ( length * ( invMassA +
                                                     invMassB ) ) * strength;
    particleA->position -= delta * normDist * invMassA;
    particleB->position += delta * normDist * invMassB;
}

void Spring::draw()
{
    ci::gl::color( ci::ColorA( .5f, .7f, .3f, 1.f ) );
    ci::gl::drawLine( particleA->position, particleB->position );
}