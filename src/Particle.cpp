#include "Particle.h"


Particle::Particle( const ci::Vec2f & position, float radius, float mass, float drag, const ci::Color & color )
{
    this->position = position;
    this->radius = radius;
    this->mass = mass;
    this->drag = drag;
    this->color = color;
    this->maxSpeed = 1.f;
    this->maxForce = .05f;
    
    anchor = position;
    prevPosition = position;
    forces = ci::Vec2f::zero();
    
    seperationEnabled = false;
    seperationFactor = 1.f;
    alignmentEnabled = false;
    alignmentFactor = 1.f;
    cohesionEnabled = false;
    cohesionFactor = 1.f;
}

void Particle::update()
{
    ci::Vec2f temp = position;
    
    position += velocity + forces / mass;
    prevPosition = temp;
    
    forces = ci::Vec2f::zero();
}

void Particle::flock( std::vector<Particle *> & particles )
{
    ci::Vec2f acc = ci::Vec2f::zero();
    
    if ( seperationEnabled )    acc += seperate( particles ) * seperationFactor;
    if ( alignmentEnabled )     acc += align( particles ) * alignmentFactor;
    if ( cohesionEnabled )      acc += cohesion( particles ) * cohesionFactor;
    
    velocity += acc;
    velocity.limit( maxSpeed );
}

void Particle::borders( const ci::Area & borders )
{
    if ( position.x < borders.getX1() || position.x > borders.getX2() ) velocity.x *= -1.f;
    if ( position.y < borders.getY1() || position.y > borders.getY2() ) velocity.y *= -1.f;
}

ci::Vec2f Particle::steer( ci::Vec2f target, bool slowdown )
{
    ci::Vec2f steer;
    ci::Vec2f desired = target - position;
    float d = desired.length();
    if (d >0) {
        desired.normalize();
        if ( ( slowdown ) && ( d < 100.f )) desired *= ( maxSpeed * ( d / 100.f ) );
        else desired *= maxSpeed;
        steer = desired - velocity;
        steer.limit(maxForce);
    } else {
        steer = ci::Vec2f::zero();
    }
    return steer;}

ci::Vec2f Particle::seperate( std::vector<Particle *> & particles )
{
    ci::Vec2f resultVec = ci::Vec2f::zero();
    float targetSeparation = 30.f;
    int count = 0;
    for( std::vector<Particle*>::const_iterator it = particles.begin(); it
        != particles.end(); ++it ) {
        ci::Vec2f diffVec = position - (*it)->position;
        if( diffVec.length() >0 && diffVec.length() < targetSeparation ) {
            resultVec += diffVec.normalized() / diffVec.length();
            count++;
        } }
    if (count >0) {
        resultVec /= (float)count;
    }
    if (resultVec.length() >0) {
        resultVec.normalize();
        resultVec *= maxSpeed;
        resultVec -= velocity;
        resultVec.limit(maxForce);
    }
    return resultVec;
}

ci::Vec2f Particle::align( std::vector<Particle *> & particles )
{
    ci::Vec2f resultVec = ci::Vec2f::zero();
    float neighborDist = 50.f;
    int count = 0;
    for( std::vector<Particle*>::const_iterator it = particles.begin(); it
        != particles.end(); ++it ) {
        ci::Vec2f diffVec = position - (*it)->position;
        if( diffVec.length() >0 && diffVec.length() <neighborDist ) {
            resultVec += (*it)->velocity;
            count++;
        }
    }
    if (count >0) {
        resultVec /= (float)count;
    }
    if (resultVec.length() >0) {
        resultVec.normalize();
        resultVec *= maxSpeed;
        resultVec -= velocity;
        resultVec.limit(maxForce);
    }
    return resultVec;
}

ci::Vec2f Particle::cohesion( std::vector<Particle *> & particles )
{
    ci::Vec2f resultVec = ci::Vec2f::zero();
    float neighborDist = 50.f;
    int count = 0;
    for( std::vector<Particle*>::const_iterator it = particles.begin(); it != particles.end(); ++it )
    {
        if (resultVec.length() >0)
        {
            resultVec.normalize();
            resultVec *= maxSpeed;
            resultVec -= velocity;
            resultVec.limit(maxForce);
            float d = position.distance( (*it)->position );
            if( d >0 && d <neighborDist ) {
                resultVec += (*it)->position;
                count++;
            }
        }
    }
    if (count >0) {
        resultVec /= (float)count;
        return steer(resultVec, false);
    }
    return resultVec;
}

void Particle::draw()
{
    float outerRadius = radius + radius * .8f;
    ci::gl::color( ci::ColorA( color, .8f ) );
    ci::gl::drawSolidCircle( position, outerRadius );
//    ci::gl::color( ci::ColorA( ci::Color::white(), 1.f ) );
//    ci::gl::drawSolidCircle( position, radius );
    ci::gl::color( ci::ColorA( color, 1.f ) );
    ci::gl::drawStrokedCircle( position, outerRadius );
}
