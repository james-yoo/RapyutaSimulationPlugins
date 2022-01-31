// Copyright 2020-2021 Rapyuta Robotics Co., Ltd.

#include "Tools/SimulationState.h"

#include "Tools/ROS2Spawnable.h"
// UE
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

// rclUE
#include "Srvs/ROS2AttachSrv.h"
#include "Srvs/ROS2DeleteEntitySrv.h"
#include "Srvs/ROS2GetEntityStateSrv.h"
#include "Srvs/ROS2SetEntityStateSrv.h"
#include "Srvs/ROS2SpawnEntitySrv.h"

// Sets default values
ASimulationState::ASimulationState()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
}

void ASimulationState::Init(AROS2Node* InROS2Node)
{
    ROSServiceNode = InROS2Node;

    // register delegates to node
    FServiceCallback GetEntityStateSrvCallback;
    FServiceCallback SetEntityStateSrvCallback;
    FServiceCallback AttachSrvCallback;
    FServiceCallback SpawnEntitySrvCallback;
    FServiceCallback DeleteEntitySrvCallback;
    GetEntityStateSrvCallback.BindUObject(this, &ASimulationState::GetEntityStateSrv);
    SetEntityStateSrvCallback.BindUObject(this, &ASimulationState::SetEntityStateSrv);
    AttachSrvCallback.BindUObject(this, &ASimulationState::AttachSrv);
    SpawnEntitySrvCallback.BindUObject(this, &ASimulationState::SpawnEntitySrv);
    DeleteEntitySrvCallback.BindUObject(this, &ASimulationState::DeleteEntitySrv);
    ROSServiceNode->AddService(TEXT("GetEntityState"), UROS2GetEntityStateSrv::StaticClass(), GetEntityStateSrvCallback);
    ROSServiceNode->AddService(TEXT("SetEntityState"), UROS2SetEntityStateSrv::StaticClass(), SetEntityStateSrvCallback);
    ROSServiceNode->AddService(TEXT("Attach"), UROS2AttachSrv::StaticClass(), AttachSrvCallback);
    ROSServiceNode->AddService(TEXT("SpawnEntity"), UROS2SpawnEntitySrv::StaticClass(), SpawnEntitySrvCallback);
    ROSServiceNode->AddService(TEXT("DeleteEntity"), UROS2DeleteEntitySrv::StaticClass(), DeleteEntitySrvCallback);

    // add all actors
#if WITH_EDITOR
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
    UE_LOG(LogTemp, Warning, TEXT("Found %d actors in the scene"), AllActors.Num());
#endif
    for (TActorIterator<AActor> It(GetWorld(), AActor::StaticClass()); It; ++It)
    {
        AActor* actor = *It;
        AddEntity(actor);
    }
}

void ASimulationState::AddEntity(AActor* Entity)
{
    if (IsValid(Entity))
    {
        Entities.Emplace(Entity->GetName(), Entity);
        for (auto& tag : Entity.Tags)
        {
            if (EntitiesWithTag.Contains(tag))
            {
                EntitiesWithTag[tag].Emplace(Entity);
            }
            else
            {
                TArray<AActor*> array;
                array.Emplace(Entity)
                EntitiesWithTag.Emplace(tag, array);
            }
        }
    }
}

void ASimulationState::AddSpawnableEntities(TMap<FString, TSubclassOf<AActor>> InSpawnableEntities)
{
    for (auto& Elem : InSpawnableEntities)
    {
        SpawnableEntities.Emplace(Elem.Key, Elem.Value);
    }
}

template<typename T>
bool ASimulationState::CheckEntity(TMap<FString, T>& InEntities, const FString& InEntityName, const bool bAllowEmpty)
{
    bool result = false;
    if (InEntities.Contains(InEntityName))
    {
        if (IsValid(InEntities[InEntityName]))
        {
            result = true;
        }
        else
        {
            UE_LOG(LogRapyutaCore, Warning, TEXT("Request name %s entity gets invalid -> removed from Entities"), *InEntityName);
            InEntities.Remove(InEntityName);
        }
    }
    else if (bAllowEmpty && InEntityName.IsEmpty())
    {
        result = true;
    }
    else
    {
        UE_LOG(LogRapyutaCore,
               Warning,
               TEXT("%s is not under SimulationState control. Please call dedicated method to make Actors under "
                    "SimulationState control."),
               *InEntityName);
    }

    return result;
}

// https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl
template bool ASimulationState::CheckEntity<AActor*>(TMap<FString, AActor*>& InEntities,
                                                     const FString& InEntityName,
                                                     const bool bAllowEmpty);

template bool ASimulationState::CheckEntity<TSubclassOf<AActor>>(TMap<FString, TSubclassOf<AActor>>& InEntities,
                                                                 const FString& InEntityName,
                                                                 const bool bAllowEmpty);

bool ASimulationState::CheckEntity(const FString& InEntityName, const bool bAllowEmpty)
{
    return CheckEntity<AActor*>(Entities, InEntityName, bAllowEmpty);
}

bool ASimulationState::CheckSpawnableEntity(const FString& InEntityName, const bool bAllowEmpty)
{
    return CheckEntity<TSubclassOf<AActor>>(SpawnableEntities, InEntityName, bAllowEmpty);
}

void ASimulationState::GetEntityStateSrv(UROS2GenericSrv* Service)
{
    UROS2GetEntityStateSrv* GetEntityStateService = Cast<UROS2GetEntityStateSrv>(Service);

    FROSGetEntityState_Request Request;
    GetEntityStateService->GetRequest(Request);

    // UE_LOG(LogTemp, Warning, TEXT("GetEntityStateSrv called - Currently ignoring Twist"));

    FROSGetEntityState_Response Response;
    Response.state_name = Request.name;
    Response.success = CheckEntity(Request.name, false) && CheckEntity(Request.reference_frame, true);

    if (Response.success)
    {
        FTransform relativeTransf;
        FTransform worldTransf = Entities[Request.name]->GetTransform();
        URRGeneralUtils::GetRelativeTransform(
            Request.reference_frame, 
            Entities.Contains(Request.reference_frame) ? Entities[Request.reference_frame] : nullptr, 
            worldTransf, 
            relativeTransf
        );
        relativeTransf = ConversionUtils::TransformUEToROS(relativeTransf);

        Response.state_pose_position_x = relativeTransf.GetTranslation().X;
        Response.state_pose_position_y = relativeTransf.GetTranslation().Y;
        Response.state_pose_position_z = relativeTransf.GetTranslation().Z;
        Response.state_pose_orientation = relativeTransf.GetRotation();

        Response.state_twist_linear = FVector::ZeroVector;
        Response.state_twist_angular = FVector::ZeroVector;
    }

    GetEntityStateService->SetResponse(Response);
}

void ASimulationState::SetEntityStateSrv(UROS2GenericSrv* Service)
{
    UROS2SetEntityStateSrv* SetEntityStateService = Cast<UROS2SetEntityStateSrv>(Service);

    FROSSetEntityState_Request Request;
    SetEntityStateService->GetRequest(Request);

    // UE_LOG(LogTemp, Warning, TEXT("SetEntityStateService called - Currently ignoring Twist"));

    FROSSetEntityState_Response Response;
    Response.success = CheckEntity(Request.state_name, false) && CheckEntity(Request.state_reference_frame, true);

    if (Response.success)
    {
        FVector pos(Request.state_pose_position_x, Request.state_pose_position_y, Request.state_pose_position_z);
        FTransform relativeTransf(Request.state_pose_orientation, pos);
        relativeTransf = ConversionUtils::TransformROSToUE(relativeTransf);
        FTransform worldTransf;
        URRGeneralUtils::GetWorldTransform(
            Request.state_reference_frame, 
            Entities.Contains(Request.state_reference_frame) ? Entities[Request.state_reference_frame] : nullptr,
            relativeTransf,
            worldTransf
        );
        Entities[Request.state_name]->SetActorTransform(worldTransf);
    }

    SetEntityStateService->SetResponse(Response);
}

void ASimulationState::AttachSrv(UROS2GenericSrv* Service)
{
    UROS2AttachSrv* AttachService = Cast<UROS2AttachSrv>(Service);

    FROSAttach_Request Request;
    AttachService->GetRequest(Request);

    FROSAttach_Response Response;
    Response.success = CheckEntity(Request.name1, false) && CheckEntity(Request.name2, false);
    if (Response.success)
    {
        AActor* Entity1 = Entities[Request.name1];
        AActor* Entity2 = Entities[Request.name2];

        if (!Entity2->IsAttachedTo(Entity1))
        {
            Entity2->AttachToActor(Entity1, FAttachmentTransformRules::KeepWorldTransform);
        }
        else
        {
            Entity2->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        }
    }
    else
    {
        UE_LOG(LogRapyutaCore,
               Warning,
               TEXT("Entity %s and/or %s not exit or not under SimulationState Actor control. Please call AddEntity to make Actors "
                    "under SimulationState control."),
               *Request.name1,
               *Request.name2);
    }

    AttachService->SetResponse(Response);
}

void ASimulationState::SpawnEntitySrv(UROS2GenericSrv* Service)
{
    UROS2SpawnEntitySrv* SpawnEntityService = Cast<UROS2SpawnEntitySrv>(Service);

    FROSSpawnEntity_Request Request;
    SpawnEntityService->GetRequest(Request);

    FROSSpawnEntity_Response Response;
    Response.success = CheckSpawnableEntity(Request.xml, false) && CheckEntity(Request.state_reference_frame, true);

    if (Response.success)
    {
        FVector Pos(Request.state_pose_position_x, Request.state_pose_position_y, Request.state_pose_position_z);
        FTransform relativeTransf(Request.state_pose_orientation, Pos);
        FTransform worldTransf;
        URRGeneralUtils::GetWorldTransform(
            Request.state_reference_frame, 
            Entities.Contains(Request.state_reference_frame) ? Entities[Request.state_reference_frame] : nullptr, 
            relativeTransf,
            worldTransf
        );
        worldTransf = ConversionUtils::TransformROSToUE(worldTransf);
        
        // todo: check data.name is valid
        // todo: check same name object is exists or not.

        UE_LOG(LogRapyutaCore, Warning, TEXT("Spawning %s"), *Request.xml);

        AActor* NewEntity = GetWorld()->SpawnActorDeferred<AActor>(SpawnableEntities[Request.xml], worldTransf);
        UROS2Spawnable* SpawnableComponent = NewObject<UROS2Spawnable>(NewEntity, FName("ROS2 Spawn Parameters"));

        SpawnableComponent->RegisterComponent();
        SpawnableComponent->InitializeParameters(Request);
        NewEntity->AddInstanceComponent(SpawnableComponent);
#if WITH_EDITOR
        NewEntity->SetActorLabel(*Request.state_name);
#endif
        NewEntity->Rename(*Request.state_name);

        UGameplayStatics::FinishSpawningActor(NewEntity, worldTransf);
        AddEntity(NewEntity);

        UE_LOG(LogRapyutaCore, Warning, TEXT("New Spawned Entity Name: %s"), *NewEntity->GetName());
    }

    SpawnEntityService->SetResponse(Response);
}

void ASimulationState::DeleteEntitySrv(UROS2GenericSrv* Service)
{
    UROS2DeleteEntitySrv* DeleteEntityService = Cast<UROS2DeleteEntitySrv>(Service);

    FString Name;
    FROSDeleteEntity_Request Request;
    DeleteEntityService->GetRequest(Request);

    UE_LOG(LogTemp, Warning, TEXT("DeleteEntityService called"));

    FROSDeleteEntity_Response Response;
    Response.success = false;
    if (Entities.Contains(Request.name))
    {
        AActor* Removed = Entities.FindAndRemoveChecked(Request.name);
        Removed->Destroy();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Entity %s not found"), *Name);
    }

    DeleteEntityService->SetResponse(Response);
}
