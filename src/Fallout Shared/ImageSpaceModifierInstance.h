#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class ImageSpaceModifierInstanceForm;
class TESImageSpaceModifier;

// A playing image space modifier: its strength, target and start time. Only some of its members are declared.
class ImageSpaceModifierInstance : public NiObject {
public:
	ImageSpaceModifierInstance(float a1, float a2, NiAVObject* a3);

	// IsExpired and IsForm are the defaults of an instance that never expires and is not a form instance.
	virtual bool IsExpired() { return false; }
	virtual void Apply() = 0;
	virtual ImageSpaceModifierInstanceForm* IsForm() { return nullptr; }
	// Writes the instance's description into apDest (0x800 bytes).
	virtual void PrintInfo(char* apDest) = 0;
	~ImageSpaceModifierInstance() override;

	// Sets the instance's strength.
	void SetStrength(float afStrength);
	// Keeps the instance from expiring.
	void SetPermanent(bool abValue);
	// Whether the instance never expires: bit 0 of iFlags.
	bool QPermanent();
	float QStrength();

	static void Stop(ImageSpaceModifierInstance* apInstance);

	bool bIsWeather;
	float fStrength;
	NiPointer<NiAVObject> spTarget;
	float fStartTime;
	uint32_t iFlags;
};

// A playing instance of an image space modifier form.
class ImageSpaceModifierInstanceForm : public ImageSpaceModifierInstance {
public:
	ImageSpaceModifierInstanceForm();
	ImageSpaceModifierInstanceForm(TESImageSpaceModifier* apMod, float afStrength, float afAge, NiAVObject* apTarget);

	bool IsExpired() override;
	void Apply() override;
	ImageSpaceModifierInstanceForm* IsForm() override { return this; }
	void PrintInfo(char* apBuffer) override;
	~ImageSpaceModifierInstanceForm() override;

	void SetImageSpaceModifier(TESImageSpaceModifier* apMod);
	void SetAge(float a2);
	bool StartTransition(TESImageSpaceModifier* apMod, float afTransitionTime, float afStrength, NiAVObject* apTarget);

	// Stops the instances of apMod.
	static void Stop(TESImageSpaceModifier* apMod);
	// Starts an instance of apMod at strength afStrength on apTarget (null: the screen).
	static ImageSpaceModifierInstanceForm* Trigger(TESImageSpaceModifier* apMod, float afStrength,
		NiAVObject* apTarget);
	static ImageSpaceModifierInstanceForm* TriggerIfNotActive(TESImageSpaceModifier* apMod, float afStrength,
		NiAVObject* apTarget);

	TESImageSpaceModifier* pImageSpaceMod;
	TESImageSpaceModifier* pLastImageSpaceMod;
	float fLastStrength;
	NiPointer<NiAVObject> spLastTarget;
	float fTransitionTime;
};

NiSmartPointer(ImageSpaceModifierInstanceForm);
