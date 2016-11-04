// Fill out your copyright notice in the Description page of Project Settings.

#include "LeapTest.h"
#include "httpFunctionCaller.h"


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

UTexture2D* UhttpFunctionCaller::setupTextures()
{
	dynamicPNGTex = UTexture2D::CreateTransient(400, 400, PF_B8G8R8A8);
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
	if (bWasSuccessful) {
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Retreived something with response code: " + Response->GetResponseCode());
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Http request was not succesfull");
	}
	

	//IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	//IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	//if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(Response->GetContent().GetData(), Response->GetContent().Num())) {
	//	const TArray<uint8>* UncompressedBGRA = NULL;
	//	if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
	//	{
	//		void* TextureData = dynamicPNGTex->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	//		FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
	//		dynamicPNGTex->PlatformData->Mips[0].BulkData.Unlock();

	//		// Update the rendering resource from data.
	//		dynamicPNGTex->UpdateResource();
	//	}
	//}
	this->OnSuccess.Broadcast(true);
}