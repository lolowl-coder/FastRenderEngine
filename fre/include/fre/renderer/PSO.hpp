#pragma once

#include "fre/core/Format.hpp"
#include "fre/core/Hash.hpp"
#include "fre/renderer/IShader.hpp"
#include "fre/renderer/SampleCount.hpp"
#include "fre/renderer/VertexLayout.hpp"

#include <vector>

namespace fre
{
    enum class PolygonMode
    {
        Fill,
        Line,
        Point
    };

    enum class CullMode
    {
        None,
        Front,
        Back
    };

    enum class FrontFace
    {
        CounterClockwise,
        Clockwise
    };

    enum class CompareOp
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class PrimitiveTopology
    {
        TriangleList,
        TriangleStrip,
        LineList
    };

    struct RenderTargetState
    {
        std::vector<Format> colorFormats;

        Format depthFormat = Format::Undefined;
    };

    struct RasterState
    {
        PolygonMode polygonMode = PolygonMode::Fill;

        CullMode cullMode = CullMode::Back;

        FrontFace frontFace = FrontFace::CounterClockwise;

        bool depthClamp = false;
    };

    struct BlendAttachmentState
    {
        bool enable = false;
    };

    struct BlendState
    {
        std::vector<BlendAttachmentState> attachments;
    };

    struct DepthStencilState
    {
        bool depthTest = false;
        bool depthWrite = false;

        CompareOp compareOp = CompareOp::Less;
    };

    struct MultisampleState
    {
        SampleCount samplesCount = SampleCount::Sample1;
    };

    struct GraphicsPipelineDesc
    {
        IShader* shader = nullptr;

        RenderTargetState renderTargets;

        RasterState raster;
        DepthStencilState depthStencil;
        BlendState blend;

        PrimitiveTopology topology = PrimitiveTopology::TriangleList;

        VertexLayout vertexLayout;

        MultisampleState multisampleState;

        bool depthTest = false;
    };
}

namespace std
{
    template<>
    struct hash<fre::RenderTargetState>
    {
        size_t operator()(const fre::RenderTargetState& v) const
        {
            size_t seed = 0;

            fre::hashRange(seed, v.colorFormats);
            fre::hashCombine(seed, v.depthFormat);

            return seed;
        }
    };

    template<>
    struct hash<fre::BlendAttachmentState>
    {
        size_t operator()(const fre::BlendAttachmentState& v) const
        {
            size_t seed = 0;

            fre::hashCombine(seed, v.enable);

            return seed;
        }
    };

    template<>
    struct hash<fre::BlendState>
    {
        size_t operator()(const fre::BlendState& v) const
        {
            size_t seed = 0;

            fre::hashRange(seed, v.attachments);

            return seed;
        }
    };

    template<>
    struct hash<fre::RasterState>
    {
        size_t operator()(const fre::RasterState& v) const
        {
            size_t seed = 0;

            fre::hashCombine(seed, v.cullMode);
            fre::hashCombine(seed, v.depthClamp);
            fre::hashCombine(seed, v.frontFace);
            fre::hashCombine(seed, v.polygonMode);

            return seed;
        }
    };

    template<>
    struct hash<fre::DepthStencilState>
    {
        size_t operator()(const fre::DepthStencilState& v) const
        {
            size_t seed = 0;

            fre::hashCombine(seed, v.compareOp);
            fre::hashCombine(seed, v.depthTest);
            fre::hashCombine(seed, v.depthWrite);

            return seed;
        }
    };

    template<>
    struct hash<fre::VertexAttribute>
    {
        size_t operator()(const fre::VertexAttribute& v) const
        {
            size_t seed = 0;

            fre::hashCombine(seed, v.format);
            fre::hashCombine(seed, v.location);
            fre::hashCombine(seed, v.offset);

            return seed;
        }
    };

    template<>
    struct hash<fre::VertexBufferBinding>
    {
        size_t operator()(const fre::VertexBufferBinding& v) const
        {
            size_t seed = 0;

            fre::hashCombine(seed, v.binding);
            fre::hashCombine(seed, v.stride);

            return seed;
        }
    };

    template<>
    struct hash<fre::VertexLayout>
    {
        size_t operator()(const fre::VertexLayout& v) const
        {
            size_t seed = 0;

            fre::hashRange(seed, v.attributes);
            fre::hashRange(seed, v.bindings);

            return seed;
        }
    };

    template<>
    struct hash<fre::MultisampleState>
    {
        size_t operator()(const fre::MultisampleState& v) const
        {
            size_t seed = 0;

            fre::hashCombine(seed, v.samplesCount);

            return seed;
        }
    };
}

namespace fre
{
    struct GraphicsPipelineDescHash
    {
        size_t operator()(const GraphicsPipelineDesc& d) const
        {
            size_t h = 0;
            hashCombine(h, std::hash<uint64_t>()((uint64_t)d.shader));
			hashCombine(h, d.renderTargets);
            hashCombine(h, d.raster);
            hashCombine(h, d.depthStencil);
            hashCombine(h, d.blend);
            hashCombine(h, static_cast<uint32_t>(d.topology));
            hashCombine(h, d.vertexLayout);
            hashCombine(h, d.multisampleState);
            hashCombine(h, d.depthTest);

            return h;
        }
    };
}
