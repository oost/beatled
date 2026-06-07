#ifndef SRC__RUNTIME__SIMULATOR__MTK_VIEW_DELEGATE__H_
#define SRC__RUNTIME__SIMULATOR__MTK_VIEW_DELEGATE__H_

#include <MetalKit/MetalKit.hpp>

#include "renderer.h"

class MyMTKViewDelegate : public MTK::ViewDelegate {
public:
  MyMTKViewDelegate(MTL::Device *pDevice, size_t numInstances);
  virtual ~MyMTKViewDelegate() override;
  virtual void drawInMTKView(MTK::View *pView) override;

private:
  Renderer *_pRenderer;
};

#endif // SRC__RUNTIME__SIMULATOR__MTK_VIEW_DELEGATE__H_