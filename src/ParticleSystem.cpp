#include "cinder/app/AppBasic.h"
#include "ParticleSystem.h"


ParticleSystem::~ParticleSystem()
{
    clear();
}

void ParticleSystem::clear()
{
    for ( auto it : particles ) {
        delete it;
    }
    particles.clear();
    
    for( std::vector<Spring*>::iterator it = springs.begin(); it != springs.end(); ++it )
    {
        delete * it;
    }
    springs.clear();
}

void ParticleSystem::update()
{
    if ( particles.size() > maxParticles )
        destroyParticle( * particles.begin() );
    
    for ( auto it : particles ) {
        it->borders( this->borders );
        it->update();
        it->flock( particles );
    }
    
    for( auto it : springs ) {
        it->update();
    }
}

void ParticleSystem::draw()
{
    for ( auto particle_first_it : particles )
    {
        for ( auto particle_second_it : particles )
        {
            float distance = particle_first_it->position.distance( particle_second_it->position );
            float per = 1.f - ( distance / 100.f );
            if ( per > 0.f ) {
                ci::Color colorFirst = ci::lerp( particle_first_it->color, particle_second_it->color, per );
                ci::gl::color( ci::ColorA(  colorFirst, per * .8f ) );
                ci::Vec2f conVec = particle_second_it->position - particle_first_it->position;
                conVec.normalize();
                ci::gl::drawLine( particle_first_it->position+conVec * ( particle_first_it->radius+2.f ),
                                  particle_second_it->position-conVec * ( particle_second_it->radius+2.f ));
            }
        }
    }
    
    for( auto particle_it : particles ) particle_it->draw();
    for( auto spring_it : springs )     spring_it->draw();
}

void ParticleSystem::addParticle( Particle *particle )
{
    particles.push_back( particle );
}

void ParticleSystem::destroyParticle( Particle *particle )
{
    std::vector< Particle *>::const_iterator it = std::find( particles.begin(), particles.end(), particle );
    delete * it;
    particles.erase( it );
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

void ParticleSystem::setBorders( const ci::Area &area )
{
    this->borders = area;
}
