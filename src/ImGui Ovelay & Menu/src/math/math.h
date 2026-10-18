#pragma once
#include <cmath>
#include <cstdint>

#define M_PI 3.14159265358979323846

inline float deg_to_rad( float degrees ) noexcept
{
	return degrees * static_cast< float >( M_PI ) / 180.f;
}

inline float rad_to_deg( float radians ) noexcept
{
	return radians * ( 180.f / static_cast< float >( M_PI ) );
}

struct vec2_t
{
	double x, y;

	vec2_t( ) : x( 0 ), y( 0 ) { }
	vec2_t( double _x, double _y ) : x( _x ), y( _y ) { }
};

struct vec3_t
{
	double x, y, z;

	constexpr vec3_t( double _x = 0.0, double _y = 0.0, double _z = 0.0 ) noexcept
		: x( _x ), y( _y ), z( _z ) { }

	friend constexpr vec3_t operator-( const vec3_t& a, const vec3_t& b ) noexcept
	{
		return { a.x - b.x, a.y - b.y, a.z - b.z };
	}

	friend constexpr vec3_t operator+( const vec3_t& a, const vec3_t& b ) noexcept
	{
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}

	friend constexpr vec3_t operator*( const vec3_t& v, double f ) noexcept
	{
		return { v.x * f, v.y * f, v.z * f };
	}

	friend constexpr vec3_t operator/( const vec3_t& v, double f ) noexcept
	{
		return { v.x / f, v.y / f, v.z / f };
	}

	constexpr vec3_t& operator+=( const vec3_t& v ) noexcept
	{
		x += v.x; y += v.y; z += v.z;
		return *this;
	}

	constexpr vec3_t& operator-=( const vec3_t& v ) noexcept
	{
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}

	double dot( const vec3_t& v ) const noexcept
	{
		return x * v.x + y * v.y + z * v.z;
	}

	double length( ) const noexcept
	{
		return std::sqrt( x * x + y * y + z * z );
	}

	double distance( const vec3_t& v ) const noexcept
	{
		return ( *this - v ).length( );
	}

	vec3_t normalize( ) const noexcept
	{
		double len = length( );
		if ( len < 1e-8 )
			return vec3_t( 0, 0, 0 );
		return *this / len;
	}

	vec3_t cross( const vec3_t& v ) const noexcept
	{
		return vec3_t(
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x );
	}

	bool is_zero( ) const noexcept
	{
		return x == 0.0 && y == 0.0 && z == 0.0;
	}
};

struct rotator_t
{
	double pitch, yaw, roll;

	constexpr rotator_t( double p = 0.0, double y = 0.0, double r = 0.0 ) noexcept
		: pitch( p ), yaw( y ), roll( r ) { }

	friend constexpr rotator_t operator-( const rotator_t& a, const rotator_t& b ) noexcept
	{
		return { a.pitch - b.pitch, a.yaw - b.yaw, a.roll - b.roll };
	}

	friend constexpr rotator_t operator+( const rotator_t& a, const rotator_t& b ) noexcept
	{
		return { a.pitch + b.pitch, a.yaw + b.yaw, a.roll + b.roll };
	}

	friend constexpr rotator_t operator*( const rotator_t& a, double f ) noexcept
	{
		return { a.pitch * f, a.yaw * f, a.roll * f };
	}

	constexpr rotator_t& normalize( ) noexcept
	{
		pitch = std::isfinite( pitch ) ? std::remainder( pitch, 360.0 ) : 0.0;
		yaw = std::isfinite( yaw ) ? std::remainder( yaw, 360.0 ) : 0.0;
		roll = 0.0;
		return *this;
	}
};

struct matrix4x4_t
{
	float m[ 4 ][ 4 ];
};

inline matrix4x4_t matrix_from_rotation( vec3_t rotation, vec3_t origin = vec3_t( 0, 0, 0 ) ) noexcept
{
	float rad_pitch = static_cast< float >( rotation.x * M_PI / 180.0 );
	float rad_yaw = static_cast< float >( rotation.y * M_PI / 180.0 );
	float rad_roll = static_cast< float >( rotation.z * M_PI / 180.0 );

	float sp = sinf( rad_pitch ), cp = cosf( rad_pitch );
	float sy = sinf( rad_yaw ), cy = cosf( rad_yaw );
	float sr = sinf( rad_roll ), cr = cosf( rad_roll );

	matrix4x4_t mat{ };
	mat.m[ 0 ][ 0 ] = cp * cy;
	mat.m[ 0 ][ 1 ] = cp * sy;
	mat.m[ 0 ][ 2 ] = sp;
	mat.m[ 0 ][ 3 ] = 0.f;
	mat.m[ 1 ][ 0 ] = sr * sp * cy - cr * sy;
	mat.m[ 1 ][ 1 ] = sr * sp * sy + cr * cy;
	mat.m[ 1 ][ 2 ] = -sr * cp;
	mat.m[ 1 ][ 3 ] = 0.f;
	mat.m[ 2 ][ 0 ] = -( cr * sp * cy + sr * sy );
	mat.m[ 2 ][ 1 ] = cy * sr - cr * sp * sy;
	mat.m[ 2 ][ 2 ] = cr * cp;
	mat.m[ 2 ][ 3 ] = 0.f;
	mat.m[ 3 ][ 0 ] = static_cast< float >( origin.x );
	mat.m[ 3 ][ 1 ] = static_cast< float >( origin.y );
	mat.m[ 3 ][ 2 ] = static_cast< float >( origin.z );
	mat.m[ 3 ][ 3 ] = 1.f;
	return mat;
}
