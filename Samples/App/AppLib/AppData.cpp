#pragma once

#include "FileSystem/FileSystem.hpp"
#include "Serialization/BaseTypesSerialization.hpp"
#include "Serialization/MathSerialization.hpp"
#include "AppData.hpp"

#include <rapidjson/document.h>
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include <iostream>
#include <fstream>

using namespace fre;
using namespace rapidjson;
using namespace std;
using namespace glm;
    
#define APP_DATA_FILE_NAME "appData.json"

namespace app
{
    AppData& AppData::getInstance()
    {
        // Static local variable to ensure only one instance is created
        static AppData instance;
        return instance;
    }

    void AppData::save()
    {
        Document d;
        d.SetObject();
            
        SERIALIZE(d, mAppTimeSec);
            
        StringBuffer sb;
        PrettyWriter<StringBuffer> writer(sb);
        writer.SetIndent(' ', 2);  // Set indentation with two spaces

        d.Accept(writer);

        FS;
        ofstream ofs(fs.getDocumentsDir() + "/" + APP_DATA_FILE_NAME);
        if (ofs.is_open())
        {
            ofs << sb.GetString();  // Write the formatted JSON to the file
            ofs.close();
        }
        else
        {
            LOG_ERROR("Error: Unable to open file for writing: %s", APP_DATA_FILE_NAME);
        }
    }

    void AppData::load()
    {
        FS;
        std::ifstream ifs(fs.getDocumentsDir() + "/" + APP_DATA_FILE_NAME);
        if (ifs.is_open())
        {
            std::string jsonString((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();

            // Parse JSON string
            Document d;
            d.Parse(jsonString.c_str());

            // Extract options from JSON object
            DESERIALIZE_MEMBER(d, (*this), mAppTimeSec);
        }
        else
        {
            LOG_ERROR("Unable to open file for reading: %s", APP_DATA_FILE_NAME);
        }
    }
}