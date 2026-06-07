#include <simd/simd.h>

#include "led_buffer.h"
#include "math.h"
#include "renderer.h"
#include "shader_types.h"
#include "shaders/triangle.metal.h"
#include "status_buffer.h"

const int Renderer::kMaxFramesInFlight = 3;

Renderer::Renderer(MTL::Device *pDevice, size_t numInstances)
    : _pDevice(pDevice->retain()), _numInstances(numInstances), _angle(0.f),
      _frame(0), _pOverlayRenderer(nullptr), _overlayUpdateCounter(0) {
  _pCommandQueue = _pDevice->newCommandQueue();
  buildShaders();
  buildDepthStencilStates();
  buildBuffers();

  _pOverlayRenderer = new OverlayRenderer(_pDevice, _pShaderLibrary);

  _semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);
}

Renderer::~Renderer() {
  delete _pOverlayRenderer;
  _pShaderLibrary->release();
  _pDepthStencilState->release();
  _pVertexDataBuffer->release();
  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    _pInstanceDataBuffer[i]->release();
  }
  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    _pCameraDataBuffer[i]->release();
  }
  _pIndexBuffer->release();
  _pPSO->release();
  _pCommandQueue->release();
  _pDevice->release();
}

void Renderer::buildShaders() {
  using NS::StringEncoding::UTF8StringEncoding;

  const char *shaderSrc = reinterpret_cast<char *>(shaders_triangle_metal);

  NS::Error *pError = nullptr;
  MTL::Library *pLibrary = _pDevice->newLibrary(
      NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError);
  if (!pLibrary) {
    __builtin_printf("%s", pError->localizedDescription()->utf8String());
    assert(false);
  }

  MTL::Function *pVertexFn = pLibrary->newFunction(
      NS::String::string("vertexMain", UTF8StringEncoding));
  MTL::Function *pFragFn = pLibrary->newFunction(
      NS::String::string("fragmentMain", UTF8StringEncoding));

  MTL::RenderPipelineDescriptor *pDesc =
      MTL::RenderPipelineDescriptor::alloc()->init();
  pDesc->setVertexFunction(pVertexFn);
  pDesc->setFragmentFunction(pFragFn);
  pDesc->colorAttachments()->object(0)->setPixelFormat(
      MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
  pDesc->setDepthAttachmentPixelFormat(
      MTL::PixelFormat::PixelFormatDepth16Unorm);

  _pPSO = _pDevice->newRenderPipelineState(pDesc, &pError);
  if (!_pPSO) {
    __builtin_printf("%s", pError->localizedDescription()->utf8String());
    assert(false);
  }

  pVertexFn->release();
  pFragFn->release();
  pDesc->release();
  _pShaderLibrary = pLibrary;
}

void Renderer::buildDepthStencilStates() {
  MTL::DepthStencilDescriptor *pDsDesc =
      MTL::DepthStencilDescriptor::alloc()->init();
  pDsDesc->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
  pDsDesc->setDepthWriteEnabled(true);

  _pDepthStencilState = _pDevice->newDepthStencilState(pDsDesc);

  pDsDesc->release();
}
void Renderer::buildBuffers() {
  using simd::float3;
  const float s = 0.5f;

  shader_types::VertexData verts[] = {
      //   Positions          Normals
      {{-s, -s, +s}, {0.f, 0.f, 1.f}},  {{+s, -s, +s}, {0.f, 0.f, 1.f}},
      {{+s, +s, +s}, {0.f, 0.f, 1.f}},  {{-s, +s, +s}, {0.f, 0.f, 1.f}},

      {{+s, -s, +s}, {1.f, 0.f, 0.f}},  {{+s, -s, -s}, {1.f, 0.f, 0.f}},
      {{+s, +s, -s}, {1.f, 0.f, 0.f}},  {{+s, +s, +s}, {1.f, 0.f, 0.f}},

      {{+s, -s, -s}, {0.f, 0.f, -1.f}}, {{-s, -s, -s}, {0.f, 0.f, -1.f}},
      {{-s, +s, -s}, {0.f, 0.f, -1.f}}, {{+s, +s, -s}, {0.f, 0.f, -1.f}},

      {{-s, -s, -s}, {-1.f, 0.f, 0.f}}, {{-s, -s, +s}, {-1.f, 0.f, 0.f}},
      {{-s, +s, +s}, {-1.f, 0.f, 0.f}}, {{-s, +s, -s}, {-1.f, 0.f, 0.f}},

      {{-s, +s, +s}, {0.f, 1.f, 0.f}},  {{+s, +s, +s}, {0.f, 1.f, 0.f}},
      {{+s, +s, -s}, {0.f, 1.f, 0.f}},  {{-s, +s, -s}, {0.f, 1.f, 0.f}},

      {{-s, -s, -s}, {0.f, -1.f, 0.f}}, {{+s, -s, -s}, {0.f, -1.f, 0.f}},
      {{+s, -s, +s}, {0.f, -1.f, 0.f}}, {{-s, -s, +s}, {0.f, -1.f, 0.f}},
  };

  uint16_t indices[] = {
      0,  1,  2,  2,  3,  0,  /* front */
      4,  5,  6,  6,  7,  4,  /* right */
      8,  9,  10, 10, 11, 8,  /* back */
      12, 13, 14, 14, 15, 12, /* left */
      16, 17, 18, 18, 19, 16, /* top */
      20, 21, 22, 22, 23, 20, /* bottom */
  };

  const size_t vertexDataSize = sizeof(verts);
  const size_t indexDataSize = sizeof(indices);

  MTL::Buffer *pVertexBuffer =
      _pDevice->newBuffer(vertexDataSize, MTL::ResourceStorageModeManaged);
  MTL::Buffer *pIndexBuffer =
      _pDevice->newBuffer(indexDataSize, MTL::ResourceStorageModeManaged);

  _pVertexDataBuffer = pVertexBuffer;
  _pIndexBuffer = pIndexBuffer;

  memcpy(_pVertexDataBuffer->contents(), verts, vertexDataSize);
  memcpy(_pIndexBuffer->contents(), indices, indexDataSize);

  _pVertexDataBuffer->didModifyRange(
      NS::Range::Make(0, _pVertexDataBuffer->length()));
  _pIndexBuffer->didModifyRange(NS::Range::Make(0, _pIndexBuffer->length()));

  const size_t instanceDataSize =
      kMaxFramesInFlight * _numInstances * sizeof(shader_types::InstanceData);
  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    _pInstanceDataBuffer[i] =
        _pDevice->newBuffer(instanceDataSize, MTL::ResourceStorageModeManaged);
  }

  const size_t cameraDataSize =
      kMaxFramesInFlight * sizeof(shader_types::CameraData);
  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    _pCameraDataBuffer[i] =
        _pDevice->newBuffer(cameraDataSize, MTL::ResourceStorageModeManaged);
  }
}

MTL::Buffer *Renderer::getInstanceDataBuffers() {
  LEDBuffer::Ptr led_data = LEDBuffer::load_instance();

  using simd::float3;
  using simd::float4;
  using simd::float4x4;

  MTL::Buffer *pInstanceDataBuffer = _pInstanceDataBuffer[_frame];

  const float scl = 0.1f;
  shader_types::InstanceData *pInstanceData =
      reinterpret_cast<shader_types::InstanceData *>(
          pInstanceDataBuffer->contents());

  float3 objectPosition = {0.f, 0.f, -10.f};

  float4x4 rt = math::makeTranslate(objectPosition);
  float4x4 rr1 = math::makeYRotate(-_angle);
  float4x4 rr0 = math::makeXRotate(_angle * 0.5);
  float4x4 rtInv = math::makeTranslate(
      {-objectPosition.x, -objectPosition.y, -objectPosition.z});
  float4x4 fullObjectRot = rt * rr1 * rr0 * rtInv;

  size_t ix = 0;
  size_t iy = 0;
  size_t iz = 0;
  float radius = 1.5;
  LEDColor c;
  for (size_t i = 0; i < _numInstances; ++i) {
    float rot_angle = 2 * M_PI * (float)i / (float)_numInstances;

    float4x4 scale = math::makeScale((float3){scl, scl, scl});
    float4x4 zrot = math::makeZRotate(_angle * sinf((float)ix));
    float4x4 yrot = math::makeYRotate(_angle * cosf((float)iy));

    float x = radius * cos(rot_angle);
    float y = radius * sin(rot_angle);
    float z = 0;

    float4x4 translate =
        math::makeTranslate(math::add(objectPosition, {x, y, z}));

    pInstanceData[i].instanceTransform =
        fullObjectRot * translate * yrot * zrot * scale;
    pInstanceData[i].instanceNormalTransform =
        math::discardTranslation(pInstanceData[i].instanceTransform);

    float iDivNumInstances = i / (float)_numInstances;
    float r = iDivNumInstances;
    float g = 1.0f - r;
    float b = sinf(M_PI * 2.0f * iDivNumInstances);

    c = led_data->at(i);
    pInstanceData[i].instanceColor = (float4){c.red, c.green, c.blue, 1.0f};
    // pInstanceData[i].instanceColor = (float4){
    //     static_cast<float>(c.red) / 255, static_cast<float>(c.green) / 255,
    //     static_cast<float>(c.blue) / 255, 1.0f};
    // pInstanceData[i].instanceColor = (float4){r, g, b, 1.0f};

    ix += 1;
  }
  pInstanceDataBuffer->didModifyRange(
      NS::Range::Make(0, pInstanceDataBuffer->length()));

  return pInstanceDataBuffer;
}

MTL::Buffer *Renderer::getCameraBuffer() {

  MTL::Buffer *pCameraDataBuffer = _pCameraDataBuffer[_frame];
  shader_types::CameraData *pCameraData =
      reinterpret_cast<shader_types::CameraData *>(
          pCameraDataBuffer->contents());
  pCameraData->perspectiveTransform =
      math::makePerspective(45.f * M_PI / 180.f, 1.f, 0.03f, 500.0f);
  pCameraData->worldTransform = math::makeIdentity();
  pCameraData->worldNormalTransform =
      math::discardTranslation(pCameraData->worldTransform);
  pCameraDataBuffer->didModifyRange(
      NS::Range::Make(0, sizeof(shader_types::CameraData)));
  return pCameraDataBuffer;
}

void Renderer::draw(MTK::View *pView) {

  NS::AutoreleasePool *pPool = NS::AutoreleasePool::alloc()->init();

  _frame = (_frame + 1) % Renderer::kMaxFramesInFlight;

  MTL::CommandBuffer *pCmd = _pCommandQueue->commandBuffer();
  dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
  Renderer *pRenderer = this;
  pCmd->addCompletedHandler(^void(MTL::CommandBuffer *pCmd) {
    dispatch_semaphore_signal(pRenderer->_semaphore);
  });

  _angle += 0.002f;

  // Update instance positions:
  MTL::Buffer *pInstanceDataBuffer = getInstanceDataBuffers();

  // Update camera state:
  MTL::Buffer *pCameraDataBuffer = getCameraBuffer();

  // Begin render pass:

  MTL::RenderPassDescriptor *pRpd = pView->currentRenderPassDescriptor();
  MTL::RenderCommandEncoder *pEnc = pCmd->renderCommandEncoder(pRpd);

  pEnc->setRenderPipelineState(_pPSO);
  pEnc->setDepthStencilState(_pDepthStencilState);

  pEnc->setVertexBuffer(_pVertexDataBuffer, /* offset */ 0, /* index */ 0);
  pEnc->setVertexBuffer(pInstanceDataBuffer, /* offset */ 0, /* index */ 1);
  pEnc->setVertexBuffer(pCameraDataBuffer, /* offset */ 0, /* index */ 2);

  pEnc->setCullMode(MTL::CullModeBack);
  pEnc->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);

  pEnc->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 6 * 6,
                              MTL::IndexType::IndexTypeUInt16, _pIndexBuffer, 0,
                              _numInstances);

  // Draw status overlay (update every 6 frames for ~10Hz refresh at 60 FPS)
  if (++_overlayUpdateCounter % 6 == 0) {
    StatusBuffer::Ptr statusBuf = StatusBuffer::load_instance();
    _pOverlayRenderer->updateTexture(statusBuf->data());
  }
  _pOverlayRenderer->draw(pEnc);

  pEnc->endEncoding();
  pCmd->presentDrawable(pView->currentDrawable());
  pCmd->commit();

  pPool->release();
}