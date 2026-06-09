#ifndef SRC__RUNTIME__SIMULATOR__RENDERER__H_
#define SRC__RUNTIME__SIMULATOR__RENDERER__H_

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

#include "overlay_renderer.h"

static constexpr size_t kMaxFramesInFlight = 3;

class Renderer {
public:
  Renderer(MTL::Device *pDevice, size_t numInstances);
  ~Renderer();
  void buildShaders();
  void buildDepthStencilStates();
  void buildBuffers();
  void buildHatGeometry();
  void draw(MTK::View *pView);

private:
  MTL::Buffer *getInstanceDataBuffers();
  MTL::Buffer *getHatInstanceBuffer();
  MTL::Buffer *getCameraBuffer();
  simd::float4x4 fullObjectRotation(const simd::float3 &objectPosition) const;

  MTL::Device *_pDevice;
  MTL::CommandQueue *_pCommandQueue;
  MTL::Library *_pShaderLibrary;
  MTL::RenderPipelineState *_pPSO;
  MTL::DepthStencilState *_pDepthStencilState;
  MTL::Buffer *_pVertexDataBuffer;
  MTL::Buffer *_pInstanceDataBuffer[kMaxFramesInFlight];
  MTL::Buffer *_pCameraDataBuffer[kMaxFramesInFlight];
  MTL::Buffer *_pIndexBuffer;

  // Top-hat mesh drawn once behind the LED band (32-bit indices; see
  // mesh_loader). _crownRadius / _bandY come from the loaded mesh and place the
  // LEDs as a glowing hatband on the crown.
  MTL::Buffer *_pHatVertexBuffer;
  MTL::Buffer *_pHatIndexBuffer;
  MTL::Buffer *_pHatInstanceBuffer[kMaxFramesInFlight];
  size_t _hatIndexCount;
  float _crownRadius;
  float _bandY;

  size_t _numInstances;
  float _angle;
  int _frame;
  dispatch_semaphore_t _semaphore;
  static const int kMaxFramesInFlight;

  OverlayRenderer *_pOverlayRenderer;
  int _overlayUpdateCounter;
};

#endif // SRC__RUNTIME__SIMULATOR__RENDERER__H_