/**
 * @file RRCrowdFollowingComponent.h
 * @brief Base component for crowd path following movement
 * @copyright Copyright 2020-2022 Rapyuta Robotics Co., Ltd.
 */

#pragma once

#include "Navigation/CrowdFollowingComponent.h"

#include "RRCrowdFollowingComponent.generated.h"

class URRFloatingMovementComponent;
/**
 * @brief Base component for crowd path following movement. Use #URRFloatingMovementComponent if IsCrowdSimulationEnabled is false.
 */
UCLASS()
class RAPYUTASIMULATIONPLUGINS_API URRCrowdFollowingComponent : public UCrowdFollowingComponent
{
    GENERATED_BODY()

public:
    URRCrowdFollowingComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<URRFloatingMovementComponent> FloatMovementComp = nullptr;
    virtual void SetNavMovementInterface(INavMovementInterface* InMoveInterface) override;

    bool Is2DMovement() const;

    bool IsIdle() const
    {
        return (EPathFollowingStatus::Idle == Status);
    }

    bool IsReadyForNewMovementOrder() const
    {
        return IsIdle();
    }

protected:
    virtual void FollowPathSegment(float InDeltaTime) override;

    virtual void ApplyCrowdAgentVelocity(const FVector& InNewVelocity,
                                         const FVector& InDestPathCorner,
                                         bool bInTraversingLink,
                                         bool bInNearEndOfPath) override;
    virtual void OnPathFinished(const FPathFollowingResult& InResult) override;
};
