#pragma once
#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

// Canonical runtime references; update here when moving authored content.
namespace RecoveryAssets
{
    inline constexpr TCHAR MPC_SurfaceState[]=TEXT("/Game/Starbase/Materials/MPC_SurfaceState.MPC_SurfaceState");
    inline constexpr TCHAR M_LayeredWeather[]=TEXT("/Game/Starbase/Materials/M_LayeredWeather.M_LayeredWeather");
    inline constexpr TCHAR S_EngineRumble[]=TEXT("/Game/Starbase/Audio/S_EngineRumble.S_EngineRumble");
    inline constexpr TCHAR S_EngineCrackle[]=TEXT("/Game/Starbase/Audio/S_EngineCrackle.S_EngineCrackle");
    inline constexpr TCHAR S_CryogenicHiss[]=TEXT("/Game/Starbase/Audio/S_CryogenicHiss.S_CryogenicHiss");
    inline constexpr TCHAR S_Deluge[]=TEXT("/Game/Starbase/Audio/S_Deluge.S_Deluge");
    inline constexpr TCHAR S_TowerDrive[]=TEXT("/Game/Starbase/Audio/S_TowerDrive.S_TowerDrive");
    inline constexpr TCHAR S_TowerContact[]=TEXT("/Game/Starbase/Audio/S_TowerContact.S_TowerContact");
    inline constexpr TCHAR S_MountRelease[]=TEXT("/Game/Starbase/Audio/S_MountRelease.S_MountRelease");
    inline constexpr TCHAR M_StarField[]=TEXT("/Game/Starbase/Materials/M_StarField.M_StarField");
    inline constexpr TCHAR BP_SuperHeavy[]=TEXT("/Game/Starbase/Vehicle/Blueprints/BP_SuperHeavy");
    inline constexpr TCHAR S_EngineRoar[]=TEXT("/Game/Starbase/Audio/S_EngineRoar.S_EngineRoar");
    inline constexpr TCHAR S_CoastalWind[]=TEXT("/Game/Starbase/Audio/S_CoastalWind.S_CoastalWind");
    inline constexpr TCHAR SM_ExhaustEnvelope[]=TEXT("/Game/Starbase/Meshes/SM_ExhaustEnvelope.SM_ExhaustEnvelope");
    inline constexpr TCHAR M_NozzleCore[]=TEXT("/Game/Starbase/Materials/M_NozzleCore.M_NozzleCore");
    inline constexpr TCHAR M_RaptorPlume[]=TEXT("/Game/Starbase/Materials/M_RaptorPlume.M_RaptorPlume");
    inline constexpr TCHAR M_GridFinAlloy[]=TEXT("/Game/Starbase/Materials/M_GridFinAlloy.M_GridFinAlloy");
    inline constexpr TCHAR M_BoosterFlight[]=TEXT("/Game/Starbase/Materials/M_BoosterFlight.M_BoosterFlight");
    inline constexpr TCHAR SM_StarshipDetailed[]=TEXT("/Game/Starbase/Meshes/SM_StarshipDetailed.SM_StarshipDetailed");
    inline constexpr TCHAR NS_RecoveryVaporTrail[]=TEXT("/Game/Starbase/FX/NS_RecoveryVaporTrail.NS_RecoveryVaporTrail");
    inline constexpr TCHAR SM_RCSBlock[]=TEXT("/Game/Starbase/Meshes/SM_RCSBlock.SM_RCSBlock");
    inline constexpr TCHAR M_AttitudeGas[]=TEXT("/Game/Starbase/Materials/M_AttitudeGas.M_AttitudeGas");
    inline constexpr TCHAR DA_RecoveryEnvironment[]=TEXT("/Game/Starbase/Data/DA_RecoveryEnvironment.DA_RecoveryEnvironment");
    inline constexpr TCHAR SM_MWAM_GrassB[]=TEXT("/Game/ThirdParty/MWLandscapeAutoMaterial/Meshes/Plants/SM_MWAM_GrassB.SM_MWAM_GrassB");
    inline constexpr TCHAR SM_River_Rock[]=TEXT("/Game/ThirdParty/WaterMaterials/Meshes/SM_River_Rock.SM_River_Rock");
    inline constexpr TCHAR M_RecoveryRock[]=TEXT("/Game/Starbase/Materials/M_RecoveryRock.M_RecoveryRock");
    inline constexpr TCHAR M_SiteLamp[]=TEXT("/Game/Starbase/Materials/M_SiteLamp.M_SiteLamp");
    inline constexpr TCHAR M_Graphite[]=TEXT("/Game/Starbase/Materials/M_Graphite.M_Graphite");
    inline constexpr TCHAR M_Cladding[]=TEXT("/Game/Starbase/Materials/M_Cladding.M_Cladding");
    inline constexpr TCHAR M_Concrete[]=TEXT("/Game/Starbase/Materials/M_Concrete.M_Concrete");
    inline constexpr TCHAR M_SafetyAmber[]=TEXT("/Game/Starbase/Materials/M_SafetyAmber.M_SafetyAmber");
    inline constexpr TCHAR M_VolumetricVapor[]=TEXT("/Game/Starbase/Materials/M_VolumetricVapor.M_VolumetricVapor");
    inline constexpr TCHAR M_CryogenicVapor[]=TEXT("/Game/Starbase/Materials/M_CryogenicVapor.M_CryogenicVapor");
    inline constexpr TCHAR M_TurbulentDeluge[]=TEXT("/Game/Starbase/Materials/Effects/M_TurbulentDeluge.M_TurbulentDeluge");
    inline constexpr TCHAR SVT_TurbulentDeluge[]=TEXT("/Game/Starbase/FX/Volumes/SVT_TurbulentDeluge.SVT_TurbulentDeluge");
    inline constexpr TCHAR SM_ServicePickup[]=TEXT("/Game/Starbase/Meshes/Starbase/SM_ServicePickup.SM_ServicePickup");
    inline constexpr TCHAR SM_WindFlag[]=TEXT("/Game/Starbase/Meshes/Starbase/SM_WindFlag.SM_WindFlag");
    inline constexpr TCHAR M_WindFlag[]=TEXT("/Game/Starbase/Materials/Starbase/M_WindFlag.M_WindFlag");
    inline TArray<FSoftObjectPath> StartupAssets()
    {
        return {
            FSoftObjectPath(MPC_SurfaceState),
            FSoftObjectPath(S_EngineRumble),
            FSoftObjectPath(S_EngineCrackle),
            FSoftObjectPath(S_CryogenicHiss),
            FSoftObjectPath(S_Deluge),
            FSoftObjectPath(S_TowerDrive),
            FSoftObjectPath(S_TowerContact),
            FSoftObjectPath(S_MountRelease),
            FSoftObjectPath(M_StarField),
            FSoftObjectPath(M_LayeredWeather),
            FSoftObjectPath(S_EngineRoar),
            FSoftObjectPath(S_CoastalWind),
            FSoftObjectPath(SM_ExhaustEnvelope),
            FSoftObjectPath(M_RaptorPlume),
            FSoftObjectPath(M_NozzleCore),
            FSoftObjectPath(M_GridFinAlloy),
            FSoftObjectPath(M_BoosterFlight),
            FSoftObjectPath(SM_StarshipDetailed),
            FSoftObjectPath(NS_RecoveryVaporTrail),
            FSoftObjectPath(SM_RCSBlock),
            FSoftObjectPath(M_AttitudeGas),
            FSoftObjectPath(DA_RecoveryEnvironment),
            FSoftObjectPath(SM_MWAM_GrassB),
            FSoftObjectPath(SM_River_Rock),
            FSoftObjectPath(M_RecoveryRock),
            FSoftObjectPath(M_SiteLamp),
            FSoftObjectPath(M_Graphite),
            FSoftObjectPath(M_Cladding),
            FSoftObjectPath(M_Concrete),
            FSoftObjectPath(M_SafetyAmber),
            FSoftObjectPath(M_VolumetricVapor),
            FSoftObjectPath(M_CryogenicVapor),
            FSoftObjectPath(M_TurbulentDeluge),
            FSoftObjectPath(SVT_TurbulentDeluge),
            FSoftObjectPath(SM_ServicePickup),
            FSoftObjectPath(SM_WindFlag),
            FSoftObjectPath(M_WindFlag),
        };
    }
}
