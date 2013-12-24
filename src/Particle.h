#pragma once

#include "cinder/gl/gl.h"
#include "cinder/Vector.h"
#include "cinder/Rect.h"
#include "cinder/Color.h"


class Particle {

    ci::Vec2f   prevPosition;
    float       targetSeparation;
    float       neighboringDistance;

public:

    Particle();
    Particle(const ci::Vec2f & position,
              float radius, float mass, float drag,
              float targetSeparation,
              float neighboringDistance,
              const ci::Color & color);

    void update();
    void draw();

    void flock(std::vector<Particle * > & particles);
    void borders(bool bounce = true);

    ci::Vec2f steer(ci::Vec2f target, bool slowdown);
    ci::Vec2f separate(std::vector<Particle * > & particles);
    ci::Vec2f align(std::vector<Particle * > & particles);
    ci::Vec2f cohesion(std::vector<Particle * > & particles);

    ci::Vec2f anchor;
    ci::Vec2f position;
    ci::Vec2f forces;
    ci::Vec2f velocity;

    ci::Color color;

    float separationFactor, alignmentFactor, cohesionFactor;

    float radius;
    float drag;
    float maxSpeed;
    float maxForce;
    float mass;

    bool separationEnabled;
    bool alignmentEnabled;
    bool cohesionEnabled;
};
