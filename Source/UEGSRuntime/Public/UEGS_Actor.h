#pragma once
#include "UEGS_Actor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class A_UEGS_Actor : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Components")
	class U_UEGS_Component* GS_Component;
	
	A_UEGS_Actor();
};
