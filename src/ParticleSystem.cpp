#include "cinder/app/AppBasic.h"
#include "ParticleSystem.h"


ParticleSystem::~ParticleSystem()
{
    clear();
}

void ParticleSystem::clear()
{
    for (std::vector< Particle *>::const_iterator it = particles.begin(); it != particles.end(); ++it)
    {
        delete * it;
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
    if ( particles.size() > MAX_PARTICLES )
        destroyParticle( * particles.begin() );
    
    for (std::vector< Particle *>::const_iterator it = particles.begin(); it != particles.end(); ++it)
    {
        Particle * particle = * it;
        particle->borders( this->borders );
        particle->update();
        particle->flock( particles );
    }
    
    for( std::vector<Spring*>::iterator it = springs.begin(); it != springs.end(); ++it )
    {
        ( * it )->update();
    }
}

void ParticleSystem::draw()
{
    std::vector< Particle *>::const_iterator particle_first_it;
    std::vector<Particle*>::const_iterator particle_second_it;
    
    for (particle_first_it = particles.begin(); particle_first_it != particles.end(); ++particle_first_it)
    {
        for( particle_second_it=particles.begin(); particle_second_it!= particles.end(); ++particle_second_it )
        {
            float distance = (*particle_first_it)->position.distance( (*particle_second_it)->position );
            float per = 1.f - ( distance / 100.f );
            if ( per > 0.f ) {
                ci::Color colorFirst = ci::lerp((*particle_first_it)->color, (*particle_second_it)->color, per );
                ci::gl::color( ci::ColorA(  colorFirst, per * .8f ) );
                ci::Vec2f conVec = (*particle_second_it)->position - (*particle_first_it)->position;
                conVec.normalize();
                ci::gl::drawLine((*particle_first_it)->position+conVec * ((*particle_first_it)->radius+2.f),
                                 (*particle_second_it)->position-conVec * ((*particle_second_it)->radius+2.f ));
            }
        }
    }
    
    std::vector<Particle*>::const_iterator particle_it;
    for(particle_it = particles.begin(); particle_it!= particles.end(); ++particle_it){
        (*particle_it)->draw();
    }
    
    for( std::vector<Spring*>::const_iterator spring_it = springs.begin(); spring_it != springs.end(); ++spring_it )
    {
        (*spring_it)->draw();
    }
}

void ParticleSystem::addParticle( Particle *particle )
{
    particles.push_back( particle );
    
//    if ( particles.size() > 2 )
//    {
//        Spring * spring = new Spring( particle, * particles.end(), 1.f, 9.f );
//        addSpring( spring );
//    }
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