#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cstdio>
#include <simd/simd.h>

#include "overlay_renderer.h"
#include "../../../../ws2812/ws2812_patterns.h"

struct OverlayVertex {
  simd::float2 position;
  simd::float2 texcoord;
};

static const char *kStateNames[] = {"UNKNOWN",    "STARTED",    "INITIALIZED",
                                    "REGISTERED", "TIME_SYNCED", "TEMPO_SYNCED"};

OverlayRenderer::OverlayRenderer(MTL::Device *pDevice,
                                 MTL::Library *pShaderLibrary)
    : _pDevice(pDevice->retain()), _pOverlayPSO(nullptr),
      _pOverlayDepthState(nullptr), _pOverlayTexture(nullptr),
      _pQuadVertexBuffer(nullptr) {
  buildPipeline(pShaderLibrary);
  buildQuadBuffers();
  buildTexture();
}

OverlayRenderer::~OverlayRenderer() {
  if (_pOverlayPSO)
    _pOverlayPSO->release();
  if (_pOverlayDepthState)
    _pOverlayDepthState->release();
  if (_pOverlayTexture)
    _pOverlayTexture->release();
  if (_pQuadVertexBuffer)
    _pQuadVertexBuffer->release();
  _pDevice->release();
}

void OverlayRenderer::buildPipeline(MTL::Library *pShaderLibrary) {
  using NS::StringEncoding::UTF8StringEncoding;

  MTL::Function *pVertexFn = pShaderLibrary->newFunction(
      NS::String::string("overlayVertexMain", UTF8StringEncoding));
  MTL::Function *pFragFn = pShaderLibrary->newFunction(
      NS::String::string("overlayFragmentMain", UTF8StringEncoding));

  MTL::RenderPipelineDescriptor *pDesc =
      MTL::RenderPipelineDescriptor::alloc()->init();
  pDesc->setVertexFunction(pVertexFn);
  pDesc->setFragmentFunction(pFragFn);

  auto *colorAtt = pDesc->colorAttachments()->object(0);
  colorAtt->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
  colorAtt->setBlendingEnabled(true);
  colorAtt->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
  colorAtt->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
  colorAtt->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
  colorAtt->setDestinationAlphaBlendFactor(
      MTL::BlendFactorOneMinusSourceAlpha);

  pDesc->setDepthAttachmentPixelFormat(
      MTL::PixelFormat::PixelFormatDepth16Unorm);

  NS::Error *pError = nullptr;
  _pOverlayPSO = _pDevice->newRenderPipelineState(pDesc, &pError);
  if (!_pOverlayPSO) {
    __builtin_printf("Overlay PSO error: %s\n",
                     pError->localizedDescription()->utf8String());
  }

  pVertexFn->release();
  pFragFn->release();
  pDesc->release();

  // Depth stencil state with depth test disabled
  MTL::DepthStencilDescriptor *pDsDesc =
      MTL::DepthStencilDescriptor::alloc()->init();
  pDsDesc->setDepthCompareFunction(
      MTL::CompareFunction::CompareFunctionAlways);
  pDsDesc->setDepthWriteEnabled(false);
  _pOverlayDepthState = _pDevice->newDepthStencilState(pDsDesc);
  pDsDesc->release();
}

void OverlayRenderer::buildQuadBuffers() {
  // Quad in top-left corner of NDC space
  // NDC: x [-1,1], y [-1,1], origin at center, y-up
  const float left = -0.95f;
  const float right = -0.35f;
  const float top = 0.95f;
  const float bottom = 0.55f;

  OverlayVertex verts[] = {
      {{left, bottom}, {0.f, 1.f}},  // bottom-left
      {{right, bottom}, {1.f, 1.f}}, // bottom-right
      {{left, top}, {0.f, 0.f}},     // top-left

      {{right, bottom}, {1.f, 1.f}}, // bottom-right
      {{right, top}, {1.f, 0.f}},    // top-right
      {{left, top}, {0.f, 0.f}},     // top-left
  };

  _pQuadVertexBuffer =
      _pDevice->newBuffer(sizeof(verts), MTL::ResourceStorageModeManaged);
  memcpy(_pQuadVertexBuffer->contents(), verts, sizeof(verts));
  _pQuadVertexBuffer->didModifyRange(
      NS::Range::Make(0, _pQuadVertexBuffer->length()));
}

void OverlayRenderer::buildTexture() {
  MTL::TextureDescriptor *pTexDesc = MTL::TextureDescriptor::alloc()->init();
  pTexDesc->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
  pTexDesc->setWidth(kOverlayWidth);
  pTexDesc->setHeight(kOverlayHeight);
  pTexDesc->setStorageMode(MTL::StorageModeManaged);
  pTexDesc->setUsage(MTL::TextureUsageShaderRead);

  _pOverlayTexture = _pDevice->newTexture(pTexDesc);
  pTexDesc->release();

  // Initialize with transparent black
  size_t bytesPerRow = kOverlayWidth * 4;
  uint8_t *pixels = (uint8_t *)calloc(kOverlayWidth * kOverlayHeight, 4);
  MTL::Region region = MTL::Region::Make2D(0, 0, kOverlayWidth, kOverlayHeight);
  _pOverlayTexture->replaceRegion(region, 0, pixels, bytesPerRow);
  free(pixels);
}

static void drawTextLine(CGContextRef ctx, const char *text, float x, float y,
                         float fontSize, CGColorRef color) {
  CFStringRef cfStr =
      CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
  CTFontRef font = CTFontCreateWithName(CFSTR("Menlo"), fontSize, NULL);

  CFStringRef keys[] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
  CFTypeRef values[] = {font, color};
  CFDictionaryRef attrs = CFDictionaryCreate(
      kCFAllocatorDefault, (const void **)keys, (const void **)values, 2,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  CFAttributedStringRef attrStr =
      CFAttributedStringCreate(kCFAllocatorDefault, cfStr, attrs);
  CTLineRef line = CTLineCreateWithAttributedString(attrStr);

  CGContextSetTextPosition(ctx, x, y);
  CTLineDraw(line, ctx);

  CFRelease(line);
  CFRelease(attrStr);
  CFRelease(attrs);
  CFRelease(font);
  CFRelease(cfStr);
}

void OverlayRenderer::updateTexture(const StatusData &status) {
  size_t bytesPerRow = kOverlayWidth * 4;
  uint8_t *pixels =
      (uint8_t *)calloc(kOverlayWidth * kOverlayHeight, 4);

  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef ctx = CGBitmapContextCreate(
      pixels, kOverlayWidth, kOverlayHeight, 8, bytesPerRow, colorSpace,
      (CGBitmapInfo)kCGImageAlphaPremultipliedFirst | (CGBitmapInfo)kCGBitmapByteOrder32Little);

  // Semi-transparent dark background with rounded corners
  CGContextSetRGBFillColor(ctx, 0.0, 0.0, 0.0, 0.75);
  CGRect bgRect = CGRectMake(4, 4, kOverlayWidth - 8, kOverlayHeight - 8);
  CGPathRef path =
      CGPathCreateWithRoundedRect(bgRect, 8.0, 8.0, NULL);
  CGContextAddPath(ctx, path);
  CGContextFillPath(ctx);
  CGPathRelease(path);

  // Text color
  CGFloat white[] = {1.0, 1.0, 1.0, 1.0};
  CGColorRef textColor = CGColorCreate(colorSpace, white);

  CGFloat green[] = {0.4, 1.0, 0.4, 1.0};
  CGColorRef greenColor = CGColorCreate(colorSpace, green);

  CGFloat yellow[] = {1.0, 0.9, 0.3, 1.0};
  CGColorRef yellowColor = CGColorCreate(colorSpace, yellow);

  // Status lines (CoreGraphics y=0 is bottom)
  float fontSize = 13.0f;
  float lineHeight = 22.0f;
  float startY = kOverlayHeight - 30.0f;
  float x = 16.0f;

  const char *stateName =
      status.state <= 5 ? kStateNames[status.state] : "???";

  // Title
  drawTextLine(ctx, "BEATLED SIMULATOR", x, startY, 11.0f, yellowColor);

  // State
  char buf[64];
  snprintf(buf, sizeof(buf), "State:   %s", stateName);
  drawTextLine(ctx, buf, x, startY - lineHeight, fontSize, textColor);

  // Connected
  snprintf(buf, sizeof(buf), "Server:  %s",
           status.connected ? "Connected" : "Waiting...");
  drawTextLine(ctx, buf, x, startY - lineHeight * 2, fontSize,
               status.connected ? greenColor : textColor);

  // Program
  const char *program_name = pattern_get_name(status.program_id);
  snprintf(buf, sizeof(buf), "Program: %s", program_name);
  drawTextLine(ctx, buf, x, startY - lineHeight * 3, fontSize, textColor);

  // Tempo
  float bpm =
      status.tempo_period_us > 0 ? 60000000.0f / status.tempo_period_us : 0.0f;
  snprintf(buf, sizeof(buf), "Tempo:   %.1f BPM", bpm);
  drawTextLine(ctx, buf, x, startY - lineHeight * 4, fontSize, textColor);

  // Beat count
  snprintf(buf, sizeof(buf), "Beat:    %u", status.beat_count);
  drawTextLine(ctx, buf, x, startY - lineHeight * 5, fontSize, textColor);

  CGColorRelease(textColor);
  CGColorRelease(greenColor);
  CGColorRelease(yellowColor);
  CGContextRelease(ctx);
  CGColorSpaceRelease(colorSpace);

  // Upload to Metal texture
  MTL::Region region = MTL::Region::Make2D(0, 0, kOverlayWidth, kOverlayHeight);
  _pOverlayTexture->replaceRegion(region, 0, pixels, bytesPerRow);
  free(pixels);
}

void OverlayRenderer::draw(MTL::RenderCommandEncoder *pEnc) {
  if (!_pOverlayPSO || !_pOverlayTexture)
    return;

  pEnc->setRenderPipelineState(_pOverlayPSO);
  pEnc->setDepthStencilState(_pOverlayDepthState);
  pEnc->setVertexBuffer(_pQuadVertexBuffer, 0, 0);
  pEnc->setFragmentTexture(_pOverlayTexture, 0);
  pEnc->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle,
                       NS::UInteger(0), NS::UInteger(6));
}
