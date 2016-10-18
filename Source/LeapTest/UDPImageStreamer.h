// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//Networking
#include "Networking.h"
#include "Engine.h"
#include <time.h>

#include "Components/ActorComponent.h"
#include "UDPImageStreamer.generated.h"

const int PACKAGESIZE = 4096;
const int HEADERSIZE = 8 + 4;
const int BUFFERSIZE = PACKAGESIZE - HEADERSIZE;

USTRUCT(BlueprintType)
struct FImageSegmentPackage
{
	GENERATED_USTRUCT_BODY()
			
	float frameTime = 0;

	int idx = 0;

	TArray<uint8, TFixedAllocator<BUFFERSIZE>> imageData;

	FImageSegmentPackage()
	{}
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FImageSegmentPackage& imageSegment)
{
	Ar << imageSegment.frameTime;
	Ar << imageSegment.idx;
	Ar << imageSegment.imageData;
	return Ar;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGetReceivedData, const FImageSegmentPackage&, receivedTex);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LEAPTEST_API UUDPImageStreamer : public UActorComponent
{
	GENERATED_BODY()

private:
	FSocket* socket;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	time_t classInitTime;

	UTexture2D* dynamicTex;

	/*struct prevTexInfo_t {
		EPixelFormat pFormat;
		uint32 width;
		uint32 height;
	} prevTexInfo;*/
	float prevFrameTime;

public:	
	// Sets default values for this component's properties
	UUDPImageStreamer();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TSharedPtr<FInternetAddr>	RemoteAddr;

	UPROPERTY(BlueprintAssignable, Category = "Internet")
		FGetReceivedData OnSuccess;

	UFUNCTION(BlueprintCallable, Category = "Internet")
		bool sendImage(UTexture2D *tex);

	bool sendSegment(FImageSegmentPackage data);

	bool initSending(FString IP, int32 port);

	bool initReceiving(FString IP, int32 port);

	UFUNCTION(BlueprintCallable, Category = "Internet")
		bool startStreamer(FString clientIP, FString serverIp, int32 port);

	const FString EnumToString(const TCHAR* Enum, int32 EnumValue);

	void closeConnection();

	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);
};
