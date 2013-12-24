#pragma once


struct TouchPoint
{
	TouchPoint() {}
    
	TouchPoint( const Vec2f &initialPt, const Color &color ) : mColor( color ), mTimeOfDeath( -1.0 )
	{
		mLine.push_back( initialPt );
	}
    
	void addPoint( const Vec2f &pt )
    {
        mLine.push_back( pt );
    }
    
	void draw() const
	{
		if( mTimeOfDeath > 0 ) // are we dying? then fade out
			gl::color( ColorA( mColor, ( mTimeOfDeath - getElapsedSeconds() ) / 2.0f ) );
		else
			gl::color( mColor );
        
		gl::draw( mLine );
	}
    
	void startDying()
    {
        // two seconds til dead
        mTimeOfDeath = getElapsedSeconds() + 2.0f;
    }
    
	bool isDead() const
    {
        return getElapsedSeconds() > mTimeOfDeath;
    }
    
	PolyLine<Vec2f>	mLine;
	Color			mColor;
	float			mTimeOfDeath;
};
