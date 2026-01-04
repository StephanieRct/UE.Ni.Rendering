// Fill out your copyright notice in the Description page of Project Settings.


#include "NiMeshComponent.h"

#include "BodySetupEnums.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "Engine/Engine.h"
#include "RenderUtils.h"
#include "PrimitiveDrawingUtils.h"
#include "PrimitiveUniformShaderParametersBuilder.h"
#include "PhysicsEngine/BodySetup.h"
#include "NiMeshComponentPluginPrivate.h"
#include "DynamicMeshBuilder.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "StaticMeshResources.h"
#include "RayTracingInstance.h"
#include "NiRendering/NiVertexFactory.h"
#include "NiRendering/Components.h"
#include "Ni/Ni.h"

#include "RHICommandList.h"


#include <tuple>
//#include "DynamicMeshBuilder.h"
//#include "Materials/Material.h"
//#include "Materials/MaterialRenderProxy.h"

DECLARE_STATS_GROUP(TEXT("NiMeshComponent"), STATGROUP_NiMeshComponent, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Create ProcMesh Proxy"),	STAT_NiMeshComponent_CreateSceneProxy,  STATGROUP_NiMeshComponent);
DECLARE_CYCLE_STAT(TEXT("Create Mesh Section"),		STAT_NiMeshComponent_CreateMeshSection, STATGROUP_NiMeshComponent);
DECLARE_CYCLE_STAT(TEXT("UpdateSection GT"),		STAT_NiMeshComponent_UpdateSectionGT,   STATGROUP_NiMeshComponent);
DECLARE_CYCLE_STAT(TEXT("UpdateSection RT"),		STAT_NiMeshComponent_UpdateSectionRT,   STATGROUP_NiMeshComponent);
DECLARE_CYCLE_STAT(TEXT("Get ProcMesh Elements"),	STAT_NiMeshComponent_GetMeshElements,   STATGROUP_NiMeshComponent);
DECLARE_CYCLE_STAT(TEXT("Update Collision"),		STAT_NiMeshComponent_UpdateCollision,   STATGROUP_NiMeshComponent);

#define NI_MESH_MAX_STATIC_TEXCOORDS 8

namespace NiT::Rendering
{
	struct Policy
	{
		using Size_t = int32;
		using Real_t = float;
		using Real2_t = FVector2f;
		using Real3_t = FVector3f;
		using Color_t = FColor;
		using Position3_t = Real3_t;
		using UV_t = Real2_t;
		using Normal3_t = Real3_t;
		using Tangent3_t = Real3_t;
	};

	//template<typename TPolicy>
	//struct CoVertexPosition3 : public Ni::NodeComponent
	//{
	//	using Policy_t = TPolicy;
	//	typename Policy_t::Position3_t Position;
	//};

	//template<typename TPolicy>
	//struct CoVertexNormal3 : public Ni::NodeComponent
	//{
	//	using Policy_t = TPolicy;
	//	typename Policy_t::Normal3_t Normal;
	//};

	//template<typename TPolicy, typename TPolicy::Size_t TChannel>
	//struct CoVertexUV : public Ni::NodeComponent
	//{
	//	using Policy_t = TPolicy;
	//	typename Policy_t::UV_t UV;
	//};

	//template<typename TPolicy, typename TPolicy::Size_t TChannel>
	//struct CoVertexColor : public Ni::NodeComponent
	//{
	//	using Policy_t = TPolicy;
	//	typename Policy_t::Color_t Color;
	//};

	//template<typename TPolicy>
	//struct CoVertexTangentX3 : public Ni::NodeComponent
	//{
	//	using Policy_t = TPolicy;
	//	typename Policy_t::Tangent3_t Tangent;
	//};
	//template<typename TPolicy>
	//struct CoVertexTangentZ3 : public Ni::NodeComponent
	//{
	//	using Policy_t = TPolicy;
	//	typename Policy_t::Tangent3_t Tangent;
	//	typename Policy_t::Real_t Determinant;
	//};


	template<typename T>
	struct CoVertexPosition3 : public Ni::NodeComponent
	{
		T Value;
	};

	template<typename T>
	struct CoVertexNormal3 : public Ni::NodeComponent
	{
		T Value;
	};

	template<typename T, uint32 TChannel>
	struct CoVertexUV : public Ni::NodeComponent
	{
		T Value;
	};

	template<typename T, uint32 TChannel>
	struct CoVertexColor : public Ni::NodeComponent
	{
		T Value;
	};

	template<typename T>
	struct CoVertexTangentX3 : public Ni::NodeComponent
	{
		T Value;
	};
	template<typename T>
	struct CoVertexTangentZ3 : public Ni::NodeComponent
	{
		T Value;
	};
	template<typename T>
	struct CoIndex : public Ni::NodeComponent
	{
		T Value;
	};

	//struct CoNormal3 : public Ni::NodeComponent
	//{
	//public:
	//	FVector Normal;
	//};

	//template<Ni::Size_t TChannel>
	//struct CoUV : public Ni::NodeComponent
	//{
	//public:
	//	FVector2D UV;
	//};

	//struct CoColor : public Ni::NodeComponent
	//{
	//public:
	//	FColor Color;
	//};
	//struct CoTangent3 : public Ni::NodeComponent
	//{
	//public:
	//	FProcMeshTangent Tangent;
	//};


	//template<typename TIndex>
	//struct CoIndex : public Ni::NodeComponent
	//{
	//public:
	//	TIndex Index;
	//};
}

struct NiMeshPolicy
{
public:
	using Size_t = int32;
	using Index_t = uint32;
	using Real_t = float;
	using Real2_t = FVector2f;
	using Real3_t = FVector3f;
	using Real4_t = FVector4f;
	using Color_t = FColor;
	using Position3_t = Real3_t;
	using UV_t = Real2_t;
	using Normal3_t = Real3_t;
	using TangentX3_t = Real3_t;
	using TangentZ3_t = Real4_t; // w is sign of normal
};
//
//using CoVertexPosition3 = NiT::Rendering::CoVertexPosition3<NiMeshPolicy>;
//using CoVertexNormal3 = NiT::Rendering::CoVertexNormal3<NiMeshPolicy>;
//template<NiMeshPolicy::Size_t TChannel>
//using CoVertexUV = NiT::Rendering::CoVertexUV<NiMeshPolicy, TChannel>;
//template<NiMeshPolicy::Size_t TChannel>
//using CoVertexColor = NiT::Rendering::CoVertexColor<NiMeshPolicy, TChannel>;
//using CoVertexTangentX3 = NiT::Rendering::CoVertexTangentX3<NiMeshPolicy>;
//using CoVertexTangentZ3 = NiT::Rendering::CoVertexTangentZ3<NiMeshPolicy>;
//using CoIndex = NiT::Rendering::CoIndex<NiMeshPolicy>;
//


using CoVertexPosition3 = NiT::Rendering::CoVertexPosition3<NiMeshPolicy::Position3_t>;
using CoVertexNormal3 = NiT::Rendering::CoVertexNormal3<NiMeshPolicy::Normal3_t>;
template<NiMeshPolicy::Size_t TChannel>
using CoVertexUV = NiT::Rendering::CoVertexUV<NiMeshPolicy::UV_t, TChannel>;
template<NiMeshPolicy::Size_t TChannel>
using CoVertexColor = NiT::Rendering::CoVertexColor<NiMeshPolicy::Color_t, TChannel>;
using CoVertexTangentX3 = NiT::Rendering::CoVertexTangentX3<NiMeshPolicy::TangentX3_t>;
using CoVertexTangentZ3 = NiT::Rendering::CoVertexTangentZ3<NiMeshPolicy::TangentZ3_t>;
using CoIndex = NiT::Rendering::CoIndex<NiMeshPolicy::Index_t>;









class FCoUEVertexBuffer : public FVertexBuffer, public Ni::NodeComponent
{
public:
	using Base_t = FVertexBuffer;
	const Ni::NChunkPointer* VertexChunkPointer;
	const Ni::Size_t ComponentDataTypeIndex;
	const uint32 AttributeIndex;
	FCoUEVertexBuffer(const Ni::NChunkPointer* vertexChunkPointer, const Ni::Size_t componentDataTypeIndex, const uint32 attributeIndex)
		: VertexChunkPointer(vertexChunkPointer)
		, ComponentDataTypeIndex(componentDataTypeIndex)
		, AttributeIndex(attributeIndex)
	{
	}
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		auto vertexCount = VertexChunkPointer->GetNodeCount();
		const Ni::ComponentType& componentType = VertexChunkPointer->GetStructure().GetComponentType(ComponentDataTypeIndex);
		auto vertexSize = componentType.GetSize();
		auto dataSize = vertexCount * vertexSize;
		Create(TEXT("FCoUEVertexBuffer"), RHICmdList, vertexSize, dataSize);
		const void* data = VertexChunkPointer->GetComponentData(ComponentDataTypeIndex);
		Upload(RHICmdList, data, dataSize);
	}
	Ni::Size_t GetVertexSize()const
	{
		return VertexChunkPointer->GetStructure().GetComponentType(ComponentDataTypeIndex).GetSize();
	}
protected:
	void Create(const TCHAR* debugName, FRHICommandListBase& RHICmdList, const Ni::Size_t vertexSize, const Ni::Size_t vertexCount)
	{
		FRHIBufferCreateDesc desc = FRHIBufferCreateDesc::Create(debugName, vertexSize * vertexCount, vertexSize, BUF_VertexBuffer | BUF_ShaderResource | BUF_Dynamic);
		desc.InitialState = ERHIAccess::VertexOrIndexBuffer;
		VertexBufferRHI = RHICmdList.CreateBuffer(desc);
	}
	void Upload(FRHICommandListBase& RHICmdList, const void* data, const Ni::Size_t dataSize)
	{
		void* bufferData = RHICmdList.LockBuffer(VertexBufferRHI, 0, dataSize, EResourceLockMode::RLM_WriteOnly);
		FMemory::Memcpy(bufferData, data, dataSize);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
	}
};


class FCoUEIndexBuffer : public FIndexBuffer, public Ni::NodeComponent
{
public:
	using Base_t = FIndexBuffer;
	const Ni::NChunkPointer* IndexChunkPointer;
	Ni::Size_t ComponentDataTypeIndex;
	FCoUEIndexBuffer(const Ni::NChunkPointer* indexChunkPointer, const Ni::Size_t componentDataTypeIndex)
		: IndexChunkPointer(indexChunkPointer)
		, ComponentDataTypeIndex(componentDataTypeIndex)
	{
	}
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		auto indexCount = IndexChunkPointer->GetNodeCount();
		const Ni::ComponentType& componentType = IndexChunkPointer->GetStructure().GetComponentType(ComponentDataTypeIndex);
		auto indexSize = componentType.GetSize();
		auto dataSize = indexCount * indexSize;
		Create(TEXT("FCoUEIndexBuffer"), RHICmdList, indexSize, dataSize);
		const void* data = IndexChunkPointer->GetComponentData(ComponentDataTypeIndex);
		Upload(RHICmdList, data, dataSize);
	}
	Ni::Size_t GetIndexSize()const
	{
		return IndexChunkPointer->GetStructure().GetComponentType(ComponentDataTypeIndex).GetSize();
	}
protected:
	void Create(const TCHAR* debugName, FRHICommandListBase& RHICmdList, const Ni::Size_t indexSize, const Ni::Size_t indexCount)
	{
		FRHIBufferCreateDesc desc = FRHIBufferCreateDesc::Create(debugName, indexSize * indexCount, indexSize, BUF_IndexBuffer | BUF_ShaderResource | BUF_Dynamic);
		desc.InitialState = ERHIAccess::VertexOrIndexBuffer;
		IndexBufferRHI = RHICmdList.CreateBuffer(desc);
	}
	void Upload(FRHICommandListBase& RHICmdList, const void* data, const Ni::Size_t dataSize)
	{
		void* bufferData = RHICmdList.LockBuffer(IndexBufferRHI, 0, dataSize, EResourceLockMode::RLM_WriteOnly);
		FMemory::Memcpy(bufferData, data, dataSize);
		RHICmdList.UnlockBuffer(IndexBufferRHI);
	}
};


//struct CoVertexChunk : public Ni::NBunch, public Ni::NodeComponent
//{
//public:
//	using Base_t = Ni::NBunch;
//	using Base_t::Base_t;
//
//	Ni::Size_t GetVertexCount()const { return GetNodeCount(); }
//};
//struct CoIndexChunk : public Ni::NBunch, public Ni::NodeComponent
//{
//public:
//	using Base_t = Ni::NBunch;
//	using Base_t::Base_t;
//	Ni::Size_t GetIndexCount()const { return GetNodeCount(); }
//};
//
//struct CoUEMaterialRef : public Ni::NodeComponent
//{
//public:
//	UMaterialInterface* Value;
//};

struct UEVertexBufferChunk : public Ni::NBunch
{
public:
	using Base_t = Ni::NBunch;
	using Base_t::Base_t;
	Ni::Size_t GetUEVertexBufferCount()const { return GetNodeCount(); }
	const FCoUEVertexBuffer* GetUEVertexBufferData() const { return GetComponentData<FCoUEVertexBuffer>(); }
	FCoUEVertexBuffer* GetUEVertexBufferData() { return GetComponentData<FCoUEVertexBuffer>(); }
};
struct UEIndexBufferChunk : public Ni::NBunch
{
public:
	using Base_t = Ni::NBunch;
	using Base_t::Base_t;
	Ni::Size_t GetUEIndexBufferCount()const { return GetNodeCount(); }
	const FCoUEIndexBuffer* GetUEIndexBufferData() const { return GetComponentData<FCoUEIndexBuffer>(); }
	FCoUEIndexBuffer* GetUEIndexBufferData() { return GetComponentData<FCoUEIndexBuffer>(); }
};

struct CoMeshSection : public Ni::NodeComponent
{
	using IndexChunk_t = Ni::NBunch;
	using VertexChunk_t = Ni::NBunch;
	IndexChunk_t IndexChunk;
	VertexChunk_t VertexChunk;
	Ni::Size_t GetIndexCount()const { return IndexChunk.GetNodeCount(); }
	Ni::Size_t GetVertexCount()const { return VertexChunk.GetNodeCount(); }
};
struct CoUEMeshSection : public Ni::NodeComponent
{
	using IndexChunk_t = Ni::NBunch;
	using VertexChunk_t = Ni::NBunch;
	IndexChunk_t UEIndexBufferChunk;
	VertexChunk_t UEVertexBufferChunk;
	UMaterialInterface* UEMaterial;

#if RHI_RAYTRACING
	Ni::Size_t RayTracingIndexBufferIndex;
	Ni::Size_t RayTracingVertexPositionBufferIndex;
	FRayTracingGeometry RayTracingGeometry;
#endif

	bool IsValid()const { return !!UEMaterial; }
	Ni::Size_t GetUEIndexBufferCount()const { return UEIndexBufferChunk.GetNodeCount(); }
	Ni::Size_t GetUEVertexBufferCount()const { return UEVertexBufferChunk.GetNodeCount(); }


	const FCoUEIndexBuffer* GetUEIndexBufferData() const { return UEIndexBufferChunk.GetComponentData<FCoUEIndexBuffer>(); }
	FCoUEIndexBuffer* GetUEIndexBufferData() { return UEIndexBufferChunk.GetComponentData<FCoUEIndexBuffer>(); }
	const FCoUEVertexBuffer* GetUEVertexBufferData() const { return UEVertexBufferChunk.GetComponentData<FCoUEVertexBuffer>(); }
	FCoUEVertexBuffer* GetUEVertexBufferData() { return UEVertexBufferChunk.GetComponentData<FCoUEVertexBuffer>(); }
};

struct MeshChunk 
{
public:
	//using Base_t = Ni::NBunch;
	Ni::NBunch Chunk;

	MeshChunk() = default;
	MeshChunk(const MeshChunk&) = default;
	MeshChunk(MeshChunk&&) = default;
	MeshChunk& operator=(const MeshChunk&) = default;
	MeshChunk& operator=(MeshChunk&&) = default;


	MeshChunk(const Ni::ChunkStructure* structure, const Ni::Size_t sectionCapacity, const Ni::Size_t sectionCount)
		: Chunk(structure, Ni::NodeCapacity(sectionCapacity), Ni::NodeCount(sectionCount))
	{
	}

	MeshChunk(const Ni::Size_t sectionCapacity, const Ni::Size_t sectionCount)
		: Chunk(BaseStructure::GetBaseMeshStructure(), Ni::NodeCapacity(sectionCapacity), Ni::NodeCount(sectionCount))
	{

	}

	Ni::NodeCount GetSectionCount()const { return Chunk.GetNodeCount(); }
	const CoMeshSection& GetSection(Ni::Size_t sectionIndex)const { return Chunk.GetComponentData<CoMeshSection>(sectionIndex); }
	CoMeshSection& GetSection(Ni::Size_t sectionIndex) { return Chunk.GetComponentData<CoMeshSection>(sectionIndex); }

	Ni::NodeIndex AddSection()
	{
		return Chunk.AddNode();
	}
	Ni::NodeIndex AddSection(const Ni::Size_t indexCount, const Ni::Size_t vertexCount)
	{
		return AddSection(indexCount, indexCount, vertexCount, vertexCount);
	}
	Ni::NodeIndex AddSection(const Ni::Size_t indexCapacity, const Ni::Size_t indexCount, const Ni::Size_t vertexCapacity, const Ni::Size_t vertexCount)
	{
		const Ni::ChunkStructure* indexStructure = BaseStructure::GetBaseIndexStructure();
		const Ni::ChunkStructure* vertexStructure = BaseStructure::GetBaseVertexStructure();

		Ni::NodeIndex sectionIndex = AddSection();
		CoMeshSection& section = GetSection(sectionIndex);
		ni_assert(section.IndexChunk.IsNull());
		ni_assert(section.VertexChunk.IsNull());
		new(&section.IndexChunk) CoMeshSection::IndexChunk_t(indexStructure, indexCapacity, indexCount);
		new(&section.VertexChunk) CoMeshSection::VertexChunk_t(vertexStructure, vertexCapacity, vertexCount);
		return sectionIndex;
	}
	//CoIndexChunk& GetIndexChunk(const Ni::Size_t sectionIndex) { return Chunk.GetComponentData<CoIndexChunk>(Ni::NodeIndex(sectionIndex)); }
	//const CoIndexChunk& GetIndexChunk(const Ni::Size_t sectionIndex) const { return Chunk.GetComponentData<CoIndexChunk>(Ni::NodeIndex(sectionIndex)); }

	//CoVertexChunk& GetVertexChunk(const Ni::Size_t sectionIndex) { return Chunk.GetComponentData<CoVertexChunk>(Ni::NodeIndex(sectionIndex)); }
	//const CoVertexChunk& GetVertexChunk(const Ni::Size_t sectionIndex) const{ return Chunk.GetComponentData<CoVertexChunk>(Ni::NodeIndex(sectionIndex)); }

	//CoUEIndexBufferChunk& GetUEIndexBuffer(const Ni::Size_t sectionIndex) { return Chunk.GetComponentData<CoUEIndexBufferChunk>(Ni::NodeIndex(sectionIndex)); }
	//const CoUEIndexBufferChunk& GetUEIndexBuffer(const Ni::Size_t sectionIndex) const { return Chunk.GetComponentData<CoUEIndexBufferChunk>(Ni::NodeIndex(sectionIndex)); }

	//CoUEVertexBufferChunk& GetUEVertexBuffer(const Ni::Size_t sectionIndex) { return Chunk.GetComponentData<CoUEVertexBufferChunk>(Ni::NodeIndex(sectionIndex)); }
	//const CoUEVertexBufferChunk& GetUEVertexBuffer(const Ni::Size_t sectionIndex) const { return Chunk.GetComponentData<CoUEVertexBufferChunk>(Ni::NodeIndex(sectionIndex)); }

	//CoUEMaterialRef& GetSectionMaterial(const Ni::Size_t sectionIndex) { return Chunk.GetComponentData<CoUEMaterialRef>(sectionIndex); }
	//const CoUEMaterialRef& GetSectionMaterial(const Ni::Size_t sectionIndex)const { return Chunk.GetComponentData<CoUEMaterialRef>(sectionIndex); }

	const CoUEMeshSection& GetUESection(const Ni::Size_t sectionIndex)const { return Chunk.GetComponentData<CoUEMeshSection>(sectionIndex); }
	CoUEMeshSection& GetUESection(const Ni::Size_t sectionIndex) { return Chunk.GetComponentData<CoUEMeshSection>(sectionIndex); }
	//CoUEMeshSection& GetOrCreateUESection(Ni::Size_t sectionIndex) 
	//{
	//	CoUEMeshSection& ueSection = GetUESection(sectionIndex);
	//	if (ueSection.IsValid())
	//		return ueSection;
	//	CreateUESection(sectionIndex);
	//	return GetUESection(sectionIndex);
	//}
	CoUEMeshSection& CreateUESection(const Ni::Size_t sectionIndex, UMaterialInterface*const ueMaterial)
	{
		const Ni::ChunkStructure* ueVertexBufferStructure = BaseStructure::GetBaseUEVertexBufferStructure();
		const Ni::ChunkStructure* ueIndexBufferStructure = BaseStructure::GetBaseUEIndexBufferStructure();

		CoMeshSection& section = GetSection(sectionIndex);
		CoUEMeshSection& ueSection = GetUESection(sectionIndex);
		auto indexBufferCount = section.IndexChunk.GetStructure().GetComponentCount();
		auto vertexBufferCount = section.VertexChunk.GetStructure().GetComponentCount();

		ni_assert(ueSection.UEIndexBufferChunk.IsNull());
		ni_assert(ueSection.UEVertexBufferChunk.IsNull());
		new(&ueSection.UEIndexBufferChunk) CoUEMeshSection::IndexChunk_t(ueIndexBufferStructure, indexBufferCount, 0);
		new(&ueSection.UEVertexBufferChunk) CoUEMeshSection::VertexChunk_t(ueVertexBufferStructure, vertexBufferCount, 0);

#if RHI_RAYTRACING
		ueSection.RayTracingIndexBufferIndex = section.IndexChunk.GetStructure().GetComponentTypeIndexInChunk<CoIndex>();
		ueSection.RayTracingVertexPositionBufferIndex = section.VertexChunk.GetStructure().GetComponentTypeIndexInChunk<CoVertexPosition3>();
#endif
		// Create a FCoUEIndexBuffer for each index buffer components;
		FCoUEIndexBuffer* ueIndexBuffers = ueSection.GetUEIndexBufferData();
		for (auto i = 0; i < indexBufferCount; ++i)
		{
			new(&ueIndexBuffers[i]) FCoUEIndexBuffer(&section.IndexChunk, i);
		}
		CoUEMeshSection::IndexChunk_t::GetInternalChunk(ueSection.UEIndexBufferChunk).NodeCount = Ni::NodeCount(indexBufferCount);

		// Create a FCoUEVertexBuffer for each vertex buffer components;
		FCoUEVertexBuffer* ueVertexBuffers = ueSection.GetUEVertexBufferData();
		for (auto i = 0; i < vertexBufferCount; ++i)
		{
			new(&ueVertexBuffers[i]) FCoUEVertexBuffer(&section.VertexChunk, i, BaseStructure::GetBaseVertexAttributeIndex(i));
		}
		CoUEMeshSection::VertexChunk_t::GetInternalChunk(ueSection.UEVertexBufferChunk).NodeCount = Ni::NodeCount(vertexBufferCount);

		for (auto i = 0; i < indexBufferCount; ++i)
			BeginInitResource(&ueIndexBuffers[i]);

		for (auto i = 0; i < vertexBufferCount; ++i)
			BeginInitResource(&ueVertexBuffers[i]);
		return ueSection;
	}
	//void InitUEResources(const Ni::Size_t sectionIndex)
	//{
	//	CoUEMeshSection& ueSection = GetUESection(sectionIndex);
	//	FCoUEIndexBuffer* ueIndexBuffers = ueSection.GetUEIndexBufferData();
	//	FCoUEVertexBuffer* ueVertexBuffers = ueSection.GetUEVertexBufferData();
	//	auto indexBufferCount = ueSection.GetUEIndexBufferCount();
	//	auto vertexBufferCount = ueSection.GetUEVertexBufferCount();

	//	for (auto i = 0; i < indexBufferCount; ++i)
	//		BeginInitResource(&ueIndexBuffers[i]);

	//	for (auto i = 0; i < vertexBufferCount; ++i)
	//		BeginInitResource(&ueVertexBuffers[i]);
	//}

	static struct BaseStructure
	{
		static auto GetBaseMeshStructureComponentTypes()
		{
			auto& reg = Ni::ComponentTypeRegistry::GetInstance();
			return std::make_tuple(
				reg.GetOrAddComponentType<CoMeshSection>(),
				reg.GetOrAddComponentType<CoUEMeshSection>());
		}
		static const Ni::ChunkStructure* GetBaseMeshStructure()
		{
			static const Ni::ChunkStructure* value = Ni::ChunkStructureRegistry::GetInstance().GetOrAddChunkStructure(GetBaseMeshStructureComponentTypes());
			return value;
		}


		static auto GetBaseIndexStructureComponentTypes()
		{
			auto& reg = Ni::ComponentTypeRegistry::GetInstance();
			return std::make_tuple(reg.GetOrAddComponentType<CoIndex>());
		}
		static const Ni::ChunkStructure* GetBaseIndexStructure()
		{
			static const Ni::ChunkStructure* value = Ni::ChunkStructureRegistry::GetInstance().GetOrAddChunkStructure(GetBaseIndexStructureComponentTypes());
			return value;
		}


		static auto GetBaseUEIndexBufferStructureComponentTypes()
		{
			auto& reg = Ni::ComponentTypeRegistry::GetInstance();
			return std::make_tuple(
				reg.GetOrAddComponentType<FCoUEIndexBuffer>());
		}
		static const Ni::ChunkStructure* GetBaseUEIndexBufferStructure()
		{
			static const Ni::ChunkStructure* value = Ni::ChunkStructureRegistry::GetInstance().GetOrAddChunkStructure(GetBaseUEIndexBufferStructureComponentTypes());
			return value;
		}



		static uint32 GetBaseVertexAttributeIndex(const uint32 baseComponentIndex)
		{
			switch (baseComponentIndex)
			{
				case 0: return 0;
				case 1: return 1;
				case 2: return 2;
				case 3: return 3;
				case 4: return 4;
				case 5: return 5;
				case 6: return 6;
				case 7: return 7;
			}
			checkNoEntry();
			return -1;
		}
		static auto GetBaseVertexStructureComponentTypes()
		{
			auto& reg = Ni::ComponentTypeRegistry::GetInstance();
			return std::make_tuple(
				reg.GetOrAddComponentType<CoVertexPosition3>(),
				reg.GetOrAddComponentType<CoVertexTangentX3>(),
				reg.GetOrAddComponentType<CoVertexTangentZ3>(),
				reg.GetOrAddComponentType<CoVertexColor<0>>(),
				reg.GetOrAddComponentType<CoVertexUV<0>>(),
				reg.GetOrAddComponentType<CoVertexUV<1>>(),
				reg.GetOrAddComponentType<CoVertexUV<2>>(),
				reg.GetOrAddComponentType<CoVertexUV<3>>()//,
				//reg.GetOrAddComponentType<InstanceOrigin>(),
				//reg.GetOrAddComponentType<InstanceTransform1>(),
				//reg.GetOrAddComponentType<InstanceTransform2>(),
				//reg.GetOrAddComponentType<InstanceTransform3>(),
			);
		}
		static const Ni::ChunkStructure* GetBaseVertexStructure()
		{
			static const Ni::ChunkStructure* value = Ni::ChunkStructureRegistry::GetInstance().GetOrAddChunkStructure(GetBaseVertexStructureComponentTypes());
			return value;
		}


		static auto GetBaseUEVertexBufferStructureComponentTypes()
		{
			auto& reg = Ni::ComponentTypeRegistry::GetInstance();
			return std::make_tuple(
				reg.GetOrAddComponentType<FCoUEVertexBuffer>());
		}
		static const Ni::ChunkStructure* GetBaseUEVertexBufferStructure()
		{
			static const Ni::ChunkStructure* value = Ni::ChunkStructureRegistry::GetInstance().GetOrAddChunkStructure(GetBaseUEVertexBufferStructureComponentTypes());
			return value;
		}
	};
	//FCoUEIndexBuffer
};



class FMeshChunkVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE_API(FMeshChunkVertexFactory, ENGINE_API);
public:
	ENGINE_API virtual ~FMeshChunkVertexFactory()
	{

	}

	ENGINE_API virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 attributeIndex = 0;
		uint32 offset = 0;
		FVertexDeclarationElementList elements;
		TEnumAsByte<EVertexElementType> VertexElementType = VET_None;

		const auto sectionCount = Mesh->GetSectionCount();
		for (auto iSection = 0; iSection < sectionCount; ++iSection)
		{
			const CoUEMeshSection& ueSection = Mesh->GetUESection(iSection);
			//const CoUEVertexBufferChunk& bufferChunk = Mesh->GetUEVertexBuffer(iSection);
			const auto bufferCount = ueSection.GetUEVertexBufferCount();
			const FCoUEVertexBuffer* buffers = ueSection.GetUEVertexBufferData();
			for (auto iBuffer = 0; iBuffer < bufferCount; ++iBuffer)
			{ 
				auto& buffer = buffers[iBuffer];
				FVertexStream VertexStream;
				VertexStream.VertexBuffer = &buffer;
				VertexStream.Stride = buffer.GetVertexSize();
				VertexStream.Offset = offset;
				VertexStream.VertexStreamUsage = EVertexStreamUsage::Default;
				FVertexElement vertexElement((uint8)Streams.AddUnique(VertexStream), offset, VertexElementType, buffer.AttributeIndex, VertexStream.Stride, EnumHasAnyFlags(EVertexStreamUsage::Instancing, VertexStream.VertexStreamUsage));

				elements.Add(vertexElement);
			}
		}
		//auto positionComponent = FVertexStreamComponent(&InVertexBuffer->PositionBuffer, 0, sizeof(FVector3f), VET_Float3, EVertexStreamUsage::Default);

		AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, elements, 13, 13);
		InitDeclaration(elements);
	}
	virtual void ReleaseRHI() override
	{
		FVertexFactory::ReleaseRHI();
	}
	FVertexStreamComponent PositionComponent;
	FRHIShaderResourceView* PositionComponentSRV = nullptr;
	MeshChunk* Mesh;
};



class FNiMeshProxySection
{
public:
	/** Material applied to this section */
	UMaterialInterface* Material;

	//Ni::KChunkPointer Chunk;

	//int IndexPosition;
	//int IndexUV[NI_MESH_MAX_STATIC_TEXCOORDS];
	//int IndexTangent;
	//int IndexColor;

	/** Vertex buffer for this section */
	FStaticMeshVertexBuffers VertexBuffers;
	/** Index buffer for this section */
	FDynamicMeshIndexBuffer32 IndexBuffer;
	/** Vertex factory for this section */
	FNiVertexFactory VertexFactory;
	/** Whether this section is currently visible */
	bool bSectionVisible;

#if RHI_RAYTRACING
	FRayTracingGeometry RayTracingGeometry;
#endif

	FNiMeshProxySection(ERHIFeatureLevel::Type InFeatureLevel)
		: Material(nullptr)
		, VertexFactory(InFeatureLevel, "FNiMeshProxySection")
		, bSectionVisible(true)
	{
	}

	//void ENGINE_API InitFromDynamicVertex(FRHICommandListBase* RHICmdList, FRenderCommandPipe* RenderCommandPipe, FLocalVertexFactory* VertexFactory, TArray<FDynamicMeshVertex>& Vertices, uint32 NumTexCoords, uint32 LightMapIndex)
	//{

	//	check(NumTexCoords < NI_MESH_MAX_STATIC_TEXCOORDS && NumTexCoords > 0);
	//	check(LightMapIndex < NumTexCoords);

	//	if (Vertices.Num())
	//	{
	//		PositionVertexBuffer.Init(Vertices.Num());
	//		StaticMeshVertexBuffer.Init(Vertices.Num(), NumTexCoords);
	//		ColorVertexBuffer.Init(Vertices.Num());

	//		for (int32 i = 0; i < Vertices.Num(); i++)
	//		{
	//			const FDynamicMeshVertex& Vertex = Vertices[i];

	//			PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
	//			StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX.ToFVector3f(), Vertex.GetTangentY(), Vertex.TangentZ.ToFVector3f());
	//			for (uint32 j = 0; j < NumTexCoords; j++)
	//			{
	//				StaticMeshVertexBuffer.SetVertexUV(i, j, Vertex.TextureCoordinate[j]);
	//			}
	//			ColorVertexBuffer.VertexColor(i) = Vertex.Color;
	//		}
	//	}
	//	else
	//	{
	//		PositionVertexBuffer.Init(1);
	//		StaticMeshVertexBuffer.Init(1, 1);
	//		ColorVertexBuffer.Init(1);

	//		PositionVertexBuffer.VertexPosition(0) = FVector3f(0, 0, 0);
	//		StaticMeshVertexBuffer.SetVertexTangents(0, FVector3f(1, 0, 0), FVector3f(0, 1, 0), FVector3f(0, 0, 1));
	//		StaticMeshVertexBuffer.SetVertexUV(0, 0, FVector2f(0, 0));
	//		ColorVertexBuffer.VertexColor(0) = FColor(1, 1, 1, 1);
	//		NumTexCoords = 1;
	//		LightMapIndex = 0;
	//	}

	//	auto Lambda = [this, VertexFactory, LightMapIndex](FRHICommandListBase& RHICmdList)
	//		{
	//			InitOrUpdateResource(RHICmdList, &PositionVertexBuffer);
	//			InitOrUpdateResource(RHICmdList, &StaticMeshVertexBuffer);
	//			InitOrUpdateResource(RHICmdList, &ColorVertexBuffer);

	//			FLocalVertexFactory::FDataType Data;
	//			PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory, Data);
	//			StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory, Data);
	//			StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactory, Data);
	//			StaticMeshVertexBuffer.BindLightMapVertexBuffer(VertexFactory, Data, LightMapIndex);
	//			ColorVertexBuffer.BindColorVertexBuffer(VertexFactory, Data);
	//			VertexFactory->SetData(RHICmdList, Data);

	//			InitOrUpdateResource(RHICmdList, VertexFactory);
	//		};

	//	if (RHICmdList)
	//	{
	//		Lambda(*RHICmdList);
	//	}
	//	else
	//	{
	//		ENQUEUE_RENDER_COMMAND(StaticMeshVertexBuffersInitFromDynamicVertex)(RenderCommandPipe, MoveTemp(Lambda));
	//	}
	//}
};

class FNiMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:

	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FNiMeshSceneProxy(UNiMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, Chunk(1, 0)
		, BodySetup(Component->GetBodySetup())
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetShaderPlatform()))
	{
		const auto vertexCount = 6;
		const auto indexCount = 6;
		auto sectionIndex = Chunk.AddSection(indexCount, vertexCount);
		CoMeshSection& section = Chunk.GetSection(sectionIndex);

		CoIndex* indexData = section.IndexChunk.GetComponentData<CoIndex>();
		indexData[0].Value = 0;
		indexData[1].Value = 1;
		indexData[2].Value = 2;
		indexData[3].Value = 3;
		indexData[4].Value = 4;
		indexData[5].Value = 5;

		CoVertexPosition3* posData = section.VertexChunk.GetComponentData<CoVertexPosition3>();
		posData[0].Value = FVector3f(0, 0, 100);
		posData[1].Value = FVector3f(100, 0, 100);
		posData[2].Value = FVector3f(100, 100, 100);
		posData[3].Value = FVector3f(100, 100, 100);
		posData[4].Value = FVector3f(0, 100, 100);
		posData[5].Value = FVector3f(0, 0, 100);

		CoVertexTangentX3* tanXData = section.VertexChunk.GetComponentData<CoVertexTangentX3>();
		tanXData[0].Value = FVector3f(1, 0, 0);
		tanXData[1].Value = FVector3f(1, 0, 0);
		tanXData[2].Value = FVector3f(1, 0, 0);
		tanXData[3].Value = FVector3f(1, 0, 0);
		tanXData[4].Value = FVector3f(1, 0, 0);
		tanXData[5].Value = FVector3f(1, 0, 0);

		CoVertexTangentZ3* tanZData = section.VertexChunk.GetComponentData<CoVertexTangentZ3>();
		tanZData[0].Value = FVector4f(0, 1, 0, 1);
		tanZData[1].Value = FVector4f(0, 1, 0, 1);
		tanZData[2].Value = FVector4f(0, 1, 0, 1);
		tanZData[3].Value = FVector4f(0, 1, 0, 1);
		tanZData[4].Value = FVector4f(0, 1, 0, 1);
		tanZData[5].Value = FVector4f(0, 1, 0, 1);

		CoVertexUV<0>* uvData = section.VertexChunk.GetComponentData<CoVertexUV<0>>();
		uvData[0].Value = FVector2f(0, 0);
		uvData[1].Value = FVector2f(0, 0);
		uvData[2].Value = FVector2f(0, 0);
		uvData[3].Value = FVector2f(0, 0);
		uvData[4].Value = FVector2f(0, 0);
		uvData[5].Value = FVector2f(0, 0);

		CoVertexColor<0>* colorData = section.VertexChunk.GetComponentData<CoVertexColor<0>>();
		colorData[0].Value = FColor(1, 1, 1, 1);
		colorData[1].Value = FColor(1, 1, 1, 1);
		colorData[2].Value = FColor(1, 1, 1, 1);
		colorData[3].Value = FColor(1, 1, 1, 1);
		colorData[4].Value = FColor(1, 1, 1, 1);
		colorData[5].Value = FColor(1, 1, 1, 1);

		auto* material = Component->GetMaterial(sectionIndex);
		if (!material)
			material = UMaterial::GetDefaultMaterial(MD_Surface);
		// create and init UE RHI buffers
		CoUEMeshSection& ueSection = Chunk.CreateUESection(sectionIndex, material);
		

#if RHI_RAYTRACING
		if (IsRayTracingEnabled())
		{
			ENQUEUE_RENDER_COMMAND(InitNiMeshRayTracingGeometry)(
				[this, DebugName = Component->GetFName(), &section, &ueSection](FRHICommandListImmediate& RHICmdList)
				{
					FRayTracingGeometryInitializer Initializer;
					Initializer.DebugName = DebugName;
					const FCoUEIndexBuffer* indexBuffers = ueSection.GetUEIndexBufferData();
					Initializer.IndexBuffer = indexBuffers[ueSection.RayTracingIndexBufferIndex].IndexBufferRHI;
					Initializer.TotalPrimitiveCount = section.IndexChunk.GetNodeCount() / 3;
					Initializer.GeometryType = RTGT_Triangles;
					Initializer.bFastBuild = true;
					Initializer.bAllowUpdate = false;

					FRayTracingGeometrySegment Segment;
					const FCoUEVertexBuffer* vertexBuffers = ueSection.GetUEVertexBufferData();
					Segment.VertexBuffer = vertexBuffers[ueSection.RayTracingVertexPositionBufferIndex].VertexBufferRHI;
					Segment.MaxVertices = section.VertexChunk.GetNodeCount();
					Segment.NumPrimitives = Initializer.TotalPrimitiveCount;

					Initializer.Segments.Add(Segment);

					ueSection.RayTracingGeometry.SetInitializer(Initializer);
					ueSection.RayTracingGeometry.InitResource(RHICmdList);
				});
		}
#endif







		// Copy each section
		const int32 NumSections = 1;// Component->ProcMeshSections.Num();
		Sections.AddZeroed(NumSections);
		//int SectionIdx = 0;
		for (int SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
		{
		/// 	FProcMeshSection& SrcSection = Component->ProcMeshSections[SectionIdx];
		/// 	if (SrcSection.ProcIndexBuffer.Num() > 0 && SrcSection.ProcVertexBuffer.Num() > 0)
		/// 	{
				FNiMeshProxySection* NewSection = new FNiMeshProxySection(GetScene().GetFeatureLevel());

				// Copy data from vertex buffer
				const int32 NumVerts = 6;

				// Allocate verts
				TArray<FDynamicMeshVertex> Vertices;
				Vertices.SetNumUninitialized(NumVerts);
				Vertices[0] = FDynamicMeshVertex(FVector3f(0, 0, 0));
				Vertices[1] = FDynamicMeshVertex(FVector3f(100, 0, 0));
				Vertices[2] = FDynamicMeshVertex(FVector3f(100, 100, 0));
				Vertices[3] = FDynamicMeshVertex(FVector3f(100, 100, 0));
				Vertices[4] = FDynamicMeshVertex(FVector3f(0, 100, 0));
				Vertices[5] = FDynamicMeshVertex(FVector3f(0, 0, 0));

				ProcIndexBuffer.SetNumUninitialized(6);
				ProcIndexBuffer[0] = 0;
				ProcIndexBuffer[1] = 1;
				ProcIndexBuffer[2] = 2;
				ProcIndexBuffer[3] = 3;
				ProcIndexBuffer[4] = 4;
				ProcIndexBuffer[5] = 5;

				// Copy index buffer
				NewSection->IndexBuffer.Indices = ProcIndexBuffer;

				NewSection->VertexBuffers.InitFromDynamicVertex((FLocalVertexFactory*)&NewSection->VertexFactory, Vertices, 4);

				//NewSection->VertexBuffers.PositionVertexBuffer.

				// Enqueue initialization of render resource
				BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
				BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
				BeginInitResource(&NewSection->VertexBuffers.ColorVertexBuffer);
				BeginInitResource(&NewSection->IndexBuffer);
				BeginInitResource(&NewSection->VertexFactory);

				// Grab material
				NewSection->Material = Component->GetMaterial(SectionIdx);
				if (NewSection->Material == NULL)
				{
					NewSection->Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}

				// Copy visibility info
				NewSection->bSectionVisible = true;

				// Save ref to new section
				Sections[SectionIdx] = NewSection;

#if RHI_RAYTRACING
				if (IsRayTracingEnabled())
				{
					ENQUEUE_RENDER_COMMAND(InitProceduralMeshRayTracingGeometry)(
						[this, DebugName = Component->GetFName(), NewSection](FRHICommandListImmediate& RHICmdList)
						{
							FRayTracingGeometryInitializer Initializer;
							Initializer.DebugName = DebugName;
							Initializer.IndexBuffer = NewSection->IndexBuffer.IndexBufferRHI;
							Initializer.TotalPrimitiveCount = NewSection->IndexBuffer.Indices.Num() / 3;
							Initializer.GeometryType = RTGT_Triangles;
							Initializer.bFastBuild = true;
							Initializer.bAllowUpdate = false;

							FRayTracingGeometrySegment Segment;
							Segment.VertexBuffer = NewSection->VertexBuffers.PositionVertexBuffer.VertexBufferRHI;
							Segment.MaxVertices = NewSection->VertexBuffers.PositionVertexBuffer.GetNumVertices();
							Segment.NumPrimitives = Initializer.TotalPrimitiveCount;

							Initializer.Segments.Add(Segment);

							NewSection->RayTracingGeometry.SetInitializer(Initializer);
							NewSection->RayTracingGeometry.InitResource(RHICmdList);
						});
				}
#endif
		/// 	}
		}
	}

	virtual ~FNiMeshSceneProxy()
	{
		for (auto iSection = 0; iSection < Chunk.GetSectionCount(); ++iSection)
		{
			CoUEMeshSection& ueSection = Chunk.GetUESection(iSection);
			auto* ueIndexBuffers = ueSection.GetUEIndexBufferData();
			for (auto iBuffer = 0; iBuffer < ueSection.GetUEIndexBufferCount(); ++iBuffer)
				ueIndexBuffers[iBuffer].ReleaseResource();
			auto* ueVertexBuffers = ueSection.GetUEVertexBufferData();
			for (auto iBuffer = 0; iBuffer < ueSection.GetUEVertexBufferCount(); ++iBuffer)
				ueVertexBuffers[iBuffer].ReleaseResource();
		}

		for (FNiMeshProxySection* Section : Sections)
		{
			if (Section != nullptr)
			{
				Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
				Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
				Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
				Section->IndexBuffer.ReleaseResource();
				Section->VertexFactory.ReleaseResource();

#if RHI_RAYTRACING
				Section->RayTracingGeometry.ReleaseResource();
#endif

				delete Section;
			}
		}
	}
//
//	/** Called on render thread to assign new dynamic data */
//	void UpdateSection_RenderThread(FRHICommandListBase& RHICmdList, FProcMeshSectionUpdateData* SectionData)
//	{
//		SCOPE_CYCLE_COUNTER(STAT_ProcMesh_UpdateSectionRT);
//
//		// Check we have data 
//		if (SectionData != nullptr)
//		{
//			// Check it references a valid section
//			if (SectionData->TargetSection < Sections.Num() &&
//				Sections[SectionData->TargetSection] != nullptr)
//			{
//				FProcMeshProxySection* Section = Sections[SectionData->TargetSection];
//
//				// Lock vertex buffer
//				const int32 NumVerts = SectionData->NewVertexBuffer.Num();
//
//				// Iterate through vertex data, copying in new info
//				for (int32 i = 0; i < NumVerts; i++)
//				{
//					const FProcMeshVertex& ProcVert = SectionData->NewVertexBuffer[i];
//					FDynamicMeshVertex Vertex;
//					ConvertProcMeshToDynMeshVertex(Vertex, ProcVert);
//
//					Section->VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
//					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX.ToFVector3f(), Vertex.GetTangentY(), Vertex.TangentZ.ToFVector3f());
//					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
//					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 1, Vertex.TextureCoordinate[1]);
//					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 2, Vertex.TextureCoordinate[2]);
//					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 3, Vertex.TextureCoordinate[3]);
//					Section->VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
//				}
//
//				{
//					auto& VertexBuffer = Section->VertexBuffers.PositionVertexBuffer;
//					void* VertexBufferData = RHICmdList.LockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
//					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
//					RHICmdList.UnlockBuffer(VertexBuffer.VertexBufferRHI);
//				}
//
//				{
//					auto& VertexBuffer = Section->VertexBuffers.ColorVertexBuffer;
//					void* VertexBufferData = RHICmdList.LockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
//					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
//					RHICmdList.UnlockBuffer(VertexBuffer.VertexBufferRHI);
//				}
//
//				{
//					auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
//					void* VertexBufferData = RHICmdList.LockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
//					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
//					RHICmdList.UnlockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
//				}
//
//				{
//					auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
//					void* VertexBufferData = RHICmdList.LockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
//					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
//					RHICmdList.UnlockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
//				}
//
//#if RHI_RAYTRACING
//				if (IsRayTracingEnabled())
//				{
//					Section->RayTracingGeometry.ReleaseResource();
//
//					FRayTracingGeometryInitializer Initializer;
//					Initializer.IndexBuffer = Section->IndexBuffer.IndexBufferRHI;
//					Initializer.TotalPrimitiveCount = Section->IndexBuffer.Indices.Num() / 3;
//					Initializer.GeometryType = RTGT_Triangles;
//					Initializer.bFastBuild = true;
//					Initializer.bAllowUpdate = false;
//
//					FRayTracingGeometrySegment Segment;
//					Segment.VertexBuffer = Section->VertexBuffers.PositionVertexBuffer.VertexBufferRHI;
//					Segment.MaxVertices = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices();
//					Segment.NumPrimitives = Initializer.TotalPrimitiveCount;
//
//					Initializer.Segments.Add(Segment);
//
//					Section->RayTracingGeometry.SetInitializer(Initializer);
//					Section->RayTracingGeometry.InitResource(RHICmdList);
//				}
//#endif
//			}
//
//			// Free data sent from game thread
//			delete SectionData;
//		}
//	}
//
//	void SetSectionVisibility_RenderThread(int32 SectionIndex, bool bNewVisibility)
//	{
//		check(IsInRenderingThread());
//
//		if (SectionIndex < Sections.Num() &&
//			Sections[SectionIndex] != nullptr)
//		{
//			Sections[SectionIndex]->bSectionVisible = bNewVisibility;
//		}
//	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		SCOPE_CYCLE_COUNTER(STAT_NiMeshComponent_GetMeshElements);


		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
				FLinearColor(0, 0.5f, 1.f)
			);

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		// Iterate over sections
		for (const FNiMeshProxySection* Section : Sections)
		{
			if (Section != nullptr && Section->bSectionVisible)
			{
				FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Section->Material->GetRenderProxy();

				// For each view..
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						const FSceneView* View = Views[ViewIndex];
						// Draw the mesh.
						FMeshBatch& Mesh = Collector.AllocateMesh();
						FMeshBatchElement& BatchElement = Mesh.Elements[0];
						BatchElement.IndexBuffer = &Section->IndexBuffer;
						Mesh.bWireframe = bWireframe;
						Mesh.VertexFactory = &Section->VertexFactory;
						Mesh.MaterialRenderProxy = MaterialProxy;

						FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
						FPrimitiveUniformShaderParametersBuilder Builder;
						BuildUniformShaderParameters(Builder);
						DynamicPrimitiveUniformBuffer.Set(Collector.GetRHICommandList(), Builder);

						BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

						BatchElement.FirstIndex = 0;
						BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
						BatchElement.MinVertexIndex = 0;
						BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
						Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
						Mesh.Type = PT_TriangleList;
						Mesh.DepthPriorityGroup = SDPG_World;
						Mesh.bCanApplyViewModeOverrides = false;
						Collector.AddMesh(ViewIndex, Mesh);
					}
				}
			}
		}

		// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				// Draw simple collision as wireframe if 'show collision', and collision is enabled, and we are not using the complex as the simple
				if (ViewFamily.EngineShowFlags.Collision && IsCollisionEnabled() && BodySetup->GetCollisionTraceFlag() != ECollisionTraceFlag::CTF_UseComplexAsSimple)
				{
					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, AlwaysHasVelocity(), ViewIndex, Collector);
				}

				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}


#if RHI_RAYTRACING
	virtual bool IsRayTracingRelevant() const override { return true; }

	virtual bool HasRayTracingRepresentation() const override { return true; }

	virtual void GetDynamicRayTracingInstances(FRayTracingInstanceCollector& Collector) override final
	{
		//if (!CVarRayTracingProceduralMesh.GetValueOnRenderThread())
		//{
		//	return;
		//}

		//TConstArrayView<const FSceneView*> Views = Collector.GetViews();
		//const uint32 VisibilityMap = Collector.GetVisibilityMap();

		//// RT geometry will be generated based on first active view and then reused for all other views
		//// TODO: Expose a way for developers to control whether to reuse RT geometry or create one per-view
		//const int32 FirstActiveViewIndex = FMath::CountTrailingZeros(VisibilityMap);
		//checkf(Views.IsValidIndex(FirstActiveViewIndex), TEXT("There should be at least one active view when calling GetDynamicRayTracingInstances(...)."));

		//const FSceneView* FirstActiveView = Views[FirstActiveViewIndex];

		//for (int32 SegmentIndex = 0; SegmentIndex < Sections.Num(); ++SegmentIndex)
		//{
		//	const FProcMeshProxySection* Section = Sections[SegmentIndex];
		//	if (Section != nullptr && Section->bSectionVisible)
		//	{
		//		FMaterialRenderProxy* MaterialProxy = Section->Material->GetRenderProxy();

		//		if (Section->RayTracingGeometry.IsValid())
		//		{
		//			check(Section->RayTracingGeometry.Initializer.IndexBuffer.IsValid());

		//			FRayTracingInstance RayTracingInstance;
		//			RayTracingInstance.Geometry = &Section->RayTracingGeometry;
		//			RayTracingInstance.InstanceTransforms.Add(GetLocalToWorld());

		//			uint32 SectionIdx = 0;
		//			FMeshBatch MeshBatch;

		//			MeshBatch.VertexFactory = &Section->VertexFactory;
		//			MeshBatch.SegmentIndex = 0;
		//			MeshBatch.MaterialRenderProxy = Section->Material->GetRenderProxy();
		//			MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
		//			MeshBatch.Type = PT_TriangleList;
		//			MeshBatch.DepthPriorityGroup = SDPG_World;
		//			MeshBatch.bCanApplyViewModeOverrides = false;
		//			MeshBatch.CastRayTracedShadow = IsShadowCast(FirstActiveView);

		//			FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		//			BatchElement.IndexBuffer = &Section->IndexBuffer;

		//			FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
		//			FPrimitiveUniformShaderParametersBuilder Builder;
		//			BuildUniformShaderParameters(Builder);
		//			DynamicPrimitiveUniformBuffer.Set(Collector.GetRHICommandList(), Builder);

		//			BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

		//			BatchElement.FirstIndex = 0;
		//			BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
		//			BatchElement.MinVertexIndex = 0;
		//			BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;

		//			RayTracingInstance.Materials.Add(MeshBatch);

		//			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		//			{
		//				if ((VisibilityMap & (1 << ViewIndex)) == 0)
		//				{
		//					continue;
		//				}

		//				Collector.AddRayTracingInstance(ViewIndex, RayTracingInstance);
		//			}
		//		}
		//	}
		//}
	}

#endif

private:

	MeshChunk Chunk;

	TArray<FNiMeshProxySection*> Sections;

	UBodySetup* BodySetup;

	FMaterialRelevance MaterialRelevance;

	TArray<uint32> ProcIndexBuffer;
};



UNiMeshComponent::UNiMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//bUseComplexAsSimpleCollision = true;
}

void UNiMeshComponent::PostLoad()
{
	Super::PostLoad();

	//if (ProcMeshBodySetup && IsTemplate())
	//{
	//	ProcMeshBodySetup->SetFlags(RF_Public | RF_ArchetypeObject);
	//}
}
FPrimitiveSceneProxy* UNiMeshComponent::CreateSceneProxy()
{
	SCOPE_CYCLE_COUNTER(STAT_NiMeshComponent_CreateSceneProxy);
	UE_LOG(LogTemp, Log, TEXT("CreateSceneProxy 1"));
	return new FNiMeshSceneProxy(this);
	//return nullptr;
}
int32 UNiMeshComponent::GetNumMaterials() const
{
	return 0;
	//return ProcMeshSections.Num();
}

FBoxSphereBounds UNiMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FSphere(FVector(0,0,0), 100));
	//FBoxSphereBounds Ret(LocalBounds.TransformBy(LocalToWorld));
	//
	//Ret.BoxExtent *= BoundsScale;
	//Ret.SphereRadius *= BoundsScale;
	//
	//return Ret;
}
UMaterialInterface* UNiMeshComponent::GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const
{
	return nullptr;
	//UMaterialInterface* Result = nullptr;
	//SectionIndex = 0;
	//
	//if (FaceIndex >= 0)
	//{
	//	// Look for element that corresponds to the supplied face
	//	int32 TotalFaceCount = 0;
	//	for (int32 SectionIdx = 0; SectionIdx < ProcMeshSections.Num(); SectionIdx++)
	//	{
	//		const FProcMeshSection& Section = ProcMeshSections[SectionIdx];
	//		int32 NumFaces = Section.ProcIndexBuffer.Num() / 3;
	//		TotalFaceCount += NumFaces;
	//
	//		if (FaceIndex < TotalFaceCount)
	//		{
	//			// Grab the material
	//			Result = GetMaterial(SectionIdx);
	//			SectionIndex = SectionIdx;
	//			break;
	//		}
	//	}
	//}
	//
	//return Result;
}
UBodySetup* UNiMeshComponent::GetBodySetup()
{
	return nullptr;
	//CreateProcMeshBodySetup();
	//return ProcMeshBodySetup;
}