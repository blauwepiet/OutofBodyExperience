// Fill out your copyright notice in the Description page of Project Settings.

#include "LeapTest.h"
#include "UDPImageStreamer.h"

// Sets default values for this component's properties
UUDPImageStreamer::UUDPImageStreamer()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	classInitTime = time(0);
}

void UUDPImageStreamer::BeginPlay() 
{
	Super::BeginPlay();

	nrOfBytesToSend = 0;
	nrOfPackagesToSend = 0;
	nrOfPackagesReceived = 0;

	OnSuccess.AddDynamic(this, &UUDPImageStreamer::updateTexture);
}

UTexture2D* UUDPImageStreamer::createDynamicOutputTex(UTexture2D *tex) {
	dynamicTex = UTexture2D::CreateTransient(tex->GetSizeX(), tex->GetSizeY(), tex->GetPixelFormat());
	dynamicTex->MipGenSettings = tex->MipGenSettings;
	dynamicTex->PlatformData->NumSlices = tex->PlatformData->NumSlices;
	dynamicTex->NeverStream = tex->NeverStream;

	FByteBulkData* rawImageData = &dynamicTex->PlatformData->Mips[0].BulkData;
	FColor* pixels = (FColor*)rawImageData->Lock(LOCK_READ_WRITE);
	int32 numPixels = dynamicTex->GetSizeX() * dynamicTex->GetSizeY();

	for (int p = 0; p < numPixels; p++) {
		pixels[p] = FColor::MakeRedToGreenColorFromScalar(1.0 / numPixels);
	}

	rawImageData->Unlock();
	dynamicTex->UpdateResource();

	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Prepped dym tex");
}

UTexture2D* UUDPImageStreamer::makeDymTexRandom() {
	FByteBulkData* rawImageData = &dynamicTex->PlatformData->Mips[0].BulkData;
	FColor* pixels = (FColor*)rawImageData->Lock(LOCK_READ_WRITE);
	int32 numPixels = dynamicTex->GetSizeX() * dynamicTex->GetSizeY();

	for (int p = 0; p < numPixels; p++) {
		pixels[p] = FColor::MakeRandomColor();
	}

	rawImageData->Unlock();
	dynamicTex->UpdateResource();
}

void UUDPImageStreamer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	this->closeConnection();
}

bool UUDPImageStreamer::initReceiving(FString IP, int32 port)
{
	if (!socket)
	{
		return false;
	}

	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(500);
	UDPReceiver = new FUdpSocketReceiver(socket, ThreadWaitTime, TEXT("UDP RECEIVER"));
	UDPReceiver->OnDataReceived().BindUObject(this, &UUDPImageStreamer::Recv);
	UDPReceiver->Start();

	return true;
}

bool UUDPImageStreamer::initSending(FString IP, int32 port)
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

	return true;
}

bool UUDPImageStreamer::startStreamer(FString clientIP, FString serverIp, int32 port) {
	FIPv4Address Addr;
	FIPv4Address::Parse(clientIP, Addr);

	//Create Socket
	FIPv4Endpoint Endpoint(Addr, port);

	FString socketname = FString(TEXT("streamerSocket"));

	socket = FUdpSocketBuilder(*socketname)
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(PACKAGESIZE)
		.WithSendBufferSize(PACKAGESIZE);

	if (!socket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Socket not initialized properly");
		return false; 
	}

	bool recvInitialized = this->initReceiving(clientIP, port);
	bool sendInitialized = this->initSending(serverIp, port);

	if (recvInitialized && sendInitialized)
	{
		return true;
	}

	return false;
}

bool UUDPImageStreamer::sendSegment(FImageSegmentPackage data)
{
	FArrayWriter Writer;
	Writer << data;
	int32 BytesSent = 0;
	socket->SendTo(Writer.GetData(), Writer.Num(), BytesSent, *RemoteAddr);

	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Send, " + FString::FromInt(BytesSent));

	if (BytesSent <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "invalid send");
		return false;
	}

	return true;
}

bool UUDPImageStreamer::sendImage(UTexture2D *tex)
{
	if (!tex) {
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Tex is null ptr");
		return false;
	}

	FByteBulkData* rawImageData = &tex->PlatformData->Mips[0]->BulkData;

	if (!&rawImageData) {
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Rawimagedata is null ptr");
		return false;
	}

	uint8* formatedImageData = (uint8*)rawImageData->Lock(LOCK_READ_WRITE);

	if (!formatedImageData) {
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Formatedimagedata is null ptr");
		rawImageData->Unlock();
		return false;
	}

	uint32 textureWidth = tex->GetSizeX(), textureHeight = tex->GetSizeX();
	nrOfBytesToSend = textureWidth * textureHeight * 4;
	nrOfPackagesToSend = (nrOfBytesToSend + BUFFERSIZE - 1) / BUFFERSIZE;

	FImageSegmentPackage imageSegmentPackage;

	imageSegmentPackage.frameTime = (float)difftime(time(0), classInitTime);

	for (uint32 i = 0; i < nrOfPackagesToSend; i++) {
		imageSegmentPackage.idx = i;
		uint8* segmentDataPtr = imageSegmentPackage.imageData.GetData();
		uint8* imageDataSegment = formatedImageData + i*BUFFERSIZE;
		uint32 residue = (i*BUFFERSIZE + BUFFERSIZE) - nrOfBytesToSend;
		if (residue < 0) {
			FMemory::Memcpy(segmentDataPtr, imageDataSegment, BUFFERSIZE);
		}
		else {
			FMemory::Memcpy(segmentDataPtr, imageDataSegment, BUFFERSIZE-residue);
		}
		sendSegment(imageSegmentPackage);
	}

	rawImageData->Unlock();

	return true;
}

void UUDPImageStreamer::closeConnection()
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

const FString UUDPImageStreamer::EnumToString(const TCHAR* Enum, int32 EnumValue)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, Enum, true);
	if (!EnumPtr)
		return NSLOCTEXT("Invalid", "Invalid", "Invalid").ToString();

#if WITH_EDITOR
	return EnumPtr->GetDisplayNameText(EnumValue).ToString();
#else
	return EnumPtr->GetEnumName(EnumValue);
#endif
}

void UUDPImageStreamer::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "RECEIVING DATA");
	FImageSegmentPackage Data;
	*ArrayReaderPtr << Data;
	FImageSegmentPackage copy;

	copy.imageData = Data.imageData;
	nrOfPackagesReceived++;

	FByteBulkData* rawImageData = &dynamicTex->PlatformData->Mips[0].BulkData;
	uint8* imageData= (uint8*)rawImageData->Lock(LOCK_READ_WRITE);
	uint8* imageDataFragment = imageData + Data.idx*BUFFERSIZE;
	uint8* receivedImageSegment = Data.imageData.GetData();
	uint32 residue = (Data.idx*BUFFERSIZE + BUFFERSIZE) - nrOfBytesToSend;
	if (residue < 0) {
		FMemory::Memcpy(imageDataFragment, receivedImageSegment, BUFFERSIZE);
	}
	else {
		FMemory::Memcpy(imageDataFragment, receivedImageSegment, BUFFERSIZE - residue);
	}

	if (copy.frameTime != prevFrameTime) {
		this->OnSuccess.Broadcast();
		prevFrameTime = Data.frameTime;
		nrOfPackagesReceived = 0;
	}
}

void UUDPImageStreamer::updateTexture() {
	FByteBulkData* rawImageData = &dynamicTex->PlatformData->Mips[0].BulkData;
	uint8* imageData = (uint8*)rawImageData->Lock(LOCK_READ_WRITE);

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdateDynamicTextureCode,
		UTexture2D*, pTexture, dynamicTex,
		const uint8*, pData, imageData,
		{
			FUpdateTextureRegion2D region;
			region.SrcX = 0;
			region.SrcY = 0;
			region.DestX = 0;
			region.DestY = 0;
			region.Width = dynamicTex->GetSizeX();
			region.Height = dynamicTex->GetSizeY();

			FTexture2DResource* resource = (FTexture2DResource*)pTexture->Resource;
			RHIUpdateTexture2D(resource->GetTexture2DRHI(), 0, region, region.Width * 4, pData);
		});


}