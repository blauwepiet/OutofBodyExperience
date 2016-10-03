// Fill out your copyright notice in the Description page of Project Settings.

#include "LeapTest.h"
#include "UDPConnection.h"


// Sets default values for this component's properties
UUDPConnection::UUDPConnection()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UUDPConnection::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

// Called every frame
void UUDPConnection::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// ...
}

bool UUDPConnection::startServer(FString socketname, FString IP, int32 port)
{
	FIPv4Address Addr;
	FIPv4Address::Parse(IP, Addr);

	//Create Socket
	FIPv4Endpoint Endpoint(Addr, port);

	//BUFFER SIZE
	int32 BufferSize = 4096;

	socket = FUdpSocketBuilder(*socketname)
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(BufferSize);
	;

	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(500);
	UDPReceiver = new FUdpSocketReceiver(socket, ThreadWaitTime, TEXT("UDP RECEIVER"));
	UDPReceiver->OnDataReceived().BindUObject(this, &UUDPConnection::Recv);
	UDPReceiver->Start();

	if (socket)
	{
		isServer = true;
	}
	return isServer;
}

bool UUDPConnection::tryToConnect(FString socketname, FString IP, int32 port, bool UDP)
{
	RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool bIsValid;
	RemoteAddr->SetIp(*IP, bIsValid);
	RemoteAddr->SetPort(port);

	if (!bIsValid)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "IP address was not valid!");
		return false;
	}

	socket = FUdpSocketBuilder(*socketname).BoundToPort(port).AsReusable().WithBroadcast();
	//Set Send Buffer Size
	int32 SendSize = 2 * 1024 * 1024;
	socket->SetSendBufferSize(SendSize, SendSize);
	socket->SetReceiveBufferSize(SendSize, SendSize);

	if (socket)
	{
		return true;
	}

	return false;
}

bool UUDPConnection::sendData(FAnyCustomData data)
{
	if (!socket || isServer)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "no sender socket");
		return false;
	}

	FArrayWriter Writer;
	Writer << data;
	int32 BytesSent = 0;
	socket->SendTo(Writer.GetData(), Writer.Num(), BytesSent, *RemoteAddr);
	if (BytesSent <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "invalid send");
		return false;
	}

	return true;
}

void UUDPConnection::closeConnection()
{
	if (UDPReceiver)
	{
		delete UDPReceiver;
		UDPReceiver = nullptr;
	}

	if (socket)
	{
		socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(socket);
	}

}

void UUDPConnection::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "RECEIVING DATA");
	FAnyCustomData Data;
	*ArrayReaderPtr << Data;
	FAnyCustomData copy;
	copy.motor = Data.motor;
	copy.servo = Data.servo;
	dataRev = copy;
	this->OnSuccess.Broadcast(dataRev);
}

FAnyCustomData UUDPConnection::getDataRev()
{
	return dataRev;
}