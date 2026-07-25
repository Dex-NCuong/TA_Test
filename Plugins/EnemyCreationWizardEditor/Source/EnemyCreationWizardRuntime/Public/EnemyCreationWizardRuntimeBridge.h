#pragma once

#include "Delegates/Delegate.h"

DECLARE_DELEGATE_RetVal(bool, FEnemyCreationWizardOpenRequested);

/**
 * Editor-owned callback used by runtime Blueprints without introducing editor dependencies
 * into the runtime module.
 */
class ENEMYCREATIONWIZARDRUNTIME_API FEnemyCreationWizardRuntimeBridge
{
public:
    static FEnemyCreationWizardOpenRequested& OnOpenWizardRequested();
};
