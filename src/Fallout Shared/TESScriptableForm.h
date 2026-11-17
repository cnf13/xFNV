#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class Script;
class TESForm;

// The script attached to a form.
class TESScriptableForm : public BaseFormComponent {
public:
    TESScriptableForm();

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // Resolves the script loaded for apOwner.
    void InitItem(TESForm* apOwner);
    // Saves the component's chunk.
    void Save();
    // Sets pFormScript.
    void SetFormScript(Script* apScript);
    // Returns pFormScript.
    Script* GetFormScript();

    // The script of the form's scriptable component, or null.
    static Script* GetFormScript(TESForm* apForm);
    static void SetFormScript(TESForm* apForm, Script* apScript);

    Script* pFormScript;
    uint8_t bIsInitialized;
    uint8_t padding09[3];
};
