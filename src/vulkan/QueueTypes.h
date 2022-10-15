/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_QUEUE_TYPES_H_
#define WATER_SURFACE_RENDERING_VULKAN_QUEUE_TYPES_H_

#include <optional>
#include <array>


namespace vkp
{

    /**
     * @brief Queue Family type
     *  Used as an index into instance of "QFamilyIndices"
     */
    enum class QFamily
    {
        Graphics = 0,
        Compute,
        Transfer,
        Present, 

        Total
    };

    /// @return Number of queue families
    constexpr inline std::size_t TotalQueues() { 
        return static_cast<std::size_t>(QFamily::Total);
    }

    constexpr inline uint32_t QFamilyToUInt(QFamily i) {
        return static_cast<uint32_t>(i);
    }

    constexpr inline size_t QFamilyToSize(QFamily i) {
        return static_cast<size_t>(i);
    }

    /**
     * @brief Index of a certain queue of a certain family type, might not be
     *  present on some devices, therefore is optional
     */
    using QFamilyIndex = std::optional<uint32_t>;

    /**
     * @brief An array of indices of queues of predefined queue families
     */
    struct QFamilyIndices
    {
        std::array<QFamilyIndex, TotalQueues()> indices;

        QFamilyIndex& operator[](QFamily i) { return indices[QFamilyToUInt(i)]; }

        const QFamilyIndex& operator[](QFamily i) const {
            return indices[QFamilyToUInt(i)];
        }

        uint32_t Graphics() const { 
            return indices[QFamilyToUInt(QFamily::Graphics)].value();
        }
        uint32_t Compute() const { 
            return indices[QFamilyToUInt(QFamily::Compute)].value();
        }
        uint32_t Transfer() const { 
            return indices[QFamilyToUInt(QFamily::Transfer)].value();
        }
        uint32_t Present() const { 
            return indices[QFamilyToUInt(QFamily::Present)].value();
        }

        QFamilyIndex& FlagBits2Index(VkQueueFlagBits b);
        const QFamilyIndex& FlagBits2Index(VkQueueFlagBits b) const;
    };

    /**
     * @brief Abstracts Array of queue handles to each queue type
     */
    struct Queues
    {
        union   ///< Anonymous union to allow looping through the members
        {
            //@pre In order of vkp::QFamily enum
            struct
            {
                VkQueue graphics;
                VkQueue compute;
                VkQueue present;
                VkQueue transfer;
            };
            std::array<VkQueue, TotalQueues()> arr;
        };

        VkQueue& operator[](QFamily i) { return arr[QFamilyToUInt(i)]; }
        const VkQueue& operator[](QFamily i) const {
            return arr[QFamilyToUInt(i)];
        }

        Queues() : arr{ VK_NULL_HANDLE } {}
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_QUEUE_TYPES_H_
