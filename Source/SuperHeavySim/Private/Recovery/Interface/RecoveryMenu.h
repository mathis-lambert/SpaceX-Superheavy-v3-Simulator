#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ARecoveryPlayerController;
class SVerticalBox;
class SBox;
class SRecoveryMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRecoveryMenu) {} SLATE_ARGUMENT(ARecoveryPlayerController*, Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    void ShowPage(int32 NewPage=0);
private:
    TWeakObjectPtr<ARecoveryPlayerController> Controller;
    TSharedPtr<SBox> Content;
    int32 Page=0;
    int32 PhotoSlot=0;
    struct FDisplayDraft
    {
        FIntPoint Resolution;
        int32 WindowMode=2,Quality=3,Shadows=3,Textures=3,Effects=3,AA=3,GI=3,Reflections=3;
        float FrameLimit=60,Light=1;
        bool bVSync=false;
    } Draft;
    static int32 ParentPage(int32 Id)
    {
        if(Id==2 || Id==3 || Id==7 || Id==11)return 8;
        if(Id==5)return 2;
        if(Id==12 || Id==14)return 2;
        if(Id==15)return 8;
        if(Id==16 || Id==17)return 10;
        if(Id>=18 && Id<=21)return 7;
        return 0;
    }
    void ReadGraphics();
    void ApplyGraphics();
    TSharedRef<SWidget> Text(const FString& Value,int32 Size=14,FLinearColor Color=FLinearColor(0.87f,0.91f,0.94f)) const;
    TSharedRef<SWidget> Button(const FString& Label,TFunction<void()> Action,bool bPrimary=false) const;
    void Choice(TSharedRef<SVerticalBox> Rows,const FString& Label,TArray<FString> Values,int32 Selected,TFunction<void(int32)> Changed);
    void Toggle(TSharedRef<SVerticalBox> Rows,const FString& Label,bool Selected,TFunction<void(bool)> Changed);
    void PhotoSlider(TSharedRef<SVerticalBox> Rows,const FString& Label,float* Value,float Minimum,float Maximum,const TCHAR* Unit,bool Logarithmic=false);
    void PhotoPage(TSharedRef<SVerticalBox> Rows);
};
