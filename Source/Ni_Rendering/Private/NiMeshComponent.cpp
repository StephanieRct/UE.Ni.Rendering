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

struct NiMeshPolicy
{
	using Size_t = int32;
	using Real_t = float;
	using Real2_t = FVector2D;
	using Real3_t = FVector;
	using Color_t = FColor;
	using Position3_t = Real3_t;
	using UV_t = Real2_t;
	using Normal3_t = Real3_t;
	using Tangent3_t = Real3_t;
};

using CoVertexPosition3 = NiT::Rendering::CoVertexPosition3<NiMeshPolicy>;

//class FNiVertexBuffer : public FVertexBuffer
//{
//public:
//	Ni::NChunkPointer Chunk;
//
//	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
//	{
//		// Create the buffer resource in the render thread
//		FRHIResourceCreateInfo CreateInfo(TEXT("MyCustomVertexBuffer"));
//		VertexBufferRHI = RHICreateBuffer(
//			Chunk.GetNodeCount() * sizeof(FMyCustomVertex),
//			BUF_Static | BUF_VertexBuffer, // Use BUF_Dynamic if updating per frame
//			0,
//			ERHIAccess::VertexOrIndexBuffer,
//			CreateInfo
//		);
//
//		// Copy the data to the GPU
//		void* VertexBufferData = RHILockBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FMyCustomVertex), RHI_LOCK_WRITE);
//		FMemory::Memcpy(VertexBufferData, Vertices.GetData(), Vertices.Num() * sizeof(FMyCustomVertex));
//		RHIUnlockBuffer(VertexBufferRHI);
//	}
//
//	// Don't forget to override ReleaseRHI to clean up the resource
//	virtual void ReleaseRHI() override
//	{
//		//Vertices.Empty();
//		FVertexBuffer::ReleaseRHI();
//	}
//};

/** Class representing a single section of the proc mesh */
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
		, BodySetup(Component->GetBodySetup())
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetShaderPlatform()))
	{

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