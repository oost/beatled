#ifndef SRC__HAL__RUNTIME__PORTS__POSIX__OVERLAY_RENDERER__H_
#define SRC__HAL__RUNTIME__PORTS__POSIX__OVERLAY_RENDERER__H_

#include <Metal/Metal.hpp>

#include "status_buffer.h"

class OverlayRenderer {
public:
  OverlayRenderer(MTL::Device *pDevice, MTL::Library *pShaderLibrary);
  ~OverlayRenderer();

  void updateTexture(const StatusData &status);
  void draw(MTL::RenderCommandEncoder *pEnc);

private:
  void buildPipeline(MTL::Library *pShaderLibrary);
  void buildQuadBuffers();
  void buildTexture();

  MTL::Device *_pDevice;
  MTL::RenderPipelineState *_pOverlayPSO;
  MTL::DepthStencilState *_pOverlayDepthState;
  MTL::Texture *_pOverlayTexture;
  MTL::Buffer *_pQuadVertexBuffer;

  static const int kOverlayWidth = 280;
  static const int kOverlayHeight = 180;
};

#endif // SRC__HAL__RUNTIME__PORTS__POSIX__OVERLAY_RENDERER__H_
