//
//  ParticleCluster.cpp
//  Climax
//
//  Created by Serkan Sökmen on 4.11.2013.
//
//

#include "ParticleCluster.h"

void ParticleCluster::addParticle( Particle * particle )
{
    particles.push_back( particle );
}

void ParticleCluster::removeParticle( Particle * particle )
{
    std::vector< Particle *>::const_iterator it = std::find( particles.begin(), particles.end(), particle );
    delete * it;
    particles.erase( it );
}
