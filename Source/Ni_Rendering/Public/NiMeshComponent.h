// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Ni/Ni.h"

#include "NiMeshComponent.generated.h"

//
//template<typename TChunkPointer>
//TChunkPointer MakeChunkPointer(const FStaticMeshVertexBuffers& buffers)
//{
//    buffers.
//    TChunkPointer::GetInternalChunk()
//    return 0;
//}
/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNiMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
public:
	NI_RENDERING_API UNiMeshComponent(const FObjectInitializer& ObjectInitializer);
	
	

	//~ Begin UPrimitiveComponent Interface.
	NI_RENDERING_API virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	NI_RENDERING_API virtual class UBodySetup* GetBodySetup() override;
	NI_RENDERING_API virtual UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	NI_RENDERING_API virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin UObject Interface
	NI_RENDERING_API virtual void PostLoad() override;
	//~ End UObject Interface.

	//~ Begin USceneComponent Interface.
	NI_RENDERING_API virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

public:

	UPROPERTY()
	float SomeValue;
};
