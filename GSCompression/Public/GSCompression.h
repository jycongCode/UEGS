#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGSCompressionModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
