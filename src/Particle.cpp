#include "Particle.h"


Particle::Particle( const ci::Vec2f & position,
                    float radius, float mass, float drag,
                    float targetSeparation, float neighboringDistance,
                    const ci::Color & color )
{
    this->position = position;
    this->radius = radius;
    this->mass = mass;
    this->drag = drag;
    this->targetSeparation = targetSeparation;
    this->neighboringDistance = neighboringDistance;
    this->color = color;
    this->maxSpeed = 1.f;
    this->maxForce = .05f;
    
    this->radius = targetSeparation / neighboringDistance * 1.6f;
    if ( this->radius > 10.f )
        this->radius = 10.f;
    
    anchor = position;
    prevPosition = position;
    forces = ci::Vec2f::zero();
    
    separationEnabled = false;
    separationFactor = 1.f;
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
    
    if ( separationEnabled )    acc += separate( particles ) * separationFactor;
    if ( alignmentEnabled )     acc += align( particles ) * alignmentFactor;
    if ( cohesionEnabled )      acc += cohesion( particles ) * cohesionFactor;
    
    velocity += acc;
    velocity.limit( maxSpeed );
}

void Particle::borders( bool bounce )
{
    ci::Rectf borders( ci::app::getWindowBounds() );
    
    if ( bounce )
    {
        if ( position.x <= borders.getX1() || position.x >= borders.getX2() ) velocity.x *= -1.f;
        if ( position.y <= borders.getY1() || position.y >= borders.getY2() ) velocity.y *= -1.f;
    }
    else
    {
        if ( position.x <= borders.getX1() + radius )
             position.x = borders.getX2() - radius;
        
        else if ( position.y <= borders.getY1() + radius )
            position.y = borders.getY2() - radius;
        
        else if ( position.x >= borders.getX2() - radius )
            position.x = borders.getX1() + radius;
        
        else if ( position.y >= borders.getY2() - radius )
            position.y = borders.getY1() + radius;
    }
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

ci::Vec2f Particle::separate( std::vector<Particle *> & particles )
{
    ci::Vec2f resultVec = ci::Vec2f::zero();
    
    int count = 0;
    
    for( auto it : particles )
    {
        ci::Vec2f diffVec = position - it->position;
        if ( diffVec.length() > 0 && diffVec.length() < targetSeparation )
        {
            resultVec += diffVec.normalized() / diffVec.length();
            count++;
        }
    }
    
    if ( count >0 )
    {
        resultVec /= (float)count;
    }
    
    if ( resultVec.length() > 0 )
    {
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
    int count = 0;
    for ( auto it : particles ) {
        ci::Vec2f diffVec = position - it->position;
        if( diffVec.length() >0 && diffVec.length() < neighboringDistance ) {
            resultVec += it->velocity;
            count++;
        }
    }
    
    if ( count > 0 )
    {
        resultVec /= (float)count;
    }
    
    if ( resultVec.length() > 0 )
    {
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
    int count = 0;
    for ( auto it : particles )
    {
        if (resultVec.length() >0)
        {
            resultVec.normalize();
            resultVec *= maxSpeed;
            resultVec -= velocity;
            resultVec.limit( maxForce );
            float d = position.distance( it->position );
            if ( d > 0 && d < neighboringDistance )
            {
                resultVec += it->position;
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
    if ( this->radius < 5.f )
    {
        ci::gl::color( color );
        ci::gl::drawSolidCircle( position, radius );
    }
    else
    {
        ci::gl::color( ci::ColorA( color, .7 ) );
        ci::gl::drawStrokedCircle( position, radius );
    }
}
