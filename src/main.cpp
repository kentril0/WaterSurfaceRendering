/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "WaterSurface.h"


int main(int argc, char** argv)
{
    vkp::Log::Init();
    VKP_LOG_TRACE("Initialized Logging");

    {
        WaterSurface app({ argc, argv });
        app.Run();
    }

    return EXIT_SUCCESS;
}
