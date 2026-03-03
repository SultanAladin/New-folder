// File: Source/Filament/Internal/RenderSubsystem/FilamentLighting.h
#pragma once

/*====================================================================================================================================
                                                   FILAMENTLIGHTING.H
====================================================================================================================================*/
// 🧩 Fullscreen deferred lighting — Cook-Torrance GGX + Lambertian diffuse.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Lighting is a compute dispatch — no dedicated state struct needed.
//    The compute shader reads GBuffer RTs and outputs to the lit color image.
//    All binding is done via the compute descriptor set at dispatch time.
//    See FilamentPipeline for orchestration.
