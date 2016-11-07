// Fill out your copyright notice in the Description page of Project Settings.

#include "LeapTest.h"
#include "httpFunctionCaller.h"
#include <iostream>
#include <fstream>

DEFINE_LOG_CATEGORY(httpFunctionCallerLogger)
// Sets default values for this component's properties
UhttpFunctionCaller::UhttpFunctionCaller()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	httpRequester = &FHttpModule::Get();
	// ...
}


// Called when the game starts
void UhttpFunctionCaller::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UhttpFunctionCaller::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// ...
}

UTexture2D* UhttpFunctionCaller::setupTextures(UTexture2D* defaultTex)
{
	dynamicPNGTex = UTexture2D::CreateTransient(400, 400, PF_B8G8R8A8);

	FByteBulkData* rawImageData = &dynamicPNGTex->PlatformData->Mips[0].BulkData;
	FByteBulkData* defaultTexRawImageData = &defaultTex->PlatformData->Mips[0].BulkData;
	uint8* dymData = (uint8*)rawImageData->Lock(LOCK_READ_WRITE);
	uint8* defData = (uint8*)defaultTexRawImageData->Lock(LOCK_READ_WRITE);
	int32 numPixels = dynamicPNGTex->GetSizeX() * dynamicPNGTex->GetSizeY();
	
	FMemory::Memcpy(dymData, defData, numPixels * 4);

	defaultTexRawImageData->Unlock();
	rawImageData->Unlock();
	dynamicPNGTex->UpdateResource();
	return dynamicPNGTex;
}

bool UhttpFunctionCaller::httpFunctionCall(FString requestURL)
{
	TSharedRef<IHttpRequest> Request = httpRequester->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UhttpFunctionCaller::OnReceive);
	//This is the url on which to process the request
	Request->SetURL(requestURL);
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", TEXT("image/png"));
	return Request->ProcessRequest();
}

/*Assigned function on successfull http call*/
void UhttpFunctionCaller::OnReceive(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	//GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Retreived http response");
	if (bWasSuccessful) {
		//GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Retreived something with response code: " + FString::FromInt(Response->GetResponseCode()));
		
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(Response->GetContent().GetData(), Response->GetContent().Num())) {
			const TArray<uint8>* UncompressedBGRA = NULL;
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
			{
				//UE_LOG(httpFunctionCallerLogger, Log, TEXT("Raw png data: %s"), *FString::FromBlob(UncompressedBGRA->GetData(), UncompressedBGRA->Num()));
				void* TextureData = dynamicPNGTex->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
				dynamicPNGTex->PlatformData->Mips[0].BulkData.Unlock();

				// Update the rendering resource from data.
				dynamicPNGTex->UpdateResource();
			}
		}
		this->OnSuccess.Broadcast(true);
	}
	else {
		//GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Http request was not succesfull");
		this->OnSuccess.Broadcast(false);
	}
		
	
}