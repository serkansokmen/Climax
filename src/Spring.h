#pragma once

#include "Particle.h"
#include "cinder/gl/gl.h"


class Spring {

public:

    Spring(Particle *particleA, Particle *particleB, float rest, float strength);
    void update();
    void draw();

    Particle *particleA;
    Particle *particleB;
    float strength, rest;
};
