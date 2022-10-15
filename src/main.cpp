
#include "pch.h"
//#include "WaterSurface.h"
#include "core/Application.h"


int main(int argc, char** argv)
{
    vkp::Log::Init();
    VKP_LOG_TRACE("Initialized Logging");

    vkp::Application app("Water Surface Rendering", { argc, argv });
    app.Run();

    /*
    {
        WaterSurface app("Water Surface Rendering", { argc, argv });

        app.Init();
            app.Run();
        app.Cleanup();
    }
    */

    return EXIT_SUCCESS;
}
