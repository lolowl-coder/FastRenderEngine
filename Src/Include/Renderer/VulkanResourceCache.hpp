#pragma once

namespace fre
{
    template <typename Key, typename Resource>
    class VulkanResourceCache
    {
    public:
        using Index = uint32_t;

        // Add or retrieve resource by key
        Index findOrCreate(const Key& key, std::function<Resource(const Key& key)> createFunc)
        {
            auto it = keyToIndex.find(key);
            if(it != keyToIndex.end())
            {
                return it->second;
            }

            Index newIndex = static_cast<Index>(resources.size());
            resources.push_back(createFunc(key));
            keyToIndex[key] = newIndex;
            return newIndex;
        }

        // Access by index
        Resource& getByIndex(Index index)
        {
            assert(index < resources.size());
            return resources[index];
        }

        const Resource& getByIndex(Index index) const
        {
            assert(index < resources.size());
            return resources[index];
        }

        size_t size() const { return resources.size(); }

    private:
        std::vector<Resource> resources;
        std::unordered_map<Key, Index> keyToIndex;
    };
}