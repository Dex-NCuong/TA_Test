#include "EnemyCreationWizardRuntimeBridge.h"

FEnemyCreationWizardOpenRequested& FEnemyCreationWizardRuntimeBridge::OnOpenWizardRequested()
{
    static FEnemyCreationWizardOpenRequested OpenWizardRequested;
    return OpenWizardRequested;
}
