#pragma once

#include "BSCore/BSSimpleList.h"

class Actor;
class TESForm;
class TESPackage;

bool TestHour(int32_t aitime, int32_t aiduration, float afhour);

// An actor's AI package list: the packages in the order they are evaluated. The members (InitItem, FindMissedPackages,
// ...) run on the list in place.
class PackageList {
public:
    // Resolves the form IDs the list holds after loading to packages, dropping the ones not found.
    void InitItem(TESForm* apOwner);
    // Whether the list differs from apCompare's: another package at some position, or another length.
    bool Compare(PackageList* apCompare);
    // The first package whose schedule and conditions let the actor run it now.
    TESPackage* FindCurrentPackage(Actor* apActor);
    // Collects the must-complete packages the actor missed since its process last ran, sorted by schedule.
    void FindMissedPackages(Actor* apActor, BSSimpleList<TESPackage*>* apPacks, float afStartdate, float afhour);

    BSSimpleList<TESPackage*> listPackages;
};
