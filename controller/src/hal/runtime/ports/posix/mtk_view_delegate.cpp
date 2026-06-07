#include "mtk_view_delegate.h"

MyMTKViewDelegate::MyMTKViewDelegate(MTL::Device *pDevice, size_t numInstances)
    : MTK::ViewDelegate(), _pRenderer(new Renderer(pDevice, numInstances)) {}

MyMTKViewDelegate::~MyMTKViewDelegate() { delete _pRenderer; }

void MyMTKViewDelegate::drawInMTKView(MTK::View *pView) {
  _pRenderer->draw(pView);
}
