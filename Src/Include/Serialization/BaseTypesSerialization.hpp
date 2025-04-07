#pragma once

#include "Utilities.hpp"

#include <rapidjson/document.h>

#include <map>

//v - value
//m - member
//d - document
#define SERIALIZE(v, m) serialize(v, #m, m, d)
//o - object
#define SERIALIZE_MEMBER(v, o, m) serialize(v, #m, o.m, d)

//Deserialization
#define DESERIALIZE(v, m) deserialize(v, #m, m)
#define DESERIALIZE_MEMBER(v, o, m) deserialize(v, #m, o.m)

namespace fre
{
    //Serialize basic types
    template<typename T>
    void serialize(rapidjson::Value& v, const char* n, const T m, rapidjson::Document& d)
    {
        v.AddMember(rapidjson::StringRef(n), m, d.GetAllocator());
    }

    //Serialize string
    inline void serialize(rapidjson::Value& v, const char* n, const std::string& m, rapidjson::Document& d)
    {
        v.AddMember(rapidjson::StringRef(n), rapidjson::StringRef(m.c_str()), d.GetAllocator());
    }

    //Serialize array
    template<typename T>
    void serialize(rapidjson::Value& v, const char* n, const std::vector<T>& m, rapidjson::Document& d)
    {
        rapidjson::Value result(rapidjson::kArrayType);
        for(const auto& element : m)
        {
            rapidjson::Value tmp(rapidjson::kObjectType);
            SERIALIZE(tmp, element);
            result.PushBack(tmp, d.GetAllocator());
        }
        v.AddMember(rapidjson::StringRef(n), result, d.GetAllocator());
    }

    //Serialize map
    template<typename K, typename T>
    void serialize(rapidjson::Value& v, const char* n, const std::map<K,T>& m, rapidjson::Document& d)
    {
        rapidjson::Value result(rapidjson::kArrayType);
        for(const auto& element : m)
        {
            rapidjson::Value tmp(rapidjson::kObjectType);
            //Key
            const auto& key = element.first;
            SERIALIZE(tmp, key);
            //Value
            const auto& value = element.second;
            SERIALIZE(tmp, value);
            result.PushBack(tmp, d.GetAllocator());
        }
        v.AddMember(rapidjson::StringRef(n), result, d.GetAllocator());
    }

    //Deserialization

    template<typename T>
    void deserialize(const rapidjson::Value& v, const char* n, T& result)
    {
        if(v.HasMember(n))
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                result = v[n].GetString(); // Use GetString() if T is std::string
            }
            else
            {
                result = v[n].Get<T>(); // Use Get<T>() for other types
            }
        }
    }

    template<typename T>
    void deserialize(const rapidjson::Value& v, const char* n, std::vector<T>& result)
    {
        if(v.HasMember(n))
        {
            const auto& a = v[n];
            for(SizeType i = 0; i < a.Size(); i++)
            {
                T element;
                DESERIALIZE(a[i], element);
                result.push_back(element);
            }
        }
    }

    template<typename K, typename T>
    void deserialize(const rapidjson::Value& v, const char* n, std::map<K,T>& result)
    {
        if(v.HasMember(n))
        {
            const auto& a = v[n];
            for(SizeType i = 0; i < a.Size(); i++)
            {
                K key;
                DESERIALIZE(a[i], key);
                T value;
                DESERIALIZE(a[i], value);
                result[key] = value;
            }
        }
    }
}