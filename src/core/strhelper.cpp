/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.
 
    Copyright (c) 2011-2018 by Cao Jiayin - All rights reserved.
 
    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 
    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#include "strhelper.h"
#include "managers/meshmanager.h"
#include <algorithm>
#include "math/transform.h"
#include <stdarg.h>
#include "core/log.h"

// transformation from string
Transform TransformFromStr( const std::string& s )
{
	// if there is no such an value , just return identity matrix and log a warning
	if( s.empty() )
	{
        slog( WARNING , GENERAL , "No value set in the transformation" );
		return Transform();
	}

	std::string str = s;

	// get the first character
	std::string t = NextToken( str , ' ' );
	if( t[0] == 't' )
	{
		t = NextToken( str , ' ' );
		float x = (float)atof( t.c_str() );
		t = NextToken( str , ' ' );
		float y = (float)atof( t.c_str() );
		t = NextToken( str , ' ' );
		float z = (float)atof( t.c_str() );
		return Translate( x , y , z );	
	}else if( t[0] == 'r' )
	{
		t = NextToken( str , ' ' );
		int axis = atoi( t.c_str() );
		t = NextToken( str , ' ' );
		float angle = (float)atof( t.c_str() );
		switch( axis )
		{
			case 0:
				return RotateX( angle );
			case 1:
				return RotateY( angle );
			case 2:
				return RotateZ( angle );
		}
	}else if( t[0] == 's' )
	{
		t = NextToken( str , ' ' );
		float s0 = (float)atof( t.c_str() );
		float s1 = s0;
		float s2 = s0;
		
		t = NextToken( str , ' ' );
		if( str.empty() == false )
		{
			s1 = (float)atof( t.c_str() );
			t = NextToken( str , ' ' );
			s2 = (float)atof( t.c_str() );
		}

		return Scale( s0 , s1 , s2 );
	}else if( t[0] == 'm' )
	{
		Matrix m;
		for( int i = 0 ; i < 16; ++i )
		{
			t = NextToken( str , ' ' );
			m.m[i] = (float)atof( t.c_str() );
		}
		return FromMatrix(m);
	}

	return Transform();
}

// spectrum from string
Spectrum SpectrumFromStr( const std::string& s )
{
	// if the string is empty , return black color
	if( s.empty() )
		return Spectrum();

	std::string str = s;
	std::string r = NextToken( str , ' ' );
	std::string g = NextToken( str , ' ' );
	std::string b = NextToken( str , ' ' );

	float fr = (float)atof( r.c_str() );
	float fg = (float)atof( g.c_str() );
	float fb = (float)atof( b.c_str() );

	return Spectrum( fr , fg , fb );
}

// point from string
Point PointFromStr( const std::string& s )
{
	// if the string is empty , return black color
	if( s.empty() )
		return Point();

	std::string str = s;
	std::string x = NextToken( str , ' ' );
	std::string y = NextToken( str , ' ' );
	std::string z = NextToken( str , ' ' );

	float fx = (float)atof( x.c_str() );
	float fy = (float)atof( y.c_str() );
	float fz = (float)atof( z.c_str() );

	return Point( fx , fy , fz );
}

// vector from string
Vector VectorFromStr( const std::string& s )
{
	if( s.empty() )
		return Vector();
	std::string str = s;
	std::string x = NextToken( str , ' ' );
	std::string y = NextToken( str , ' ' );
	std::string z = NextToken( str , ' ' );

	float fx = (float)atof( x.c_str() );
	float fy = (float)atof( y.c_str() );
	float fz = (float)atof( z.c_str() );

	return Vector( fx , fy , fz ); 
}

// get the next token
std::string NextToken( std::string& str , char t )
{
	// get the next t index
	unsigned id = (int)str.find_first_of( t );
	while( id == 0 )
	{
		// get to the next one
		str = str.substr( 1 , std::string::npos );
		id = (int)str.find_first_of( t );
	}
	std::string res = str.substr( 0 , id );

	// if there is no such a character , set it none
	if( id == (unsigned)std::string::npos )
		str = "";
	else
		str = str.substr( id + 1 , std::string::npos );

	return res;
}