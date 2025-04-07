#pragma once

#define SET_FEATURE_MEMBER(fstruct, member) fstruct.member = VK_TRUE; mDeviceFeaturesEnabled.push_back(&fstruct.member);
#define FOR_EACH_1(macro, fstruct, x)  macro(fstruct, x)
#define FOR_EACH_2(macro, fstruct, x, ...)  macro(fstruct, x) FOR_EACH_1(macro, fstruct, __VA_ARGS__)
#define FOR_EACH_3(macro, fstruct, x, ...)  macro(fstruct, x) FOR_EACH_2(macro, fstruct, __VA_ARGS__)
#define FOR_EACH_4(macro, fstruct, x, ...)  macro(fstruct, x) FOR_EACH_3(macro, fstruct, __VA_ARGS__)
// ... add more if needed

#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define FOR_EACH(macro, fstruct, ...) GET_MACRO(__VA_ARGS__, FOR_EACH_4, FOR_EACH_3, FOR_EACH_2, FOR_EACH_1)(macro, fstruct, __VA_ARGS__)

#define REQUEST_FEATURE(structType, structId, ...)                            \
    {                                                                               \
        auto storage = std::make_unique<fre::FeatureStorage<structType>>();             \
        auto* feature = static_cast<structType*>(storage->get());                  \
        feature->sType = structId;                                                 \
        feature->pNext = nullptr;                                                  \
        *mLastDeviceFeatures = feature;                                            \
        mLastDeviceFeatures = &feature->pNext;                                     \
        FOR_EACH(SET_FEATURE_MEMBER, (*feature), __VA_ARGS__)                      \
        mFeatureChain.push_back(std::move(storage));                               \
    }