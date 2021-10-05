// Copyright 2020-2021 Rapyuta Robotics Co., Ltd.


#include "Sensors/BaseLidar.h"


// Sets default values
ABaseLidar::ABaseLidar()
{
    Gen = std::mt19937{Rng()};
    GaussianRNGPosition = std::normal_distribution<>{PositionalNoiseMean, PositionalNoiseVariance};
    GaussianRNGIntensity = std::normal_distribution<>{IntensityNoiseMean, IntensityNoiseVariance};

    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

}

// Called when the game starts or when spawned
void ABaseLidar::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseLidar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (!IsInitialized)
    {
        return;
    }
}

void ABaseLidar::Run()
{
    checkNoEntry();
}

void ABaseLidar::Scan()
{
    checkNoEntry();
}

void ABaseLidar::LidarMessageUpdate(UROS2GenericMsg* TopicMessage)
{
    checkNoEntry();
}

bool ABaseLidar::Visible(AActor* TargetActor)
{
    checkNoEntry();
    return false;
}

void ABaseLidar::InitLidar(AROS2Node* Node, const FString& TopicName)
{
    LidarPublisher->TopicName = TopicName;
    LidarPublisher->PublicationFrequencyHz = ScanFrequency;
    LidarPublisher->OwnerNode = Node;

    InitToNode(Node);
}

void ABaseLidar::InitToNode(AROS2Node* Node)
{
    checkNoEntry();
}

void ABaseLidar::GetData(TArray<FHitResult>& OutHits, float& OutTime)
{
    // what about the rest of the information?
    OutHits = RecordedHits;
    OutTime = TimeOfLastScan;
}

FLinearColor ABaseLidar::GetColorFromIntensity(const float Intensity)
{
    float NormalizedIntensity = (Intensity - IntensityMin) / (IntensityMax - IntensityMin);
    return InterpolateColor(NormalizedIntensity);
}

FLinearColor ABaseLidar::InterpolateColor(float x)
{
    // this means that viz and data sent won't correspond, which should be ok
    x = x + WithNoise * GaussianRNGIntensity(Gen);
    return x > .5 ? FLinearColor::LerpUsingHSV(ColorMid, ColorMax, 2 * x - 1)
                  : FLinearColor::LerpUsingHSV(ColorMin, ColorMid, 2 * x);
}

float ABaseLidar::IntensityFromDist(float BaseIntensity, float Distance)
{
    return BaseIntensity * 1.3f * exp(-.1f * (pow(3.5f * Distance, .6f))) / (1 + exp(-((3.5f * Distance))));
}

