#include "RecoveryLoadingScreen.h"
#include "Modules/ModuleManager.h"
#include "PreLoadScreenBase.h"
#include "PreLoadScreenManager.h"
#include "Misc/App.h"

class FRecoveryEngineScreen : public FPreLoadScreenBase
{
    TSharedPtr<SWidget> Widget;
public:
    virtual void Init() override
    { Widget=SNew(SRecoveryLoadingScreen).Status(FText::FromString(TEXT("INITIALIZING"))); }
    virtual TSharedPtr<SWidget> GetWidget() override { return Widget; }
    virtual const TSharedPtr<const SWidget> GetWidget() const override { return Widget; }
    virtual bool IsDone() const override { return bIsEngineLoadingFinished; }
    virtual void CleanUp() override { Widget.Reset(); }
};

class FRecoveryLoadingModule : public IModuleInterface
{
    TSharedPtr<FRecoveryEngineScreen> Screen;
public:
    virtual void StartupModule() override
    {
        if(!IsRunningCommandlet() && FApp::CanEverRender())
            if(auto* Manager=FPreLoadScreenManager::Get())
            {
                Screen=MakeShared<FRecoveryEngineScreen>();
                Screen->Init();
                Manager->RegisterPreLoadScreen(Screen);
            }
    }
    virtual void ShutdownModule() override
    {
        if(Screen)if(auto* Manager=FPreLoadScreenManager::Get())Manager->UnRegisterPreLoadScreen(Screen);
        Screen.Reset();
    }
};
IMPLEMENT_MODULE(FRecoveryLoadingModule,RecoveryLoading)
