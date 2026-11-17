#pragma once

class NiLookAtController;
class NiObject;
class NiObjectNET;
class NiPathController;
class NiRollController;
class NiStream;
class NiTimeController;

// Converts the keyframe controllers of files older than 10.1.0.104 to interpolator-based animation. Only some of the
// members are declared.
class NiOldAnimationConverter {
public:
    // Converts pkObject's scene when the stream is older than 10.1.0.104 and pkObject is an NiObjectNET.
    static void Convert(NiStream& kStream, NiObject* pkObject);
    // Converts the old controllers of pkObject and, for a node, of its children, when pkObject is an NiAVObject.
    static void RecurseScene(NiStream& kStream, NiObjectNET* pkObject);
    static void ConvertRollController(NiRollController* pkController);
    static void ConvertLookAtController(NiLookAtController* pkController);
    static void ConvertPathController(NiPathController* pkController);
    static void SynchronizeTimeControllers(const NiTimeController* pkSrc, NiTimeController* pkDest);
};
