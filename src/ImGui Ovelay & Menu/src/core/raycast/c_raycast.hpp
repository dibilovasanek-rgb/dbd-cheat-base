#pragma once
#include <cstdio>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../settings/settings.h"
#include "../../math/math.h"
#include "../../types/types.h"
#include "../../render/render.h"
#include "../../classes/c_skeletal_mesh/c_skeletal_mesh.h"
#include "../../classes/c_fname/c_fname.h"

#include "../../../thirdparty/imgui/imgui.h"

#include <vector>
#include <utility>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

namespace math
{
	struct triangle_t
	{
		vec3_t verts[ 3 ];
	};

	struct edge_t
	{
		vec3_t a, b;
	};

	struct geometry_t
	{
		std::uintptr_t actor = 0;
		std::vector< triangle_t > triangles;
		std::vector< edge_t > edges;
		vec3_t location{ };
		vec3_t bounds_center{ };
		double bounds_radius = 0.0;
	};

	struct ray_t
	{
		vec3_t origin, direction, inverted_direction;
		double distance;
	};

	struct matrix44_t
	{
		double m[ 4 ][ 4 ];
	};

	struct frame_t
	{
		vec3_t origin{ };
		vec3_t axis_x{ 1.0, 0.0, 0.0 };
		vec3_t axis_y{ 0.0, 1.0, 0.0 };
		vec3_t axis_z{ 0.0, 0.0, 1.0 };

		vec3_t to_local( double x, double y, double z ) const noexcept
		{
			return origin + axis_x * x + axis_y * y + axis_z * z;
		}
	};

	inline frame_t frame_from_rotator( const vec3_t& center,
		double pitch, double yaw, double roll ) noexcept
	{
		const double rp = pitch * M_PI / 180.0;
		const double ry = yaw * M_PI / 180.0;
		const double rr = roll * M_PI / 180.0;

		const double sp = std::sin( rp ), cp = std::cos( rp );
		const double sy = std::sin( ry ), cy = std::cos( ry );
		const double sr = std::sin( rr ), cr = std::cos( rr );

		frame_t frame;
		frame.origin = center;
		frame.axis_x = vec3_t( cp * cy, cp * sy, sp );
		frame.axis_y = vec3_t( sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp );
		frame.axis_z = vec3_t( -( cr * sp * cy + sr * sy ), cy * sr - cr * sp * sy, cr * cp );
		return frame;
	}
}

enum class component_kind : std::uint8_t
{
	none = 0,
	static_mesh,
	instanced_mesh,
	box_shape,
	capsule_shape,
	sphere_shape
};

struct component_ref_t
{
	std::uintptr_t address = 0;
	component_kind kind = component_kind::none;
};

namespace
{
	constexpr int    max_levels           = 64;
	constexpr int    max_actors_per_level = 20000;
	constexpr int    max_components       = 40000;
	constexpr int    max_triangles        = 20000;
	constexpr int    max_geometry         = 60000;
	constexpr int    calibration_samples  = 64;
	constexpr double max_draw_distance    = 8000.0;
	constexpr int    rebuild_interval_ms  = 1000;
	constexpr int    worker_poll_ms       = 50;

	constexpr int    max_instances        = 4096;
	constexpr int    max_elems            = 64;
	constexpr double min_wall_size        = 8.0;
}

class c_raycast
{
public:
	static c_raycast* get( ) noexcept
	{
		static c_raycast instance;
		return &instance;
	}

	std::size_t geometry_count( ) const noexcept
	{
		std::lock_guard< std::mutex > lock( m_mutex );
		return m_geometry.size( );
	}

	std::size_t triangle_count( ) const noexcept
	{
		std::lock_guard< std::mutex > lock( m_mutex );
		return m_triangle_count;
	}

	void start_worker( ) noexcept
	{
		if ( m_worker.joinable( ) )
			return;

		m_worker_running = true;
		m_worker = std::thread( [ this ]( ) noexcept { worker_loop( ); } );
	}

	void stop_worker( ) noexcept
	{
		m_worker_running = false;
		if ( m_worker.joinable( ) )
			m_worker.join( );
	}

	void invalidate_cache( ) noexcept { m_force_rebuild = true; }

	void rebuild_cache( ) noexcept
	{
		const bool verbose = !m_built_once;

		const std::uintptr_t component_to_world = offsets::scene_component::component_to_world;

		const uint64_t world_ptr =
			g_memory->read< uint64_t >( g_memory->get_base_address( ) + offsets::g_world );

		if ( !world_ptr )
		{
			if ( verbose )
				printf( "[raycast] GWorld is null - offsets::g_world is stale\n" );
			return;
		}

		std::vector< component_ref_t > components;
		std::vector< std::string > names;
		components.reserve( 8192 );

		if ( verbose )
			names.reserve( 16384 );

		collect_components( world_ptr, components, verbose ? &names : nullptr );

		if ( verbose )
		{
			print_class_histogram( names );
			printf( "[raycast] %zu collidable components\n", components.size( ) );
		}

		if ( components.empty( ) )
		{
			if ( verbose )
				printf( "[raycast] nothing found - check offsets::world::levels / level::actors\n" );
			return;
		}

		if ( verbose )
			verify_component_to_world( components );

		std::vector< math::geometry_t > geometry_out;
		geometry_out.reserve( components.size( ) );
		std::size_t triangles_out = 0;

		for ( const component_ref_t& ref : components )
		{
			if ( geometry_out.size( ) >= max_geometry )
				break;

			build_geometry( ref, component_to_world, geometry_out, triangles_out );
		}

		{
			std::lock_guard< std::mutex > lock( m_mutex );
			m_geometry.swap( geometry_out );
			m_triangle_count = triangles_out;
			m_component_to_world = component_to_world;
		}

		if ( verbose )
		{
			printf( "[raycast] cache: %zu solids / %zu triangles\n",
				geometry_count( ), triangles_out );
		}

		m_built_once = true;
	}

	void render_geometry( int screen_width, int screen_height, const camera_info_t& cam_info ) const noexcept
	{
		( void )screen_width;
		( void )screen_height;

		const render::camera_basis_t basis = render::make_basis( cam_info, max_draw_distance );
		auto draw_list = ImGui::GetBackgroundDrawList( );

		std::size_t drawn_actors = 0;
		std::size_t drawn_edges = 0;

		{
			std::lock_guard< std::mutex > lock( m_mutex );

			for ( const auto& geometry : m_geometry )
			{
				if ( !render::is_sphere_visible( basis, geometry.bounds_center, geometry.bounds_radius ) )
					continue;

				drawn_actors++;
				drawn_edges += draw_wireframe( basis, geometry.edges, draw_list );
			}
		}

		m_drawn_actors = drawn_actors;
		m_drawn_edges = drawn_edges;
	}

	std::size_t last_drawn_actors( ) const noexcept { return m_drawn_actors; }
	std::size_t last_drawn_edges( ) const noexcept { return m_drawn_edges; }

	std::uintptr_t line_trace( const vec3_t& origin, const vec3_t& position ) const noexcept
	{
		constexpr double epsilon = 1e-8;
		vec3_t delta = position - origin;
		double distance = delta.length( );

		if ( distance < epsilon || position.is_zero( ) )
			return 0;

		vec3_t direction = delta / distance;

		vec3_t inverted_direction(
			std::abs( direction.x ) > epsilon ? 1.0 / direction.x : epsilon,
			std::abs( direction.y ) > epsilon ? 1.0 / direction.y : epsilon,
			std::abs( direction.z ) > epsilon ? 1.0 / direction.z : epsilon );

		math::ray_t ray{ origin, direction, inverted_direction, distance };

		std::lock_guard< std::mutex > lock( m_mutex );

		for ( const auto& geometry : m_geometry )
		{
			if ( !segment_hits_sphere( ray, geometry.bounds_center, geometry.bounds_radius ) )
				continue;

			for ( const auto& triangle : geometry.triangles )
			{
				if ( intersect_triangle( ray, triangle ) )
					return geometry.actor;
			}
		}

		return 0;
	}

	bool is_visible( const vec3_t& origin, const vec3_t& position ) const noexcept
	{
		return line_trace( origin, position ) == 0;
	}

	static bool cache_wanted( ) noexcept
	{
		return g_settings->map_wireframe || g_settings->visible_check;
	}

private:
	c_raycast( ) = default;

	~c_raycast( )
	{
		stop_worker( );
	}

	void worker_loop( ) noexcept
	{
		bool was_enabled = false;

		while ( m_worker_running )
		{
			const bool enabled = cache_wanted( );

			if ( !enabled )
			{
				if ( was_enabled )
				{
					std::lock_guard< std::mutex > lock( m_mutex );
					m_geometry.clear( );
					m_triangle_count = 0;
				}

				was_enabled = false;
				std::this_thread::sleep_for( std::chrono::milliseconds( worker_poll_ms ) );
				continue;
			}

			m_force_rebuild = false;
			was_enabled = true;

			rebuild_cache( );

			for ( int waited = 0; waited < rebuild_interval_ms && m_worker_running; waited += worker_poll_ms )
			{
				if ( !cache_wanted( ) || m_force_rebuild )
					break;

				std::this_thread::sleep_for( std::chrono::milliseconds( worker_poll_ms ) );
			}
		}
	}

	void collect_components( uint64_t world_ptr, std::vector< component_ref_t >& out,
		std::vector< std::string >* names ) const noexcept
	{
		const array_t levels = g_memory->read< array_t >( world_ptr + offsets::world::levels );
		if ( !levels.data || levels.count <= 0 || levels.count > max_levels )
			return;

		for ( int l = 0; l < levels.count && static_cast< int >( out.size( ) ) < max_components; ++l )
		{
			const uint64_t level = g_memory->read< uint64_t >( levels.data + static_cast< uint64_t >( l ) * 8 );
			if ( !level )
				continue;

			const array_t actors = g_memory->read< array_t >( level + offsets::level::actors );
			if ( !actors.data || actors.count <= 0 || actors.count > max_actors_per_level )
				continue;

			for ( int a = 0; a < actors.count && static_cast< int >( out.size( ) ) < max_components; ++a )
			{
				const uint64_t actor =
					g_memory->read< uint64_t >( actors.data + static_cast< uint64_t >( a ) * 8 );
				if ( !actor )
					continue;

				const uint64_t root =
					g_memory->read< uint64_t >( actor + offsets::actor::root_component );
				consider_component( root, out, names );

				collect_from_array( actor + offsets::actor::instance_components, out, names );
				collect_from_array( actor + offsets::actor::blueprint_components, out, names );
			}
		}
	}

	void collect_from_array( uint64_t array_addr, std::vector< component_ref_t >& out,
		std::vector< std::string >* names ) const noexcept
	{
		const array_t arr = g_memory->read< array_t >( array_addr );
		if ( !arr.data || arr.count <= 0 || arr.count > 256 )
			return;

		for ( int i = 0; i < arr.count && static_cast< int >( out.size( ) ) < max_components; ++i )
		{
			const uint64_t component =
				g_memory->read< uint64_t >( arr.data + static_cast< uint64_t >( i ) * 8 );
			consider_component( component, out, names );
		}
	}

	void consider_component( uint64_t component, std::vector< component_ref_t >& out,
		std::vector< std::string >* names ) const noexcept
	{
		if ( !component )
			return;

		const uint64_t klass = g_memory->read< uint64_t >( component + offsets::object::class_private );
		if ( !klass )
			return;

		if ( names )
		{
			const std::string name = class_name_of( klass );
			if ( !name.empty( ) )
				names->push_back( name );
		}

		const component_kind kind = classify( klass );
		if ( kind == component_kind::none )
			return;

		if ( !has_blocking_collision( component ) )
			return;

		out.push_back( component_ref_t{ static_cast< std::uintptr_t >( component ), kind } );
	}

	component_kind classify( uint64_t klass ) const noexcept
	{
		{
			const auto it = m_class_kinds.find( klass );
			if ( it != m_class_kinds.end( ) )
				return it->second;
		}

		component_kind kind = component_kind::none;
		uint64_t current = klass;

		for ( int depth = 0; depth < 16 && current; ++depth )
		{
			const std::string name = class_name_of( current );

			if ( name == "StaticMeshComponent" )
			{
				kind = component_kind::static_mesh;
				break;
			}
			if ( name == "InstancedStaticMeshComponent" ||
				 name == "HierarchicalInstancedStaticMeshComponent" )
			{
				kind = component_kind::instanced_mesh;
				break;
			}
			if ( name == "BoxComponent" )
			{
				kind = component_kind::box_shape;
				break;
			}
			if ( name == "CapsuleComponent" )
			{
				kind = component_kind::capsule_shape;
				break;
			}
			if ( name == "SphereComponent" )
			{
				kind = component_kind::sphere_shape;
				break;
			}

			if ( name == "SceneComponent" || name == "ActorComponent" || name == "Object" )
				break;

			current = g_memory->read< uint64_t >( current + offsets::ustruct::super_struct );
		}

		m_class_kinds.emplace( klass, kind );
		return kind;
	}

	static bool has_blocking_collision( uint64_t component ) noexcept
	{
		const uint8_t enabled =
			g_memory->read< uint8_t >( component + offsets::primitive_component::collision_enabled );

		if ( enabled == offsets::collision::enabled_none || enabled > 5 )
			return false;

		const uint8_t response = g_memory->read< uint8_t >(
			component + offsets::primitive_component::response_to_channels +
			offsets::collision::channel_visibility );

		return response == offsets::collision::response_block;
	}

	static std::string class_name_of( uint64_t klass ) noexcept
	{
		if ( !klass )
			return std::string( );

		return fname::get_name_by_id( g_memory->read< uint32_t >( klass + offsets::object::name_id ) );
	}

	static void print_class_histogram( const std::vector< std::string >& names ) noexcept
	{
		if ( names.empty( ) )
		{
			printf( "[raycast] no components visited - levels/actors arrays are empty\n" );
			return;
		}

		std::unordered_map< std::string, std::size_t > counts;
		counts.reserve( names.size( ) );

		for ( const auto& name : names )
			counts[ name ]++;

		std::vector< std::pair< std::string, std::size_t > > sorted( counts.begin( ), counts.end( ) );
		std::sort( sorted.begin( ), sorted.end( ),
			[ ]( const auto& a, const auto& b ) noexcept { return a.second > b.second; } );

		printf( "[raycast] %zu components, %zu distinct classes (top 15):\n",
			names.size( ), sorted.size( ) );

		const std::size_t limit = sorted.size( ) < 15 ? sorted.size( ) : 15;
		for ( std::size_t i = 0; i < limit; ++i )
			printf( "[raycast]   %6zu x %s\n", sorted[ i ].second, sorted[ i ].first.c_str( ) );
	}

	static std::string get_class_name( uint64_t object ) noexcept
	{
		if ( !object )
			return std::string( );

		return class_name_of(
			g_memory->read< uint64_t >( object + offsets::object::class_private ) );
	}

	void verify_component_to_world( const std::vector< component_ref_t >& components ) const noexcept
	{
		static constexpr std::uintptr_t candidates[ ] =
		{
			offsets::scene_component::component_to_world,
			0x1F8, 0x200, 0x1E8, 0x208, 0x210, 0x218, 0x220, 0x228, 0x238, 0x240
		};

		int configured_score = 0;
		std::uintptr_t best = candidates[ 0 ];
		int best_score = 0;
		int samples = 0;

		for ( std::uintptr_t candidate : candidates )
		{
			int score = 0;
			int tested = 0;

			for ( const component_ref_t& ref : components )
			{
				if ( tested++ >= calibration_samples )
					break;

				const transform_t transform =
					g_memory->read< transform_t >( ref.address + candidate );
				if ( !is_sane_transform( transform, 0.9, 1.1 ) )
					continue;

				score++;
			}

			samples = tested;
			if ( candidate == offsets::scene_component::component_to_world )
				configured_score = score;

			if ( score > best_score )
			{
				best_score = score;
				best = candidate;
			}
		}

		printf( "[raycast] ComponentToWorld 0x%llX -> %d/%d samples are valid transforms\n",
			static_cast< unsigned long long >( offsets::scene_component::component_to_world ),
			configured_score, samples );

		if ( best != offsets::scene_component::component_to_world && best_score > configured_score )
		{
			printf( "[raycast] WARNING: 0x%llX scores better (%d/%d) - "
				"consider changing offsets::scene_component::component_to_world\n",
				static_cast< unsigned long long >( best ), best_score, samples );
		}
	}

	static bool is_sane_transform( const transform_t& t, double quat_min, double quat_max ) noexcept
	{
		if ( std::isnan( t.translation.x ) || std::isnan( t.translation.y ) || std::isnan( t.translation.z ) )
			return false;
		if ( std::isnan( t.scale.x ) || std::isnan( t.scale.y ) || std::isnan( t.scale.z ) )
			return false;
		if ( std::isinf( t.translation.x ) || std::isinf( t.translation.y ) || std::isinf( t.translation.z ) )
			return false;

		const double quat_len =
			t.rot[ 0 ] * t.rot[ 0 ] + t.rot[ 1 ] * t.rot[ 1 ] +
			t.rot[ 2 ] * t.rot[ 2 ] + t.rot[ 3 ] * t.rot[ 3 ];
		if ( quat_len < quat_min || quat_len > quat_max )
			return false;

		if ( std::abs( t.translation.x ) > 1e7 ||
			 std::abs( t.translation.y ) > 1e7 ||
			 std::abs( t.translation.z ) > 1e7 )
			return false;

		if ( t.scale.x <= 0.0 || t.scale.x > 1000.0 ||
			 t.scale.y <= 0.0 || t.scale.y > 1000.0 ||
			 t.scale.z <= 0.0 || t.scale.z > 1000.0 )
			return false;

		return true;
	}

	vec3_t transform_position( const transform_t& t, const vec3_t& v ) const noexcept
	{
		return apply_transform( t, v );
	}

	void build_geometry( const component_ref_t& ref, std::uintptr_t component_to_world_offset,
		std::vector< math::geometry_t >& out, std::size_t& triangles_out ) const noexcept
	{
		const std::uintptr_t component = ref.address;

		const transform_t component_to_world =
			g_memory->read< transform_t >( component + component_to_world_offset );

		switch ( ref.kind )
		{
		case component_kind::static_mesh:
		{
			if ( !is_sane_transform( component_to_world, 0.5, 1.5 ) )
				return;

			const uint64_t body_setup = static_mesh_body_setup( component );
			if ( !body_setup )
				return;

			math::geometry_t geometry;
			geometry.actor = component;
			geometry.location = component_to_world.translation;

			bounds_accumulator_t bounds;
			emit_agg_geom( body_setup, component_to_world, geometry, bounds );
			finalize( geometry, bounds, out, triangles_out );
			return;
		}

		case component_kind::instanced_mesh:
		{
			const uint64_t body_setup = static_mesh_body_setup( component );
			if ( !body_setup )
				return;

			emit_instances( component, body_setup, component_to_world_offset, out, triangles_out );
			return;
		}

		case component_kind::box_shape:
		{
			if ( !is_sane_transform( component_to_world, 0.5, 1.5 ) )
				return;

			const vec3_t extent =
				g_memory->read< vec3_t >( component + offsets::box_component::box_extent );

			if ( !is_sane_extent( extent ) )
				return;

			math::geometry_t geometry;
			geometry.actor = component;
			geometry.location = component_to_world.translation;

			bounds_accumulator_t bounds;
			emit_box( math::frame_t{ }, extent.x, extent.y, extent.z,
				component_to_world, geometry, bounds );
			finalize( geometry, bounds, out, triangles_out );
			return;
		}

		case component_kind::capsule_shape:
		case component_kind::sphere_shape:
		{
			return;
		}

		case component_kind::none:
		default:
			return;
		}
	}

	static uint64_t static_mesh_body_setup( std::uintptr_t component ) noexcept
	{
		const uint64_t mesh =
			g_memory->read< uint64_t >( component + offsets::static_mesh_component::static_mesh );
		if ( !mesh )
			return 0;

		return g_memory->read< uint64_t >( mesh + offsets::static_mesh::body_setup );
	}

	struct bounds_accumulator_t
	{
		vec3_t min{ 1e9, 1e9, 1e9 };
		vec3_t max{ -1e9, -1e9, -1e9 };

		void expand( const vec3_t& v ) noexcept
		{
			min.x = ( std::min )( min.x, v.x );
			min.y = ( std::min )( min.y, v.y );
			min.z = ( std::min )( min.z, v.z );
			max.x = ( std::max )( max.x, v.x );
			max.y = ( std::max )( max.y, v.y );
			max.z = ( std::max )( max.z, v.z );
		}
	};

	static void finalize( math::geometry_t& geometry, const bounds_accumulator_t& bounds,
		std::vector< math::geometry_t >& out, std::size_t& triangles_out ) noexcept
	{
		if ( geometry.triangles.empty( ) )
			return;

		geometry.bounds_center = vec3_t(
			( bounds.min.x + bounds.max.x ) * 0.5,
			( bounds.min.y + bounds.max.y ) * 0.5,
			( bounds.min.z + bounds.max.z ) * 0.5 );
		geometry.bounds_radius = ( bounds.max - bounds.min ).length( ) * 0.5;

		triangles_out += geometry.triangles.size( );
		out.emplace_back( std::move( geometry ) );
	}

	static bool is_sane_extent( const vec3_t& extent ) noexcept
	{
		if ( std::isnan( extent.x ) || std::isnan( extent.y ) || std::isnan( extent.z ) )
			return false;

		const double largest = ( std::max )( ( std::max )( extent.x, extent.y ), extent.z );
		return largest > min_wall_size * 0.5 && largest < 1e5;
	}

	void emit_agg_geom( uint64_t body_setup, const transform_t& to_world,
		math::geometry_t& geometry, bounds_accumulator_t& bounds ) const noexcept
	{
		const uint64_t agg = body_setup + offsets::body_setup::agg_geom;

		emit_convex_elems( g_memory->read< array_t >( agg + offsets::agg_geom::convex_elems ),
			to_world, geometry, bounds );

		emit_box_elems( g_memory->read< array_t >( agg + offsets::agg_geom::box_elems ),
			to_world, geometry, bounds );

		emit_sphyl_elems( g_memory->read< array_t >( agg + offsets::agg_geom::sphyl_elems ),
			to_world, geometry, bounds );
	}

	void emit_convex_elems( const array_t& elems, const transform_t& to_world,
		math::geometry_t& geometry, bounds_accumulator_t& bounds ) const noexcept
	{
		if ( !elems.data || elems.count <= 0 || elems.count > max_elems )
			return;

		for ( int e = 0; e < elems.count; ++e )
		{
			const uint64_t elem =
				elems.data + static_cast< uint64_t >( e ) * offsets::convex_elem::size;

			const array_t vertex_data =
				g_memory->read< array_t >( elem + offsets::convex_elem::vertex_data );
			const array_t index_data =
				g_memory->read< array_t >( elem + offsets::convex_elem::index_data );

			if ( !vertex_data.data || vertex_data.count <= 0 || vertex_data.count > 4096 )
				continue;
			if ( !index_data.data || index_data.count <= 0 || index_data.count > 16384 )
				continue;

			std::vector< vec3_t > vertices( static_cast< std::size_t >( vertex_data.count ) );
			g_memory->read_raw( vertex_data.data, vertices.data( ), vertices.size( ) * sizeof( vec3_t ) );

			std::vector< int32_t > indices( static_cast< std::size_t >( index_data.count ) );
			g_memory->read_raw( index_data.data, indices.data( ), indices.size( ) * sizeof( int32_t ) );

			if ( static_cast< int >( geometry.triangles.size( ) ) + index_data.count / 3 > max_triangles )
				break;

			std::vector< std::pair< int, int > > unique_edges;
			unique_edges.reserve( static_cast< std::size_t >( index_data.count ) );

			for ( std::size_t i = 0; i + 2 < indices.size( ); i += 3 )
			{
				const int i0 = indices[ i ];
				const int i1 = indices[ i + 1 ];
				const int i2 = indices[ i + 2 ];

				if ( i0 < 0 || i1 < 0 || i2 < 0 )
					continue;
				if ( static_cast< std::size_t >( i0 ) >= vertices.size( ) ||
					 static_cast< std::size_t >( i1 ) >= vertices.size( ) ||
					 static_cast< std::size_t >( i2 ) >= vertices.size( ) )
					continue;
				if ( vertices[ i0 ].is_zero( ) || vertices[ i1 ].is_zero( ) || vertices[ i2 ].is_zero( ) )
					continue;

				math::triangle_t triangle;
				triangle.verts[ 0 ] = transform_position( to_world, vertices[ i0 ] );
				triangle.verts[ 1 ] = transform_position( to_world, vertices[ i1 ] );
				triangle.verts[ 2 ] = transform_position( to_world, vertices[ i2 ] );
				geometry.triangles.emplace_back( triangle );

				bounds.expand( triangle.verts[ 0 ] );
				bounds.expand( triangle.verts[ 1 ] );
				bounds.expand( triangle.verts[ 2 ] );

				add_edge( unique_edges, i0, i1 );
				add_edge( unique_edges, i1, i2 );
				add_edge( unique_edges, i2, i0 );
			}

			for ( const auto& pair : unique_edges )
			{
				geometry.edges.push_back( math::edge_t{
					transform_position( to_world, vertices[ pair.first ] ),
					transform_position( to_world, vertices[ pair.second ] ) } );
			}
		}
	}

	void emit_box_elems( const array_t& elems, const transform_t& to_world,
		math::geometry_t& geometry, bounds_accumulator_t& bounds ) const noexcept
	{
		if ( !elems.data || elems.count <= 0 || elems.count > max_elems )
			return;

		for ( int e = 0; e < elems.count; ++e )
		{
			const uint64_t elem =
				elems.data + static_cast< uint64_t >( e ) * offsets::box_elem::size;

			const vec3_t center = g_memory->read< vec3_t >( elem + offsets::box_elem::center );
			const vec3_t rotator = g_memory->read< vec3_t >( elem + offsets::box_elem::rotation );

			const double half_x = g_memory->read< float >( elem + offsets::box_elem::extent_x ) * 0.5;
			const double half_y = g_memory->read< float >( elem + offsets::box_elem::extent_y ) * 0.5;
			const double half_z = g_memory->read< float >( elem + offsets::box_elem::extent_z ) * 0.5;

			if ( !is_sane_extent( vec3_t( half_x, half_y, half_z ) ) )
				continue;

			const math::frame_t frame =
				math::frame_from_rotator( center, rotator.x, rotator.y, rotator.z );

			emit_box( frame, half_x, half_y, half_z, to_world, geometry, bounds );
		}
	}

	void emit_sphyl_elems( const array_t& elems, const transform_t& to_world,
		math::geometry_t& geometry, bounds_accumulator_t& bounds ) const noexcept
	{
		if ( !elems.data || elems.count <= 0 || elems.count > max_elems )
			return;

		for ( int e = 0; e < elems.count; ++e )
		{
			const uint64_t elem =
				elems.data + static_cast< uint64_t >( e ) * offsets::sphyl_elem::size;

			const vec3_t center = g_memory->read< vec3_t >( elem + offsets::sphyl_elem::center );
			const vec3_t rotator = g_memory->read< vec3_t >( elem + offsets::sphyl_elem::rotation );
			const double radius = g_memory->read< float >( elem + offsets::sphyl_elem::radius );
			const double length = g_memory->read< float >( elem + offsets::sphyl_elem::length );

			if ( radius < min_wall_size * 0.5 || radius > 1e5 )
				continue;

			const double half_height = length * 0.5 + radius;

			const math::frame_t frame =
				math::frame_from_rotator( center, rotator.x, rotator.y, rotator.z );

			emit_box( frame, radius, radius, half_height, to_world, geometry, bounds );
		}
	}

	void emit_box( const math::frame_t& frame, double half_x, double half_y, double half_z,
		const transform_t& to_world, math::geometry_t& geometry,
		bounds_accumulator_t& bounds ) const noexcept
	{
		if ( static_cast< int >( geometry.triangles.size( ) ) + 12 > max_triangles )
			return;

		static constexpr double sign[ 8 ][ 3 ] =
		{
			{ -1, -1, -1 }, { +1, -1, -1 }, { +1, +1, -1 }, { -1, +1, -1 },
			{ -1, -1, +1 }, { +1, -1, +1 }, { +1, +1, +1 }, { -1, +1, +1 }
		};

		vec3_t corners[ 8 ];
		for ( int i = 0; i < 8; ++i )
		{
			const vec3_t local = frame.to_local(
				sign[ i ][ 0 ] * half_x,
				sign[ i ][ 1 ] * half_y,
				sign[ i ][ 2 ] * half_z );

			corners[ i ] = transform_position( to_world, local );
			bounds.expand( corners[ i ] );
		}

		static constexpr int faces[ 12 ][ 3 ] =
		{
			{ 0, 1, 2 }, { 0, 2, 3 },
			{ 4, 6, 5 }, { 4, 7, 6 },
			{ 0, 5, 1 }, { 0, 4, 5 },
			{ 3, 2, 6 }, { 3, 6, 7 },
			{ 0, 3, 7 }, { 0, 7, 4 },
			{ 1, 5, 6 }, { 1, 6, 2 }
		};

		for ( const auto& face : faces )
		{
			math::triangle_t triangle;
			triangle.verts[ 0 ] = corners[ face[ 0 ] ];
			triangle.verts[ 1 ] = corners[ face[ 1 ] ];
			triangle.verts[ 2 ] = corners[ face[ 2 ] ];
			geometry.triangles.emplace_back( triangle );
		}

		static constexpr int wire[ 12 ][ 2 ] =
		{
			{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
			{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
		};

		for ( const auto& pair : wire )
			geometry.edges.push_back( math::edge_t{ corners[ pair[ 0 ] ], corners[ pair[ 1 ] ] } );
	}

	void emit_instances( std::uintptr_t component, uint64_t body_setup,
		std::uintptr_t component_to_world_offset,
		std::vector< math::geometry_t >& out, std::size_t& triangles_out ) const noexcept
	{
		const transform_t component_to_world = g_memory->read< transform_t >(
			component + component_to_world_offset );
		if ( !is_sane_transform( component_to_world, 0.5, 1.5 ) )
			return;

		const array_t instances = g_memory->read< array_t >(
			component + offsets::instanced_static_mesh_component::per_instance_sm_data );

		if ( !instances.data || instances.count <= 0 || instances.count > max_instances )
			return;

		for ( int i = 0; i < instances.count; ++i )
		{
			if ( out.size( ) >= max_geometry )
				return;

			const uint64_t entry = instances.data +
				static_cast< uint64_t >( i ) *
				offsets::instanced_static_mesh_component::instance_stride;

			const math::matrix44_t local = g_memory->read< math::matrix44_t >( entry );

			transform_t to_world;
			if ( !compose_instance_transform( local, component_to_world, to_world ) )
				continue;

			math::geometry_t geometry;
			geometry.actor = component;
			geometry.location = to_world.translation;

			bounds_accumulator_t bounds;
			emit_agg_geom( body_setup, to_world, geometry, bounds );
			finalize( geometry, bounds, out, triangles_out );
		}
	}

	static bool compose_instance_transform( const math::matrix44_t& local,
		const transform_t& component_to_world, transform_t& out ) noexcept
	{
		vec3_t axis_x( local.m[ 0 ][ 0 ], local.m[ 0 ][ 1 ], local.m[ 0 ][ 2 ] );
		vec3_t axis_y( local.m[ 1 ][ 0 ], local.m[ 1 ][ 1 ], local.m[ 1 ][ 2 ] );
		vec3_t axis_z( local.m[ 2 ][ 0 ], local.m[ 2 ][ 1 ], local.m[ 2 ][ 2 ] );
		const vec3_t translation( local.m[ 3 ][ 0 ], local.m[ 3 ][ 1 ], local.m[ 3 ][ 2 ] );

		const double scale_x = axis_x.length( );
		const double scale_y = axis_y.length( );
		const double scale_z = axis_z.length( );

		if ( scale_x < 1e-6 || scale_y < 1e-6 || scale_z < 1e-6 )
			return false;
		if ( scale_x > 1e4 || scale_y > 1e4 || scale_z > 1e4 )
			return false;
		if ( std::isnan( translation.x ) || std::isnan( translation.y ) || std::isnan( translation.z ) )
			return false;
		if ( std::abs( translation.x ) > 1e7 ||
			 std::abs( translation.y ) > 1e7 ||
			 std::abs( translation.z ) > 1e7 )
			return false;

		axis_x = axis_x / scale_x;
		axis_y = axis_y / scale_y;
		axis_z = axis_z / scale_z;

		const double trace = axis_x.x + axis_y.y + axis_z.z;
		double qx, qy, qz, qw;

		if ( trace > 0.0 )
		{
			const double s = std::sqrt( trace + 1.0 ) * 2.0;
			qw = 0.25 * s;
			qx = ( axis_y.z - axis_z.y ) / s;
			qy = ( axis_z.x - axis_x.z ) / s;
			qz = ( axis_x.y - axis_y.x ) / s;
		}
		else if ( axis_x.x > axis_y.y && axis_x.x > axis_z.z )
		{
			const double s = std::sqrt( 1.0 + axis_x.x - axis_y.y - axis_z.z ) * 2.0;
			qw = ( axis_y.z - axis_z.y ) / s;
			qx = 0.25 * s;
			qy = ( axis_y.x + axis_x.y ) / s;
			qz = ( axis_z.x + axis_x.z ) / s;
		}
		else if ( axis_y.y > axis_z.z )
		{
			const double s = std::sqrt( 1.0 + axis_y.y - axis_x.x - axis_z.z ) * 2.0;
			qw = ( axis_z.x - axis_x.z ) / s;
			qx = ( axis_y.x + axis_x.y ) / s;
			qy = 0.25 * s;
			qz = ( axis_z.y + axis_y.z ) / s;
		}
		else
		{
			const double s = std::sqrt( 1.0 + axis_z.z - axis_x.x - axis_y.y ) * 2.0;
			qw = ( axis_x.y - axis_y.x ) / s;
			qx = ( axis_z.x + axis_x.z ) / s;
			qy = ( axis_z.y + axis_y.z ) / s;
			qz = 0.25 * s;
		}

		transform_t instance{ };
		instance.rot[ 0 ] = qx;
		instance.rot[ 1 ] = qy;
		instance.rot[ 2 ] = qz;
		instance.rot[ 3 ] = qw;
		instance.translation = translation;
		instance.scale = vec3_t( scale_x, scale_y, scale_z );

		out.translation = apply_transform( component_to_world, instance.translation );
		out.scale = vec3_t(
			component_to_world.scale.x * scale_x,
			component_to_world.scale.y * scale_y,
			component_to_world.scale.z * scale_z );

		quat_multiply( component_to_world.rot, instance.rot, out.rot );
		return true;
	}

	static void quat_multiply( const double a[ 4 ], const double b[ 4 ], double out[ 4 ] ) noexcept
	{
		const double ax = a[ 0 ], ay = a[ 1 ], az = a[ 2 ], aw = a[ 3 ];
		const double bx = b[ 0 ], by = b[ 1 ], bz = b[ 2 ], bw = b[ 3 ];

		out[ 0 ] = aw * bx + ax * bw + ay * bz - az * by;
		out[ 1 ] = aw * by - ax * bz + ay * bw + az * bx;
		out[ 2 ] = aw * bz + ax * by - ay * bx + az * bw;
		out[ 3 ] = aw * bw - ax * bx - ay * by - az * bz;
	}

	static vec3_t apply_transform( const transform_t& t, const vec3_t& v ) noexcept
	{
		const double cx = t.rot[ 0 ], cy = t.rot[ 1 ], cz = t.rot[ 2 ], cw = t.rot[ 3 ];

		const double sx = v.x * t.scale.x;
		const double sy = v.y * t.scale.y;
		const double sz = v.z * t.scale.z;

		const double rx = sx * ( 1.0 - 2.0 * ( cy * cy + cz * cz ) ) +
						  sy * 2.0 * ( cx * cy - cz * cw ) +
						  sz * 2.0 * ( cx * cz + cy * cw );

		const double ry = sx * 2.0 * ( cx * cy + cz * cw ) +
						  sy * ( 1.0 - 2.0 * ( cx * cx + cz * cz ) ) +
						  sz * 2.0 * ( cy * cz - cx * cw );

		const double rz = sx * 2.0 * ( cx * cz - cy * cw ) +
						  sy * 2.0 * ( cy * cz + cx * cw ) +
						  sz * ( 1.0 - 2.0 * ( cx * cx + cy * cy ) );

		return vec3_t(
			rx + t.translation.x,
			ry + t.translation.y,
			rz + t.translation.z );
	}

	static void add_edge( std::vector< std::pair< int, int > >& edges, int a, int b ) noexcept
	{
		if ( a == b )
			return;

		const std::pair< int, int > key =
			( a < b ) ? std::pair< int, int >{ a, b } : std::pair< int, int >{ b, a };

		if ( std::find( edges.begin( ), edges.end( ), key ) == edges.end( ) )
			edges.push_back( key );
	}

	std::size_t draw_wireframe( const render::camera_basis_t& basis,
		const std::vector< math::edge_t >& edges, ImDrawList* draw_list ) const noexcept
	{
		if ( edges.empty( ) )
			return 0;

		const ImColor wire_color( 255, 255, 255, 60 );
		std::size_t drawn = 0;

		for ( const auto& edge : edges )
		{
			vec2_t screen_a, screen_b;

			if ( !render::world_to_screen( basis, edge.a, screen_a ) )
				continue;
			if ( !render::world_to_screen( basis, edge.b, screen_b ) )
				continue;

			draw_list->AddLine(
				ImVec2( static_cast< float >( screen_a.x ), static_cast< float >( screen_a.y ) ),
				ImVec2( static_cast< float >( screen_b.x ), static_cast< float >( screen_b.y ) ),
				wire_color, 1.0f );
			drawn++;
		}

		return drawn;
	}

	static bool segment_hits_sphere( const math::ray_t& ray, const vec3_t& center, double radius ) noexcept
	{
		const vec3_t to_center = center - ray.origin;
		double along = to_center.dot( ray.direction );

		if ( along < 0.0 )
			along = 0.0;
		else if ( along > ray.distance )
			along = ray.distance;

		const vec3_t closest = ray.origin + ray.direction * along;
		const vec3_t offset = center - closest;

		return offset.dot( offset ) <= radius * radius;
	}

	bool intersect_triangle( const math::ray_t& ray, const math::triangle_t& triangle ) const noexcept
	{
		constexpr double epsilon = 1e-8;

		const vec3_t& vert0 = triangle.verts[ 0 ];
		const vec3_t& vert1 = triangle.verts[ 1 ];
		const vec3_t& vert2 = triangle.verts[ 2 ];

		vec3_t edge1 = vert1 - vert0;
		vec3_t edge2 = vert2 - vert0;

		vec3_t ray_cross_edge2 = ray.direction.cross( edge2 );
		double determinant = edge1.dot( ray_cross_edge2 );

		if ( determinant > -epsilon && determinant < epsilon )
			return false;

		double inverted_determinant = 1.0 / determinant;
		vec3_t origin_to_vert0 = ray.origin - vert0;
		double barycentric_u = inverted_determinant * origin_to_vert0.dot( ray_cross_edge2 );

		if ( barycentric_u < 0.0 || barycentric_u > 1.0 )
			return false;

		vec3_t origin_cross_edge1 = origin_to_vert0.cross( edge1 );
		double barycentric_v = inverted_determinant * ray.direction.dot( origin_cross_edge1 );

		if ( barycentric_v < 0.0 || barycentric_u + barycentric_v > 1.0 )
			return false;

		double intersection_distance = inverted_determinant * edge2.dot( origin_cross_edge1 );
		return intersection_distance > epsilon && intersection_distance < ray.distance;
	}

private:
	mutable std::mutex m_mutex;
	std::vector< math::geometry_t > m_geometry;
	std::size_t m_triangle_count = 0;
	std::uintptr_t m_component_to_world = offsets::scene_component::component_to_world;

	std::thread m_worker;
	std::atomic< bool > m_worker_running{ false };
	std::atomic< bool > m_force_rebuild{ false };
	bool m_built_once = false;

	mutable std::unordered_map< uint64_t, component_kind > m_class_kinds;

	mutable std::size_t m_drawn_actors = 0;
	mutable std::size_t m_drawn_edges = 0;
};

inline c_raycast* g_raycast = c_raycast::get( );
