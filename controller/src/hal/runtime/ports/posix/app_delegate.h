#ifndef SRC__RUNTIME__SIMULATOR__APP_DELEGATE__H_
#define SRC__RUNTIME__SIMULATOR__APP_DELEGATE__H_

#include <AppKit/AppKit.hpp>

#include "mtk_view_delegate.h"

class MyAppDelegate : public NS::ApplicationDelegate {
public:
  ~MyAppDelegate();

  NS::Menu *createMenuBar();

  virtual void
  applicationWillFinishLaunching(NS::Notification *pNotification) override;
  virtual void
  applicationDidFinishLaunching(NS::Notification *pNotification) override;
  virtual bool applicationShouldTerminateAfterLastWindowClosed(
      NS::Application *pSender) override;

private:
  NS::Window *_pWindow;
  MTK::View *_pMtkView;
  MTL::Device *_pDevice;
  MyMTKViewDelegate *_pViewDelegate = nullptr;
};

#endif // SRC__RUNTIME__SIMULATOR__APP_DELEGATE__H_