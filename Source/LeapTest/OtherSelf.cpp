// Fill out your copyright notice in the Description page of Project Settings.

#include "LeapTest.h"
#include "OtherSelf.h"


// Sets default values
AOtherSelf::AOtherSelf()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AOtherSelf::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOtherSelf::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

