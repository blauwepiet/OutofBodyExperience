// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//Networking
#include "Networking.h"
#include "Engine.h"
#include <time.h>

#include "Components/ActorComponent.h"
#include "UDPImageStreamer.generated.h"

const int PACKAGESIZE = 4096;
const int HEADERSIZE = 4 + 4;
const int BUFFERSIZE = PACKAGESIZE - HEADERSIZE;

DECLARE_LOG_CATEGORY_EXTERN(UDPImageStreamerLogger, Log, All);

USTRUCT(BlueprintType)
struct FImageSegmentPackage
{
	GENERATED_USTRUCT_BODY()
			
	float frameTime = 0;

	int idx = 0;

	uint8 imageData[BUFFERSIZE];

	FImageSegmentPackage()
	{}
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FImageSegmentPackage& imageSegment)
{
	Ar << imageSegment.frameTime;
	Ar << imageSegment.idx;
	for (int i = 0; i < BUFFERSIZE; i++) {
		Ar << imageSegment.imageData[i];
	}	
	return Ar;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGetReceivedData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LEAPTEST_API UUDPImageStreamer : public UActorComponent
{
	GENERATED_BODY()

private:
	FSocket* socket;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	clock_t classInitTime;

	UTexture2D* dynamicTex;
	UTexture2D* textureToSend;
	uint8* dymTexData;

	uint32 nrOfBytesToSend;
	uint32 nrOfPackagesToSend;
	uint32 nrOfPackagesReceived;
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

	UFUNCTION(BlueprintCallable, Category = "Internet")
		UTexture2D* createDynamicOutputTex(UTexture2D *tex);

	UFUNCTION(BlueprintCallable, Category = "Internet")
		UTexture2D* makeDymTexRandom();

	bool sendSegment(FImageSegmentPackage data);

	bool sendBinaryData(uint8* data, uint32 packageLength);

	bool initSending(FString IP, int32 port);

	bool initReceiving(FString IP, int32 port);

	UFUNCTION(BlueprintCallable, Category = "Internet")
		bool startStreamer(FString clientIP, FString serverIp, int32 port);

	const FString EnumToString(const TCHAR* Enum, int32 EnumValue);

	UFUNCTION()
		void updateTexture();

	void closeConnection();

	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);
};
