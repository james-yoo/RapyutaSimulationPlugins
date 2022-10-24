// Copyright 2020-2021 Rapyuta Robotics Co., Ltd.

#include "Sensors/RRROS2BaseSensorComponent.h"

DEFINE_LOG_CATEGORY(LogROS2Sensor);

void URRROS2BaseSensorComponent::InitROS2(AROS2Node* InROS2Node,
                                          const FString& InPublisherName,
                                          const FString& InTopicName,
                                          const TEnumAsByte<UROS2QoS> InQoS)
{
    verify(IsValid(InROS2Node));
    CreatePublisher(InPublisherName);
    PreInitializePublisher(InROS2Node, InTopicName);
    InitializePublisher(InROS2Node, InQoS);
}

void URRROS2BaseSensorComponent::CreatePublisher(const FString& InPublisherName)
{
    // Init [SensorPublisher] info
    if (nullptr == SensorPublisher)
    {
        verify(SensorPublisherClass);
        FString PublisherName =
            InPublisherName.IsEmpty() ? FString::Printf(TEXT("%sSensorPublisher"), *GetName()) : InPublisherName;
        // Instantiate publisher
        SensorPublisher = NewObject<URRROS2BaseSensorPublisher>(this, SensorPublisherClass, *PublisherName);
        SensorPublisher->DataSourceComponent = this;
    }
}

void URRROS2BaseSensorComponent::PreInitializePublisher(AROS2Node* InROS2Node, const FString& InTopicName)
{
    if (IsValid(SensorPublisher))
    {
        SensorPublisher->PublicationFrequencyHz = PublicationFrequencyHz;

        // Update [SensorPublisher]'s topic name
        SensorPublisher->TopicName = InTopicName.IsEmpty() ? TopicName : InTopicName;
        verify(false == SensorPublisher->TopicName.IsEmpty());

        if (bAppendNodeNamespace)
        {
            FrameId = URRGeneralUtils::ComposeROSFullFrameId(InROS2Node->Namespace, *FrameId);
            verify(false == FrameId.IsEmpty());
        }
    }
}

void URRROS2BaseSensorComponent::InitializePublisher(AROS2Node* InROS2Node, const TEnumAsByte<UROS2QoS> InQoS)
{
    if (IsValid(SensorPublisher))
    {
        SensorPublisher->InitializeWithROS2(InROS2Node);
        SensorPublisher->Init(InQoS);
    }
}
