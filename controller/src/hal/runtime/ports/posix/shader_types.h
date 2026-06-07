#ifndef SRC__HAL__RUNTIME__PORTS__PICO__SHADER_TYPES__H_
#define SRC__HAL__RUNTIME__PORTS__PICO__SHADER_TYPES__H_

#include <simd/simd.h>

namespace shader_types {
struct VertexData {
  simd::float3 position;
  simd::float3 normal;
};

struct InstanceData {
  simd::float4x4 instanceTransform;
  simd::float3x3 instanceNormalTransform;
  simd::float4 instanceColor;
};

struct CameraData {
  simd::float4x4 perspectiveTransform;
  simd::float4x4 worldTransform;
  simd::float3x3 worldNormalTransform;
};
} // namespace shader_types

#endif // SRC__HAL__RUNTIME__PORTS__PICO__SHADER_TYPES__H_