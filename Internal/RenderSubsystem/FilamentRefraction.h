// File: Source/Filament/Internal/RenderSubsystem/FilamentRefraction.h
#pragma once

/*====================================================================================================================================
                                                   FILAMENTREFRACTION.H
====================================================================================================================================*/
// 🧩 UV-offset refraction composite — compute shader pass.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Refraction is a compute dispatch — UV offset via normal perturbation.
//    The compute shader reads the lit image + GBuffer normals/depth
//    and outputs the refracted composite image.
//    See FilamentPipeline for orchestration.
