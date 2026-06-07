#ifndef SRC__RUNTIME__SIMULATOR__RENDERER__H_
#define SRC__RUNTIME__SIMULATOR__RENDERER__H_

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "overlay_renderer.h"

static constexpr size_t kMaxFramesInFlight = 3;

class Renderer {
public:
  Renderer(MTL::Device *pDevice, size_t numInstances);
  ~Renderer();
  void buildShaders();
  void buildDepthStencilStates();
  void buildBuffers();
  void draw(MTK::View *pView);

private:
  MTL::Buffer *getInstanceDataBuffers();
  MTL::Buffer *getCameraBuffer();

  MTL::Device *_pDevice;
  MTL::CommandQueue *_pCommandQueue;
  MTL::Library *_pShaderLibrary;
  MTL::RenderPipelineState *_pPSO;
  MTL::DepthStencilState *_pDepthStencilState;
  MTL::Buffer *_pVertexDataBuffer;
  MTL::Buffer *_pInstanceDataBuffer[kMaxFramesInFlight];
  MTL::Buffer *_pCameraDataBuffer[kMaxFramesInFlight];
  MTL::Buffer *_pIndexBuffer;
  size_t _numInstances;
  float _angle;
  int _frame;
  dispatch_semaphore_t _semaphore;
  static const int kMaxFramesInFlight;

  OverlayRenderer *_pOverlayRenderer;
  int _overlayUpdateCounter;
};

#endif // SRC__RUNTIME__SIMULATOR__RENDERER__H_