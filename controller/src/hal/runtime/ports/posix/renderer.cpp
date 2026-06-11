#include <simd/simd.h>
#include <string>

#include "assets/top_hat.obj.h"
#include "led_buffer.h"
#include "math.h"
#include "mesh_loader.h"
#include "renderer.h"
#include "shader_types.h"
#include "shaders/triangle.metal.h"
#include "status_buffer.h"

const int Renderer::kMaxFramesInFlight = 3;

namespace {
// Scene-space size of the hat. The mesh is canonically scaled to a max extent
// of 2 in mesh_loader, so the hat spans roughly +/-kHatScale around its center.
constexpr float kHatScale = 2.0f;
// Seat the LED band on the crown surface (slightly inside so the cubes hug it
// rather than floating off the felt).
constexpr float kBandRadiusMargin = 0.97f;
// Drop the band down the crown toward the brim, in scene units. The mesh's
// detected band sits mid-crown; this lowers it to where a real hatband rides.
constexpr float kBandHeightDrop = 0.35f;
// Dark felt colour for the hat body.
constexpr simd::float4 kHatColor = {0.05f, 0.05f, 0.06f, 1.0f};
// Orbit-drag sensitivity: radians of hat rotation per pixel of mouse travel.
constexpr float kDragRadiansPerPixel = 0.01f;
} // namespace

Renderer::Renderer(MTL::Device *pDevice, size_t numInstances)
    : _pDevice(pDevice->retain()), _numInstances(numInstances), _yaw(0.f), _pitch(0.f),
      _dragging(false), _frame(0), _pOverlayRenderer(nullptr), _overlayUpdateCounter(0) {
  _pCommandQueue = _pDevice->newCommandQueue();
  buildShaders();
  buildDepthStencilStates();
  buildBuffers();
  buildHatGeometry();

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
  _pHatVertexBuffer->release();
  _pHatIndexBuffer->release();
  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    _pHatInstanceBuffer[i]->release();
  }
  _pPSO->release();
  _pCommandQueue->release();
  _pDevice->release();
}

void Renderer::buildShaders() {
  using NS::StringEncoding::UTF8StringEncoding;

  const char *shaderSrc = reinterpret_cast<char *>(shaders_triangle_metal);

  NS::Error *pError = nullptr;
  MTL::Library *pLibrary =
      _pDevice->newLibrary(NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError);
  if (!pLibrary) {
    __builtin_printf("%s", pError->localizedDescription()->utf8String());
    assert(false);
  }

  MTL::Function *pVertexFn =
      pLibrary->newFunction(NS::String::string("vertexMain", UTF8StringEncoding));
  MTL::Function *pFragFn =
      pLibrary->newFunction(NS::String::string("fragmentMain", UTF8StringEncoding));

  MTL::RenderPipelineDescriptor *pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
  pDesc->setVertexFunction(pVertexFn);
  pDesc->setFragmentFunction(pFragFn);
  pDesc->colorAttachments()->object(0)->setPixelFormat(
      MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
  pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);

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
  MTL::DepthStencilDescriptor *pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();
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

  MTL::Buffer *pVertexBuffer = _pDevice->newBuffer(vertexDataSize, MTL::ResourceStorageModeManaged);
  MTL::Buffer *pIndexBuffer = _pDevice->newBuffer(indexDataSize, MTL::ResourceStorageModeManaged);

  _pVertexDataBuffer = pVertexBuffer;
  _pIndexBuffer = pIndexBuffer;

  memcpy(_pVertexDataBuffer->contents(), verts, vertexDataSize);
  memcpy(_pIndexBuffer->contents(), indices, indexDataSize);

  _pVertexDataBuffer->didModifyRange(NS::Range::Make(0, _pVertexDataBuffer->length()));
  _pIndexBuffer->didModifyRange(NS::Range::Make(0, _pIndexBuffer->length()));

  const size_t instanceDataSize =
      kMaxFramesInFlight * _numInstances * sizeof(shader_types::InstanceData);
  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    _pInstanceDataBuffer[i] =
        _pDevice->newBuffer(instanceDataSize, MTL::ResourceStorageModeManaged);
  }

  const size_t cameraDataSize = kMaxFramesInFlight * sizeof(shader_types::CameraData);
  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    _pCameraDataBuffer[i] = _pDevice->newBuffer(cameraDataSize, MTL::ResourceStorageModeManaged);
  }
}

void Renderer::buildHatGeometry() {
  std::string objText(reinterpret_cast<const char *>(assets_top_hat_obj), assets_top_hat_obj_len);
  mesh_loader::LoadedMesh mesh = mesh_loader::loadObjFromString(objText);

  _crownRadius = mesh.crownRadius;
  _bandY = mesh.bandY;
  _hatIndexCount = mesh.indices.size();

  const size_t vertexDataSize = mesh.vertices.size() * sizeof(shader_types::VertexData);
  const size_t indexDataSize = mesh.indices.size() * sizeof(uint32_t);

  _pHatVertexBuffer = _pDevice->newBuffer(vertexDataSize, MTL::ResourceStorageModeManaged);
  _pHatIndexBuffer = _pDevice->newBuffer(indexDataSize, MTL::ResourceStorageModeManaged);

  memcpy(_pHatVertexBuffer->contents(), mesh.vertices.data(), vertexDataSize);
  memcpy(_pHatIndexBuffer->contents(), mesh.indices.data(), indexDataSize);
  _pHatVertexBuffer->didModifyRange(NS::Range::Make(0, _pHatVertexBuffer->length()));
  _pHatIndexBuffer->didModifyRange(NS::Range::Make(0, _pHatIndexBuffer->length()));

  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    _pHatInstanceBuffer[i] =
        _pDevice->newBuffer(sizeof(shader_types::InstanceData), MTL::ResourceStorageModeManaged);
  }
}

// The whole scene slowly tumbles about the object's position. Both the LED
// band and the hat share this transform so they spin together.
simd::float4x4 Renderer::fullObjectRotation(const simd::float3 &objectPosition) const {
  using simd::float4x4;
  float4x4 rt = math::makeTranslate(objectPosition);
  float4x4 rr1 = math::makeYRotate(_yaw);
  float4x4 rr0 = math::makeXRotate(_pitch);
  float4x4 rtInv = math::makeTranslate({-objectPosition.x, -objectPosition.y, -objectPosition.z});
  return rt * rr1 * rr0 * rtInv;
}

void Renderer::beginDrag() {
  _dragging = true;
}

void Renderer::dragBy(float dxPixels, float dyPixels) {
  // NSEvent deltaY > 0 means the mouse moved down; pitching the hat backward
  // (top toward the viewer) for a downward drag makes it follow the cursor.
  _yaw += dxPixels * kDragRadiansPerPixel;
  _pitch -= dyPixels * kDragRadiansPerPixel;
}

void Renderer::endDrag() {
  _dragging = false;
}

MTL::Buffer *Renderer::getHatInstanceBuffer() {
  using simd::float3;
  using simd::float4x4;

  MTL::Buffer *pBuffer = _pHatInstanceBuffer[_frame];
  shader_types::InstanceData *pInstanceData =
      reinterpret_cast<shader_types::InstanceData *>(pBuffer->contents());

  float3 objectPosition = {0.f, 0.f, -10.f};
  // Mesh is canonically centred and crown already points +Y, so no reorient is
  // needed -- just place at the scene centre and scale.
  float4x4 transform = fullObjectRotation(objectPosition) * math::makeTranslate(objectPosition) *
                       math::makeScale({kHatScale, kHatScale, kHatScale});

  pInstanceData->instanceTransform = transform;
  pInstanceData->instanceNormalTransform = math::discardTranslation(transform);
  pInstanceData->instanceColor = kHatColor;

  pBuffer->didModifyRange(NS::Range::Make(0, pBuffer->length()));
  return pBuffer;
}

MTL::Buffer *Renderer::getInstanceDataBuffers() {
  LEDBuffer::Ptr led_data = LEDBuffer::load_instance();

  using simd::float3;
  using simd::float4;
  using simd::float4x4;

  MTL::Buffer *pInstanceDataBuffer = _pInstanceDataBuffer[_frame];

  const float scl = 0.1f;
  shader_types::InstanceData *pInstanceData =
      reinterpret_cast<shader_types::InstanceData *>(pInstanceDataBuffer->contents());

  float3 objectPosition = {0.f, 0.f, -10.f};

  float4x4 fullObjectRot = fullObjectRotation(objectPosition);

  // Wrap the LEDs horizontally around the crown so they read as a glowing
  // hatband. Radius/height come from the loaded hat mesh (canonical units),
  // scaled into the scene the same way the hat is. The band is rigid -- no
  // per-LED wobble -- so it sits on the crown like a real strip.
  const float bandRadius = _crownRadius * kHatScale * kBandRadiusMargin;
  const float bandHeight = _bandY * kHatScale - kBandHeightDrop;
  LEDColor c;
  for (size_t i = 0; i < _numInstances; ++i) {
    float rot_angle = 2 * M_PI * (float)i / (float)_numInstances;

    float4x4 scale = math::makeScale((float3){scl, scl, scl});

    float x = bandRadius * cosf(rot_angle);
    float z = bandRadius * sinf(rot_angle);
    float y = bandHeight;

    float4x4 translate = math::makeTranslate(math::add(objectPosition, {x, y, z}));

    pInstanceData[i].instanceTransform = fullObjectRot * translate * scale;
    pInstanceData[i].instanceNormalTransform =
        math::discardTranslation(pInstanceData[i].instanceTransform);

    c = led_data->at(i);
    pInstanceData[i].instanceColor = (float4){c.red, c.green, c.blue, 1.0f};
  }
  pInstanceDataBuffer->didModifyRange(NS::Range::Make(0, pInstanceDataBuffer->length()));

  return pInstanceDataBuffer;
}

MTL::Buffer *Renderer::getCameraBuffer() {

  MTL::Buffer *pCameraDataBuffer = _pCameraDataBuffer[_frame];
  shader_types::CameraData *pCameraData =
      reinterpret_cast<shader_types::CameraData *>(pCameraDataBuffer->contents());
  pCameraData->perspectiveTransform =
      math::makePerspective(45.f * M_PI / 180.f, 1.f, 0.03f, 500.0f);
  pCameraData->worldTransform = math::makeIdentity();
  pCameraData->worldNormalTransform = math::discardTranslation(pCameraData->worldTransform);
  pCameraDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::CameraData)));
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

  // Auto-tumble unless the user is orbiting with the mouse; on release the
  // tumble resumes from wherever the drag left the hat. The rates reproduce
  // the original single-angle tumble (yaw at -0.002, pitch at half speed).
  if (!_dragging) {
    _yaw -= 0.002f;
    _pitch += 0.001f;
  }

  // Update instance positions:
  MTL::Buffer *pInstanceDataBuffer = getInstanceDataBuffers();
  MTL::Buffer *pHatInstanceBuffer = getHatInstanceBuffer();

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
                              MTL::IndexType::IndexTypeUInt16, _pIndexBuffer, 0, _numInstances);

  // Draw the hat behind the band: same PSO/depth state, but its own vertex +
  // instance buffers and 32-bit indices. The camera buffer (index 2) is shared.
  pEnc->setVertexBuffer(_pHatVertexBuffer, /* offset */ 0, /* index */ 0);
  pEnc->setVertexBuffer(pHatInstanceBuffer, /* offset */ 0, /* index */ 1);
  pEnc->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, _hatIndexCount,
                              MTL::IndexType::IndexTypeUInt32, _pHatIndexBuffer, 0,
                              /* instanceCount */ 1);

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