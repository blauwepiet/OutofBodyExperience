// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Engine.h"
#include "Runtime/ImageWrapper/Public/Interfaces/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/Interfaces/IImageWrapperModule.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "httpFunctionCaller.generated.h"

USTRUCT(BlueprintType)
struct FBogus
{
	GENERATED_USTRUCT_BODY()

		float frameTime = 0;

	int idx = 0;

	uint8 imageData[1];

	FBogus()
	{}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReceivedHTTPData, bool, succeeded);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LEAPTEST_API UhttpFunctionCaller : public UActorComponent
{
	GENERATED_BODY()

public:	
	FHttpModule* httpRequester;
	UTexture2D* dynamicPNGTex;

	// Sets default values for this component's properties
	UhttpFunctionCaller();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UFUNCTION(BlueprintCallable, Category = "Internet")
	UTexture2D* setupTextures();

	/* The actual HTTP call */
	UFUNCTION(BlueprintCallable, Category = "Internet")
		bool httpFunctionCall(FString requestURL);

	UPROPERTY(BlueprintAssignable, Category = "Internet")
		FReceivedHTTPData OnSuccess;

	/*Assign this function to call when the GET request processes sucessfully*/
	void OnReceive(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	
};
