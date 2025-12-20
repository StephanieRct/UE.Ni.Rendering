
#pragma once

#include "CoreMinimal.h"
#include "Ni/Public/Ni.h"

namespace NiT::Rendering
{
	struct Policy 
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

	template<typename TPolicy>
	struct CoVertexPosition3 : public Ni::NodeComponent
	{
		using Policy_t = TPolicy;
		typename Policy_t::Position3_t Position;
	};

	template<typename TPolicy>
	struct CoVertexNormal3 : public Ni::NodeComponent
	{
		using Policy_t = TPolicy;
		typename Policy_t::Normal3_t Normal;
	};

	template<typename TPolicy, typename TPolicy::Size_t TChannel>
	struct CoVertexUV : public Ni::NodeComponent
	{
		using Policy_t = TPolicy;
		typename Policy_t::UV_t UV;
	};

	template<typename TPolicy, typename TPolicy::Size_t TChannel>
	struct CoVertexColor : public Ni::NodeComponent
	{
		using Policy_t = TPolicy;
		typename Policy_t::Color_t Color;
	};

	template<typename TPolicy>
	struct CoVertexTangent3 : public Ni::NodeComponent
	{
		using Policy_t = TPolicy;
		typename Policy_t::Tangent3_t Tangent;
	};

	template<typename TIndex>
	struct CoIndex : public Ni::NodeComponent
	{
		TIndex Index;
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