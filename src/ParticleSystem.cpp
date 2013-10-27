#include "cinder/app/AppBasic.h"
#include "ParticleSystem.h"


ParticleSystem::~ParticleSystem()
{
    clear();
}

void ParticleSystem::clear()
{
    for ( auto it : particles ){
        delete it;
    }
    particles.clear();
    
    for( auto it : springs ){
        delete it;
    }
    springs.clear();
}

void ParticleSystem::update()
{
    if ( particles.size() > maxParticles )
        destroyParticle( * particles.begin() );
    
    for ( auto particle : particles ){
        particle->borders( true );
        particle->update();
        particle->flock( particles );
    }
    
    for ( auto spring : springs )
        spring->update();
}

void ParticleSystem::draw()
{
    for( auto particle : particles ){
//        for ( auto second : particles ){
//            if ( particle != second ){
//                float distBetweenParticles = particle->position.distance( second->position );
//                float distancePercent = 1.f - ( distBetweenParticles / 100.f );
//                
//                if ( distancePercent > 0.f ){
//                    ci::Color colorFirst = ci::lerp( particle->color, second->color, distancePercent );
//                    ci::gl::color( ci::ColorA(  colorFirst, distancePercent * .8f ) );
//                    ci::Vec2f conVec = particle->position - second->position;
//                    conVec.normalize();
//                    ci::gl::drawLine( particle->position+conVec * ( particle->radius+ .5f ),
//                                      second->position-conVec * ( second->radius+ .5f ));
//                }
//            }
//        }
        particle->draw();
    }
    for( auto spring : springs )     spring->draw();
}

void ParticleSystem::addParticle( Particle *particle )
{
    particles.push_back( particle );
    
    for ( auto second : particles ){
        if ( particle != second ){
            float d = particle->position.distance( second->position );
            float d2 = ( particle->radius + second->radius ) * 100.f;

            if ( d > 0.f && d <= d2 && d < 500.f ){
                Spring * spring = new Spring( particle, second, d * 1.2f, .001f );
                addSpring( spring );
            }
        }
    }
}

void ParticleSystem::destroyParticle( Particle *particle )
{
    std::vector< Particle *>::const_iterator it = std::find( particles.begin(), particles.end(), particle );
    delete * it;
    particles.erase( it );
    
    for ( auto spring : springs )
        if ( spring->particleA == * it || spring->particleB == * it )
            destroySpring( spring );
}

void ParticleSystem::addSpring( Spring *spring )
{
    springs.push_back( spring );
}

void ParticleSystem::destroySpring( Spring *spring )
{
    std::vector<Spring*>::iterator it = std::find( springs.begin(), springs.end(), spring );
    delete *it;
    springs.erase( it );
}
