// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//Networking
#include "Networking.h"
#include "Engine.h"

#include "Components/ActorComponent.h"
#include "UDPConnection.generated.h"


USTRUCT(BlueprintType)
struct FAnyCustomData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DATA")
		float motor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DATA")
		float servo = 0;

	FAnyCustomData()
	{}
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FAnyCustomData& TheStruct)
{
	Ar << TheStruct.motor;
	Ar << TheStruct.servo;
	return Ar;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGetReceivedData, const FAnyCustomData&, receivedData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LEAPTEST_API UUDPConnection : public UActorComponent
{
	GENERATED_BODY()

private:
	bool isServer = false;
	FSocket* socket;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	FAnyCustomData dataRev;

public:	
	// Sets default values for this component's properties
	UUDPConnection();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	TSharedPtr<FInternetAddr>	RemoteAddr;

	UPROPERTY(BlueprintAssignable)
		FGetReceivedData OnSuccess;

	UFUNCTION(BlueprintPure, Category = "Internet")
		FAnyCustomData getDataRev();

	UFUNCTION(BlueprintCallable, Category = "Internet")
		bool sendData(FAnyCustomData data);

	UFUNCTION(BlueprintCallable, Category = "Internet")
		bool tryToConnect(FString socketname, FString IP, int32 port, bool UDP = false);

	UFUNCTION(BlueprintCallable, Category = "Internet")
		bool startServer(FString socketname, FString IP, int32 port);

	UFUNCTION(BlueprintCallable, Category = "Internet")
		void closeConnection();

	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);
};
