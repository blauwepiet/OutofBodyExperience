// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//Networking
#include "Networking.h"
#include "Engine.h"
#include <time.h>
#include <chrono>
#include <thread>

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

/**
* Asynchronously sends data to an UDP socket.
*
* @todo networking: gmp: implement rate limits
*/
class FUdpSocketSenderBinary
	: public FRunnable
{

public:

	/**
	* Creates and initializes a new socket sender.
	*
	* @param InSocket The UDP socket to use for sending data.
	* @param ThreadDescription The thread description text (for debugging).
	*/
	FUdpSocketSenderBinary(FSocket* InSocket, const TCHAR* ThreadDescription, TSharedPtr<FInternetAddr>	RemoteAddr)
		: SendRate(0)
		, Socket(InSocket)
		, Stopping(false)
		, WaitTime(FTimespan::FromMilliseconds(100))
		, RemoteAddr(RemoteAddr)
	{
		check(Socket != nullptr);
		check(Socket->GetSocketType() == SOCKTYPE_Datagram);

		WorkEvent = FPlatformProcess::GetSynchEventFromPool();
		Thread = FRunnableThread::Create(this, ThreadDescription, 128 * 1024, TPri_AboveNormal, FPlatformAffinity::GetPoolThreadMask());
	}

	/** Virtual destructor. */
	virtual ~FUdpSocketSenderBinary()
	{
		if (Thread != nullptr)
		{
			Thread->Kill(true);
			delete Thread;
		}

		FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
		WorkEvent = nullptr;
	}

public:

	/**
	* Gets the maximum send rate (in bytes per second).
	*
	* @return Current send rate.
	*/
	uint32 GetSendRate() const
	{
		return SendRate;
	}

	/**
	* Gets the current throughput (in bytes per second).
	*
	* @return Current throughput.
	*/
	uint32 GetThroughput() const
	{
		return Throughput;
	}

	/**
	* Sends data to the specified recipient.
	*
	* @param Data The data to send.
	* @param Recipient The recipient.
	* @return true if the data will be sent, false otherwise.
	*/
	bool Send(const TSharedRef<TArray<uint8, TFixedAllocator<PACKAGESIZE>>, ESPMode::ThreadSafe>& Data)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Queueing: " + FString::FromBlob(Data,20));
		if (!Stopping && SendQueue.Enqueue(Data))
		{
			WorkEvent->Trigger();

			return true;
		}

		return false;
	}

	/**
	* Sets the send rate (in bytes per second).
	*
	* @param Rate The new send rate (0 = unlimited).
	* @see SetWaitTime
	*/
	void SetSendRate(uint32 Rate)
	{
		SendRate = Rate;
	}

	/**
	* Sets the maximum time span to wait for work items.
	*
	* @param Timespan The wait time.
	* @see SetSendRate
	*/
	void SetWaitTime(const FTimespan& Timespan)
	{
		WaitTime = Timespan;
	}

public:

	// FRunnable interface

	virtual bool Init() override
	{
		return true;
	}

	virtual uint32 Run() override
	{
		while (!Stopping)
		{
			while (!SendQueue.IsEmpty())
			{
				if (Socket->Wait(ESocketWaitConditions::WaitForWrite, WaitTime))
				{
					TSharedPtr<TArray<uint8, TFixedAllocator<PACKAGESIZE>>, ESPMode::ThreadSafe> Packet;
					int32 Sent = 0;

					SendQueue.Dequeue(Packet);
					//GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Queueing: " + FString::FromBlob(Packet, 20));
					Socket->SendTo(Packet->GetData(), PACKAGESIZE, Sent, *RemoteAddr);
					Packet->Empty();
					Packet.Reset();
					if (Sent <= 0)
					{
						GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Invalid sent");
						//return 0;
					}
				}
			}

			WorkEvent->Wait(WaitTime);
		}

		return 0;
	}

	virtual void Stop() override
	{
		Stopping = true;
		WorkEvent->Trigger();
	}

	virtual void Exit() override { }

private:

	/** Holds the send queue. */
	TQueue<TSharedPtr<TArray<uint8, TFixedAllocator<PACKAGESIZE>>, ESPMode::ThreadSafe>, EQueueMode::Mpsc> SendQueue;

	/** Holds the send rate. */
	uint32 SendRate;

	/** Holds the network socket. */
	FSocket* Socket;

	/** Holds a flag indicating that the thread is stopping. */
	bool Stopping;

	/** Holds the thread object. */
	FRunnableThread* Thread;

	/** Holds the current throughput. */
	uint32 Throughput;

	/** Holds the amount of time to wait for inbound packets. */
	FTimespan WaitTime;

	/** Holds an event signaling that inbound messages need to be processed. */
	FEvent* WorkEvent;

	TSharedPtr<FInternetAddr>	RemoteAddr;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGetReceivedData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LEAPTEST_API UUDPImageStreamer : public UActorComponent
{
	GENERATED_BODY()

private:
	FSocket* socket;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	FUdpSocketSenderBinary* UDPSender = nullptr;
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
	clock_t startSending;

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

	bool sendBinaryData(const TSharedRef<TArray<uint8, TFixedAllocator<PACKAGESIZE>>, ESPMode::ThreadSafe>& data, uint32 packageLength);

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
