#pragma once

#include "FileSystem/FileSystem.hpp"
#include "Serialization/BaseTypesSerialization.hpp"
#include "Serialization/MathSerialization.hpp"
#include "Options.hpp"

#include <rapidjson/document.h>
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include <iostream>
#include <fstream>

using namespace rapidjson;
using namespace std;
using namespace glm;
    
#define OPTIONS_FILE_NAME "options.json"

namespace fre
{
    void serialize(Value& v, const char* n, const Options::Common& m, Document& d)
    {
        Value result(kObjectType);

        SERIALIZE_MEMBER(result, m, mDiffuseIntensity);
        SERIALIZE_MEMBER(result, m, mSpecularIntensity);

        v.AddMember(StringRef(n), result, d.GetAllocator());
    }

    void deserialize(const Value& v, const char* n, Options::Common& result)
    {
        if(v.HasMember(n))
        {
            const Value& tmp = v[n];

            DESERIALIZE_MEMBER(tmp, result, mDiffuseIntensity);
            DESERIALIZE_MEMBER(tmp, result, mSpecularIntensity);
        }
    }

    Options::Options()
    {
    }

    Options& Options::getInstance()
    {
        // Static local variable to ensure only one instance is created
        static Options instance;
        return instance;
    }

    void Options::save()
    {
        Document d;
        d.SetObject();
            
        SERIALIZE(d, mCommon);
        SERIALIZE(d, mEnableStat);
            
        StringBuffer sb;
        PrettyWriter<StringBuffer> writer(sb);
        writer.SetIndent(' ', 2);  // Set indentation with two spaces

        d.Accept(writer);

        FS;
        ofstream ofs(fs.getDocumentsDir() + "/" + OPTIONS_FILE_NAME);
        if (ofs.is_open())
        {
            ofs << sb.GetString();  // Write the formatted JSON to the file
            ofs.close();
        }
        else
        {
            LOG_ERROR("Error: Unable to open file for writing: %s", OPTIONS_FILE_NAME);
        }
    }

    void Options::load()
    {
        FS;
        std::ifstream ifs(fs.getDocumentsDir() + "/" + OPTIONS_FILE_NAME);
        if (ifs.is_open())
        {
            std::string jsonString((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();

            // Parse JSON string
            Document d;
            d.Parse(jsonString.c_str());

            // Extract options from JSON object
            DESERIALIZE_MEMBER(d, (*this), mCommon);
            DESERIALIZE_MEMBER(d, (*this), mEnableStat);
        } else
        {
            LOG_ERROR("Unable to open file for reading: %s", OPTIONS_FILE_NAME);
        }
    }
}