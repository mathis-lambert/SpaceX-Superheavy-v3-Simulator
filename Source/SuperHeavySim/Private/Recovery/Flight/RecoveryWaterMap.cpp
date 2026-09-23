#include "Recovery/Flight/RecoveryWaterMap.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"

TSharedPtr<const FRecoveryWaterMap,ESPMode::ThreadSafe> FRecoveryWaterMap::Load()
{
    // Only called by game-thread configuration creation. Never perform I/O in a substep.
    static TSharedPtr<const FRecoveryWaterMap,ESPMode::ThreadSafe> Cached=[]()
    {
        auto Map=MakeShared<FRecoveryWaterMap,ESPMode::ThreadSafe>();
        TArray<uint8> Data;
        if(!FFileHelper::LoadFileToArray(Data,*(FPaths::ProjectContentDir()/TEXT("Starbase/Data/Water/Surface.bin"))))
        {UE_LOG(LogTemp,Error,TEXT("Water classification missing"));return Map;}
        FMemoryReader Reader(Data,true);uint32 Magic=0,Count=0;Reader<<Magic<<Count;
        if(Magic!=0x31525457 || Count!=3)return Map;
        for(uint32 I=0;I<Count && !Reader.IsError();++I)
        {
            FLayer L;Reader<<L.Width<<L.Height<<L.West<<L.South<<L.East<<L.North;
            if(L.Width==0 || L.Height==0 || L.Width>4096 || L.Height>4096 || L.East<=L.West || L.North<=L.South)
            {Map->Layers.Reset();return Map;}
            const int64 Bytes=(int64(L.Width)*L.Height+7)/8;
            if(Reader.TotalSize()-Reader.Tell()<Bytes){Map->Layers.Reset();return Map;}
            L.Bits.SetNumUninitialized(Bytes);Reader.Serialize(L.Bits.GetData(),Bytes);
            Map->Layers.Add(MoveTemp(L));
        }
        if(Reader.IsError() || Map->Layers.Num()!=3)Map->Layers.Reset();
        return Map;
    }();
    return Cached;
}

bool FRecoveryWaterMap::IsWater(const FVector& WorldM) const
{
    const FVector Q=(WorldM-FVector(0,0,-FlightGeometry::EarthRadiusM)).GetSafeNormal();
    const FVector Geo=FVector(.992208696,-.124586893,0)*Q.X+FVector(.054610022,.434913642,.898814703)*Q.Y+
        FVector(-.111980532,-.891811774,.438328791)*Q.Z;
    const double Lon=FMath::RadiansToDegrees(FMath::Atan2(Geo.Y,Geo.X));
    const double Lat=FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Geo.Z,-1.,1.)));
    for(const auto& L:Layers)
    {
        if(Lon<L.West || Lon>=L.East || Lat<L.South || Lat>L.North)continue;
        const uint32 X=FMath::Clamp(int32((Lon-L.West)/(L.East-L.West)*L.Width),0,int32(L.Width)-1);
        const uint32 Y=FMath::Clamp(int32((L.North-Lat)/(L.North-L.South)*L.Height),0,int32(L.Height)-1);
        const uint32 Bit=Y*L.Width+X;
        return (L.Bits[Bit/8] & (1<<(Bit%8)))!=0;
    }
    return false;
}
