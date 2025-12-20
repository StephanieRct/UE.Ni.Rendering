
#pragma once

#include "CoreMinimal.h"
#include "Ni/Public/Ni.h"

namespace NiT::Rendering
{
	template<typename TPolicy>
	struct CoMeshSection : public Ni::NodeComponent
	{
		using Policy_t = TPolicy;
		using IndexChunk_t = typename Policy_t::IndexChunk_t;
		using VertexChunk_t = typename Policy_t::VertexChunk_t;
		IndexChunk_t Index;
		VertexChunk_t Vertex;
	};

	struct CoMultiSectionMesh : public Ni::ChunkComponent
	{
	public:
		const Ni::ChunkStructure* DefaultIndexStructure;
		const Ni::ChunkStructure* DefaultVertexStructure;

		CoMultiSectionMesh(
			const Ni::ChunkStructure* defaultIndexStructure,
			const Ni::ChunkStructure* defaultVertexStructure)
			: DefaultIndexStructure(defaultIndexStructure)
			, DefaultVertexStructure(defaultVertexStructure)
		{
		}
	};

	struct MeshPolicy
	{
		using Policy_t = Policy;
		using Index_t = uint32;
		using IndexChunk_t = Ni::NBucket;
		using VertexChunk_t = Ni::NBucket;
		using CoMultiSectionMesh_t = CoMultiSectionMesh;
		using CoMeshSection_t = CoMeshSection<MeshPolicy>;
	};

	template<typename TMeshPolicy = MeshPolicy, typename TBase = Ni::NBucket>
	struct DMeshChunk : public TBase //public Ni::NBucket
	{
	public:
		using Base_t = TBase;// Ni::NBucket;
		using Self_t = DMeshChunk<TMeshPolicy, TBase>;
		using MeshPolicy_t =			TMeshPolicy;
		using Policy_t =				typename MeshPolicy_t::Policy_t;
		using Size_t =					Base_t::Size_t;
		using ChunkStructure_t =		Base_t::ChunkStructure_t;
		using Real_t =					typename Policy_t::Real_t;
		using Real2_t =					typename Policy_t::Real2_t;
		using Real3_t =					typename Policy_t::Real3_t;
		using Position3_t =				typename Policy_t::Position3_t;
		using UV_t =					typename Policy_t::UV_t;
		using Normal3_t =				typename Policy_t::Normal3_t;
		using Tangent3_t =				typename Policy_t::Tangent3_t;
		using Color_t =					typename Policy_t::Color_t;
		using Index_t =					typename MeshPolicy_t::Index_t;
		using IndexChunk_t =			typename MeshPolicy_t::IndexChunk_t;
		using VertexChunk_t =			typename MeshPolicy_t::VertexChunk_t;
		using CoMultiSectionMesh_t =	typename MeshPolicy_t::CoMultiSectionMesh_t;
		using CoMeshSection_t =			typename MeshPolicy_t::CoMeshSection_t;
		using CoVertexPosition3_t =		CoVertexPosition3<Policy_t>;
		using CoVertexNormal3_t =		CoVertexNormal3<Policy_t>;
		using CoVertexUV0_t =			CoVertexUV<Policy_t, 0>;
		using CoVertexColor0_t =		CoVertexColor<Policy_t, 0>;
		using CoVertexTangent3_t =		CoVertexTangent3<Policy_t>;
		using CoIndex_t =				CoIndex<Index_t>;

		using Base_t::GetComponentData;
		using Base_t::AddNode;
		using Base_t::AddNodes;
	public:
		DMeshChunk(
			const ChunkStructure_t* meshStructure,
			const ChunkStructure_t* indexStructure,
			const ChunkStructure_t* vertexStructure,
			const NiT::NodeCapacityT<Size_t> sectionCapacity,
			const NiT::NodeCountT<Size_t> sectionCount = NiT::NodeCountT<Size_t>::V_0())
			: Base_t(meshStructure, sectionCapacity, sectionCount)
		{
			new(&GetMesh()) CoMultiSectionMesh_t(indexStructure, vertexStructure);

		}

		DMeshChunk(const Size_t sectionCapacity, const Size_t sectionCount = 0)
			: Base_t(
				ChunkStructure_t::Registry.template GetOrAddChunkStructure<CoMultiSectionMesh_t, CoMeshSection_t>(),
				sectionCapacity,
				sectionCount)
		{
			CoMultiSectionMesh_t* mesh = &GetMesh();
			const auto* indexStructure = ChunkStructure_t::Registry.template GetOrAddChunkStructure<CoMultiSectionMesh_t, CoMeshSection_t>();
			const auto* vertexStructure = ChunkStructure_t::Registry.template GetOrAddChunkStructure<CoMultiSectionMesh_t, CoMeshSection_t>();
			new(mesh) CoMultiSectionMesh_t(indexStructure, vertexStructure);
		}

		DMeshChunk(const DMeshChunk& other) = delete; // TODO
		DMeshChunk(const DMeshChunk&& other) = delete; // TODO
		DMeshChunk& operator=(const DMeshChunk& other) = delete; // TODO


		Size_t AddSection(const Size_t indexCapacity, const Size_t vertexCapacity, const Size_t indexCount = 0, const Size_t vertexCount = 0)
		{
			Size_t index = AddNode();
			CoMultiSectionMesh_t& mesh = GetMesh();
			CoMeshSection_t& section = GetSections()[index];
			section.Index = IndexChunk_t(mesh.DefaultIndexStructure, indexCapacity, indexCount);
			section.Vertex = VertexChunk_t(mesh.DefaultVertexStructure, vertexCapacity, vertexCount);
			return index;
		}
		const CoMeshSection_t* GetSections()const { return this->GetComponentData<CoMeshSection_t>(); }
			  CoMeshSection_t* GetSections()	  { return this->GetComponentData<CoMeshSection_t>(); }








		IndexChunk_t& GetIndexChunk(CoMeshSection_t& section)
		{
			return section.Index;
		}
		VertexChunk_t& GetVertexChunk(CoMeshSection_t& section)
		{
			return section.Index;
		}

		//Ni::ChunkPointer& GetIndexChunk(const Size_t sectionIndex)
		//{
		//	CoMultiSectionMesh& mesh = GetMesh();
		//	CoMeshSection& section = mesh.MeshSection[sectionIndex];
		//	return *section.Index;
		//}

		//Ni::ChunkPointer& GetVertexChunk(const Size_t sectionIndex)
		//{
		//	CoMultiSectionMesh& mesh = GetMesh();
		//	CoMeshSection& section = mesh.MeshSection[sectionIndex];
		//	return *section.Vertex;
		//}


		//void Upload(UProceduralMeshComponent& proceduralMeshComponent, const Ni::NChunkPointer& chunk)
		//{
		//	proceduralMeshComponent.ClearAllMeshSections();
		//	const CoMeshSection_t* sections = chunk.GetComponentData<CoMeshSection_t>();
		//	for (Ni::Size_t i = 0; i < chunk.GetNodeCount(); ++i)
		//	{
		//		const Ni::NChunkPointer& indexChunk = sections[i].Index;
		//		int32 indexCount = (int32)indexChunk.GetNodeCount(); //TODO test size type conversion fits

		//		const Ni::NChunkPointer& vertexChunk = sections[i].Vertex;
		//		int32 vertexCount = (int32)vertexChunk.GetNodeCount(); //TODO test size type conversion fits
		//		const auto * position3 = vertexChunk.GetComponentData<CoVertexPosition3_t>();
		//		const auto * normal3 = vertexChunk.GetComponentData<CoVertexNormal3_t>();
		//		const auto * uv0 = vertexChunk.GetComponentData<CoVertexUV0_t>();
		//		const auto * color0 = vertexChunk.GetComponentData<CoVertexColor0_t>();
		//		const auto * tangent3 = vertexChunk.GetComponentData<CoVertexTangent3_t>();
		//		//  CoVertexPosition3_t
		//		//	CoVertexNormal3_t
		//		//	CoVertexUV0_t
		//		//	CoVertexColor0_t
		//		//	CoTangent3_t
		//		//	CoIndex_t

		//		const auto* triangle = indexChunk.GetComponentData<CoIndex_t>();
		//		proceduralMeshComponent.CreateMeshSection(
		//			i, // SectionIndex
		//			TArray<FVector>(&position3.Position, vertexCount),
		//			TArray<int32>(&triangle.Index, indexCount),
		//			TArray<FVector>(&normal3.Normal, vertexCount),
		//			TArray<FVector2D>(&uv0.UV, vertexCount),
		//			TArray<FColor>(&color0.Color, vertexCount),
		//			TArray<FProcMeshTangent>(&tangent3.Tangent, vertexCount),
		//			false // bCreateCollision
		//		);
		//	}
		//}


	protected:
		CoMultiSectionMesh_t& GetMesh()
		{
			return *this->GetComponentData<CoMultiSectionMesh_t>();
		}
	};

	using MeshChunk = DMeshChunk<MeshPolicy, Ni::NBucket>;
}