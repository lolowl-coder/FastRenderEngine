#pragma once

#include "Serialization/BaseTypesSerialization.hpp"

#include <rapidjson/document.h>

#include <glm/glm.hpp>

namespace fre
{
    inline void serialize(rapidjson::Value& v, const char* n, const glm::vec2& m, rapidjson::Document& d)
    {
        rapidjson::Value tmp(rapidjson::kObjectType);
        SERIALIZE_MEMBER(tmp, m, x);
        SERIALIZE_MEMBER(tmp, m, y);
        v.AddMember(rapidjson::StringRef(n), tmp, d.GetAllocator());
    }

    inline void deserialize(const rapidjson::Value& v, const char* n, glm::vec2& result)
    {
        if(v.HasMember(n))
        {
            const rapidjson::Value& tmp = v[n];

            DESERIALIZE_MEMBER(tmp, result, x);
            DESERIALIZE_MEMBER(tmp, result, y);
        }
    }

    inline void serialize(rapidjson::Value& v, const char* n, const glm::vec4& m, rapidjson::Document& d)
    {
        rapidjson::Value tmp(rapidjson::kObjectType);
        SERIALIZE_MEMBER(tmp, m, x);
        SERIALIZE_MEMBER(tmp, m, y);
        SERIALIZE_MEMBER(tmp, m, z);
        SERIALIZE_MEMBER(tmp, m, w);
        v.AddMember(rapidjson::StringRef(n), tmp, d.GetAllocator());
    }

    inline void deserialize(const rapidjson::Value& v, const char* n, glm::vec4& result)
    {
        if(v.HasMember(n))
        {
            const rapidjson::Value& tmp = v[n];

            DESERIALIZE_MEMBER(tmp, result, x);
            DESERIALIZE_MEMBER(tmp, result, y);
            DESERIALIZE_MEMBER(tmp, result, z);
            DESERIALIZE_MEMBER(tmp, result, w);
        }
    }

    inline void serialize(rapidjson::Value& v, const char* n, const fre::BoundingBox2D& m, rapidjson::Document& d)
    {
        rapidjson::Value tmp(rapidjson::kObjectType);
        SERIALIZE_MEMBER(tmp, m, mMax);
        SERIALIZE_MEMBER(tmp, m, mMin);
        v.AddMember(rapidjson::StringRef(n), tmp, d.GetAllocator());
    }

    inline void deserialize(const rapidjson::Value& v, const char* n, fre::BoundingBox2D& result)
    {
        if(v.HasMember(n))
        {
            const rapidjson::Value& tmp = v[n];

            DESERIALIZE_MEMBER(tmp, result, mMin);
            DESERIALIZE_MEMBER(tmp, result, mMax);
        }
    }
}