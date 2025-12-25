#include "UEGS_Actor.h"

#include "UEGS_Component.h"


A_UEGS_Actor::A_UEGS_Actor()
{
	GS_Component = CreateDefaultSubobject<U_UEGS_Component>(TEXT("Gaussian Splat"));
	RootComponent = GS_Component;
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("U_UEGS_Actor::BeginPlay() Not sub"));
	}
}
