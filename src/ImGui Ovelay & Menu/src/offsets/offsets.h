#pragma once
#include <cstdint>

namespace offsets
{

	constexpr uint64_t g_world      = 0xC51C3A8; // GWorld
	constexpr uint64_t g_name_table = 0xC260F40; // GNames (FNamePool)
	constexpr uint64_t g_objects    = 0xC353820; // GObjects (FUObjectArray)
	namespace object
	{
		constexpr uint64_t class_private = 0x0010; // UObject::ClassPrivate
		constexpr uint64_t name_id       = 0x0018; // UObject::NamePrivate (FName)
	}


	namespace ustruct
	{
		constexpr uint64_t super_struct = 0x0060; // UStruct::SuperStruct
	}
	namespace world
	{
		constexpr uint64_t game_state    = 0x01D8; // AGameStateBase*
		constexpr uint64_t game_instance = 0x0250; // UGameInstance* (OwningGameInstance)
		constexpr uint64_t levels        = 0x01F0; // TArray<ULevel*>
	}
	namespace game_instance
	{
		constexpr uint64_t local_players = 0x0058; // TArray<ULocalPlayer*>
	}
	namespace local_player
	{
		constexpr uint64_t player_controller = 0x0050; // APlayerController*
	}
	namespace player_controller
	{
		constexpr uint64_t acknowledged_pawn     = 0x03B0; // APawn*
		constexpr uint64_t player_camera_manager = 0x03C0; // APlayerCameraManager*
	}

	namespace camera_manager
	{
		// FMinimalViewInfo: Location 0x00, Rotation 0x18, FOV 0x30
		constexpr uint64_t camera_cache_private = 0x15D0 + 0x0010;
	}
	namespace game_state
	{
		constexpr uint64_t player_array = 0x0318; // TArray<APlayerState*>
	}


	namespace player_state
	{
		constexpr uint64_t pawn_private = 0x0380; // APawn*
		constexpr uint64_t player_name  = 0x03A0; // FString PlayerNamePrivate
	}
	namespace actor
	{
		constexpr uint64_t root_component       = 0x01E8; // USceneComponent*
		constexpr uint64_t instance_components  = 0x02D8; // TArray<UActorComponent*>
		constexpr uint64_t blueprint_components = 0x02E8; // TArray<UActorComponent*>
	}

	namespace scene_component
	{
		constexpr uint64_t relative_location = 0x0188; // FVector
		constexpr uint64_t mobility          = 0x01EC; // EComponentMobility: 0=Static 1=Stationary 2=Movable
		constexpr uint64_t component_to_world = 0x0230;
	}
	namespace character
	{
		constexpr uint64_t mesh = 0x0380; // USkeletalMeshComponent*
	}
	namespace skeletal_mesh
	{
		constexpr uint64_t skeletal_asset = 0x0638; // USkinnedAsset*
		constexpr uint64_t bone_array     = 0x6A8; // TArray<FTransform>
		constexpr uint64_t bone_array_alt = 0x6B8; // TArray<FTransform> (fallback)
	}

	namespace level
	{
		constexpr uint64_t actors = 0x00C0; // TArray<AActor*>
		constexpr uint64_t model  = 0x00E8; // UModel* (BSP)
	}

	namespace primitive_component
	{
		constexpr uint64_t body_instance = 0x03E0; // FBodyInstance (size 0x180)

		// offsets inside FBodyInstance
		//   +0x17 ECollisionEnabled CollisionEnabled
		//   +0x68 FCollisionResponse -> FCollisionResponseContainer (32 x uint8,
		constexpr uint64_t collision_enabled      = 0x03E0 + 0x0017;
		constexpr uint64_t response_to_channels   = 0x03E0 + 0x0068;
	}
	namespace collision
	{
		constexpr uint8_t channel_visibility = 3; // ECC_Visibility - what LOS traces use
		constexpr uint8_t channel_camera     = 4; // ECC_Camera
		constexpr uint8_t response_ignore    = 0;
		constexpr uint8_t response_overlap   = 1;
		constexpr uint8_t response_block     = 2;
		constexpr uint8_t enabled_none       = 0; // ECollisionEnabled::NoCollision
	}

	namespace static_mesh_component
	{
		constexpr uint64_t static_mesh = 0x0640; // UStaticMesh*
	}
	namespace instanced_static_mesh_component
	{
		constexpr uint64_t per_instance_sm_data = 0x06F0; // TArray<FInstancedStaticMeshInstanceData>
		constexpr uint64_t instance_stride      = 0x0080; // sizeof FInstancedStaticMeshInstanceData
	}
	namespace shape_component
	{
		constexpr uint64_t shape_body_setup = 0x0588; // UBodySetup*
	}
	namespace box_component
	{
		constexpr uint64_t box_extent = 0x05A8; // FVector (half extents)
	}

	namespace brush_component
	{
		constexpr uint64_t brush_body_setup = 0x0590; // UBodySetup*
	}
	namespace static_mesh
	{
		constexpr uint64_t body_setup = 0x0350; // UBodySetup*
	}


	namespace body_setup
	{
		constexpr uint64_t agg_geom = 0x0058; // FKAggregateGeom
	}
	namespace agg_geom
	{
		constexpr uint64_t sphere_elems = 0x0000; // TArray<FKSphereElem>
		constexpr uint64_t box_elems    = 0x0010; // TArray<FKBoxElem>
		constexpr uint64_t sphyl_elems  = 0x0020; // TArray<FKSphylElem>
		constexpr uint64_t convex_elems = 0x0030; // TArray<FKConvexElem>
	}

	namespace shape_elem
	{
		constexpr uint64_t size = 0x0030;
	}

	namespace convex_elem
	{
		constexpr uint64_t size        = 0x0100;
		constexpr uint64_t vertex_data = 0x0030; // TArray<FVector>
		constexpr uint64_t index_data  = 0x0040; // TArray<int32>
	}

	namespace box_elem
	{
		constexpr uint64_t size     = 0x0070;
		constexpr uint64_t center   = 0x0030; // FVector
		constexpr uint64_t rotation = 0x0048; // FRotator (pitch, yaw, roll)
		constexpr uint64_t extent_x = 0x0060; // float
		constexpr uint64_t extent_y = 0x0064; // float
		constexpr uint64_t extent_z = 0x0068; // float
	}

	namespace sphere_elem
	{
		constexpr uint64_t size   = 0x0050;
		constexpr uint64_t center = 0x0030; // FVector
		constexpr uint64_t radius = 0x0048; // float
	}

	namespace sphyl_elem
	{
		constexpr uint64_t size     = 0x0068;
		constexpr uint64_t center   = 0x0030; // FVector
		constexpr uint64_t rotation = 0x0048; // FRotator
		constexpr uint64_t radius   = 0x0060; // float
		constexpr uint64_t length   = 0x0064; // float
	}

	namespace dbd
	{
		constexpr uint64_t health_component   = 0x18E0; // ASurvivor::_healthComponent (UHealthComponent*)
		constexpr uint64_t health_state_count = 0x02A0; // UHealthComponent::_currentHealthStateCount (int32)
	}
}
