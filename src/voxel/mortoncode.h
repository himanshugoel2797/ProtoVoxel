#pragma once

#include <stdint.h>

namespace ProtoVoxel::Voxel
{
    struct b2m_table
    {
        uint32_t byteToMorton[256];
        uint8_t mortonToByte[1 << (3 * 4)];

        constexpr b2m_table() : byteToMorton(), mortonToByte()
        {
            for (uint32_t i = 0; i < 256; i++)
            {
                uint32_t cur_val = 0;
                for (int j = 0; j < 8; j++)
                    cur_val |= (((i >> j) & 1) << (j * 3));
                byteToMorton[i] = cur_val;
                mortonToByte[cur_val & 0xfff] = (uint8_t)(i & 0x0f);
            }

            for (uint32_t i = 0; i < 1 << (3 * 4); i++)
            {
                auto idx_i = i & 0x9249;
                mortonToByte[i] = mortonToByte[idx_i];
            }
        }
    };
    class MortonCode
    {
    private:
        static constexpr const auto tbl = b2m_table();
        static const uint32_t xMask = 0x49249249;
        static const uint32_t yMask = 0x92492492;
        static const uint32_t zMask = 0x24924924;
        static const uint32_t xyMask = xMask | yMask;
        static const uint32_t xzMask = xMask | zMask;
        static const uint32_t yzMask = yMask | zMask;

    public:
        [[gnu::pure]] static constexpr inline uint32_t Encode(const uint8_t x, const uint8_t y, const uint8_t z)
        {
            auto m_x = tbl.byteToMorton[x];
            auto m_y = tbl.byteToMorton[y];
            auto m_z = tbl.byteToMorton[z];
            return m_x | (m_y << 1) | (m_z << 2);
        }

        [[gnu::pure]] static constexpr inline uint8_t DecodeX(const uint32_t val)
        {
            auto v_x = val;
            return (tbl.mortonToByte[(v_x >> 12)] << 4) | tbl.mortonToByte[v_x & 0xfff];
        }

        [[gnu::pure]] static constexpr inline uint8_t DecodeY(const uint32_t val)
        {
            auto v_y = (val >> 1);
            return (tbl.mortonToByte[(v_y >> 12)] << 4) | tbl.mortonToByte[v_y & 0xfff];
        }

        [[gnu::pure]] static constexpr inline uint8_t DecodeZ(const uint32_t val)
        {
            auto v_z = (val >> 2);
            return (tbl.mortonToByte[(v_z >> 12)] << 4) | tbl.mortonToByte[v_z & 0xfff];
        }

        [[gnu::pure]] static constexpr inline uint32_t Add(const uint32_t a, const uint32_t b)
        {
            auto xS = (a & yzMask) + (b & xMask);
            auto yS = (a & xzMask) + (b & yMask);
            auto zS = (a & xyMask) + (b & zMask);
            return (xS & xMask) | (yS & yMask) | (zS & zMask);
        }

        [[gnu::pure]] static constexpr inline uint32_t Sub(const uint32_t a, const uint32_t b)
        {
            auto xS = (a & xMask) - (b & xMask);
            auto yS = (a & yMask) - (b & yMask);
            auto zS = (a & zMask) - (b & zMask);
            return (xS & xMask) | (yS & yMask) | (zS & zMask);
        }

        static constexpr const uint32_t OneX = noexcept(MortonCode::Encode(1, 0, 0));
        static constexpr const uint32_t OneY = noexcept(MortonCode::Encode(0, 1, 0));
        static constexpr const uint32_t OneZ = noexcept(MortonCode::Encode(0, 0, 1));
    };
} // namespace ProtoVoxel::Voxel