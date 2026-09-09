#include "Recovery/Presentation/RecoverySiteDetailsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/App.h"

URecoverySiteDetailsComponent::URecoverySiteDetailsComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void URecoverySiteDetailsComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(!FApp::CanEverRender() || !bEnabled){SetComponentTickEnabled(false);return;}
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if(!D || !D->Tower)return;
    if(!bBuilt)Build(D->Tower->GetActorTransform());
    SetComponentTickEnabled(false);
}
void URecoverySiteDetailsComponent::Box(int32 Material,const FVector& Centre,const FVector& Size,const FQuat& Rotation)
{Batches[Material*2]->AddInstance(FTransform(Rotation,Centre*100,Size));}
void URecoverySiteDetailsComponent::Pipe(int32 Material,const FVector& Start,const FVector& End,double Diameter)
{
    const FVector Delta=End-Start;
    Batches[Material*2+1]->AddInstance(FTransform(FRotationMatrix::MakeFromZ(Delta).ToQuat(),(Start+End)*50,FVector(Diameter,Diameter,Delta.Size())));
}
void URecoverySiteDetailsComponent::Ring(int32 Material,const FVector& Centre,double Radius,double Diameter)
{
    constexpr int Segments=32;
    for(int I=0;I<Segments;++I)
    {
        const double A=I*2*PI/Segments,B=(I+1)*2*PI/Segments;
        Pipe(Material,Centre+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0),Centre+FVector(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,0),Diameter);
    }
}
void URecoverySiteDetailsComponent::Ladder(const FVector& Bottom,double Height)
{
    for(double Side:{-.32,.32})Pipe(0,Bottom+FVector(Side,0,0),Bottom+FVector(Side,0,Height),.055);
    for(double Z=.3;Z<Height;Z+=.3)Pipe(0,Bottom+FVector(-.32,0,Z),Bottom+FVector(.32,0,Z),.035);
}
int32 URecoverySiteDetailsComponent::GetInstanceCount() const
{int32 Count=0;for(const auto& Batch:Batches)Count+=Batch->GetInstanceCount();return Count;}
void URecoverySiteDetailsComponent::Build(const FTransform& Site)
{
    const TCHAR* Materials[]={RecoveryAssets::M_Cladding,RecoveryAssets::M_Graphite,RecoveryAssets::M_Concrete,RecoveryAssets::M_SafetyAmber};
    for(int Material=0;Material<4;++Material)for(int Shape=0;Shape<2;++Shape)
    {
        auto* Batch=NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner(),FName(*FString::Printf(TEXT("SiteDetail_%d_%d"),Material,Shape)));
        Batch->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Shape==0?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        Batch->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Materials[Material]));
        Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);Batch->SetGenerateOverlapEvents(false);
        Batch->SetCastShadow(true);Batch->SetCullDistances(DrawDistanceM*80,DrawDistanceM*100);
        Batch->bAutoRebuildTreeOnInstanceChanges=false;
        Batch->RegisterComponent();GetOwner()->AddInstanceComponent(Batch);Batch->SetWorldTransform(Site);Batches.Add(Batch);
    }
    // Pipe corridor between the tank farm and mount, with actual supports,
    // insulation collars, expansion offsets and accessible valve stations.
    for(int Line=0;Line<5;++Line)
    {
        const double Y=45+Line*.85,Z=2.+Line*.22;
        for(int Span=0;Span<13;++Span)
        {
            const double X=-104+Span*10;
            Pipe(0,FVector(X,Y,Z),FVector(X+10,Y,Z),.22+Line*.07);
            Pipe(1,FVector(X+.2,Y,Z),FVector(X+.35,Y,Z),.38+Line*.07);
        }
        Pipe(0,FVector(26,Y,Z),FVector(35,Y,Z),.22+Line*.07);
        Pipe(0,FVector(35,Y,Z),FVector(35,20,Z),.22+Line*.07);
        Pipe(0,FVector(35,20,Z),FVector(30,15,Z),.22+Line*.07);
        for(int Valve=0;Valve<4;++Valve)
        {
            const double X=-88+Valve*29;
            Box(1,FVector(X,Y,Z),FVector(.65,.55,.55));
            Pipe(0,FVector(X,Y,Z),FVector(X,Y,Z+.7),.07);
            Ring(3,FVector(X,Y,Z+.7),.22,.035);
            Pipe(3,FVector(X-.22,Y,Z+.7),FVector(X+.22,Y,Z+.7),.03);
        }
    }
    for(int Span=0;Span<14;++Span)
    {
        const double X=-104+Span*10;
        Box(2,FVector(X,47,.25),FVector(1.5,6,.5));
        for(double Y:{44.2,49.8})Box(1,FVector(X,Y,1.5),FVector(.2,.2,3));
        Box(1,FVector(X,47,1.7),FVector(.18,5.8,.18));
    }
    // Tank catwalks, insulation seams, handrails, access ladders and feed pipes.
    for(int Tank=0;Tank<5;++Tank)
    {
        const double X=-95+Tank*20;
        for(double Z:{3.,6.,9.,12.})Ring(0,FVector(X,70,Z),6.035,.055);
        Ring(1,FVector(X,70,12.1),6.25,.32);
        for(double Z:{12.5,13.1})Ring(0,FVector(X,70,Z),6.4,.045);
        for(int Post=0;Post<24;++Post)
        {
            const double A=Post*2*PI/24;
            const FVector Foot(X+6.4*FMath::Cos(A),70+6.4*FMath::Sin(A),12.1);
            Pipe(0,Foot,Foot+FVector(0,0,1.05),.045);
        }
        Ladder(FVector(X,63.5,.2),12.2);
        Pipe(0,FVector(X-2,70,13.5),FVector(X-2,70,15.2),.18);
        Pipe(0,FVector(X+2,63.9,1),FVector(X+2,52,1),.45);
        Box(2,FVector(X+3.8,60,.2),FVector(2.6,2.2,.4));
        Box(0,FVector(X+3.8,60,1),FVector(1.5,1.2,1.4));
        Box(1,FVector(X+3.8,59.38,1),FVector(1.2,.025,1.1));
    }
    // Operations roof mechanical units, side-wall ribs and rainwater drainage.
    for(int Unit=0;Unit<6;++Unit)
    {
        const FVector Position(-88+Unit*7,-60,13);
        Box(0,Position,FVector(4,3,1.6));
        for(double Offset:{-1.,1.})
        {
            Pipe(1,Position+FVector(Offset,0,.81),Position+FVector(Offset,0,.91),1.3);
            for(int Blade=0;Blade<6;++Blade)
                Box(0,Position+FVector(Offset,0,.94),FVector(1.15,.04,.025),FQuat(FVector::UpVector,Blade*PI/6));
        }
    }
    for(int Rib=0;Rib<33;++Rib)Box(0,FVector(-93.5+Rib*1.47,-70.05,5.6),FVector(.045,.08,10.8));
    for(double X:{-93.,-47.})Pipe(1,FVector(X,-69.5,.3),FVector(X,-69.5,12.3),.16);
    // Cable trays and protected service cabinets at the tower's rear face.
    for(int Line=0;Line<6;++Line)
        Pipe(0,FVector(-6.8,-3+Line,1),FVector(-6.8,-3+Line,92),Line<2?.24:.12);
    for(int Level=1;Level<19;++Level)
        Box(1,FVector(-6.85,-.5,Level*5),FVector(.3,7,.15));
    for(int Cabinet=0;Cabinet<5;++Cabinet)
    {
        const FVector C(-13, -7+Cabinet*3,1.25);
        Box(2,C-FVector(0,0,1),FVector(2.2,2.5,.5));
        Box(0,C,FVector(1.4,1.9,2));
        Box(1,C+FVector(.71,0,.25),FVector(.025,1.5,1.25));
        Pipe(0,C+FVector(.78,.58,-.1),C+FVector(.78,.58,.35),.04);
    }
    // Perimeter wire infill uses a single instanced batch and no collision.
    for(double Y:{-135.,135.})for(int Panel=0;Panel<52;++Panel)
    {
        const double X=-140+Panel*8;
        Pipe(1,FVector(X,Y,.15),FVector(X+8,Y,3.15),.025);
        Pipe(1,FVector(X,Y,3.15),FVector(X+8,Y,.15),.025);
        for(int Wire=1;Wire<8;++Wire)Pipe(1,FVector(X+Wire,Y,.15),FVector(X+Wire,Y,3.15),.012);
    }
    BuildServiceFacilities();
    for(auto& Batch:Batches){Batch->bAutoRebuildTreeOnInstanceChanges=true;Batch->BuildTreeIfOutdated(true,true);}
    bBuilt=true;
    UE_LOG(LogTemp,Display,TEXT("STARBASE_INDUSTRIAL_DETAIL instances=%d batches=%d"),GetInstanceCount(),Batches.Num());
}
