Water Surface Rendering
=======================
Experimenting with real-time water surface generation and rendering in 3D using C++ and Vulkan.
Trying to keep it simple, stupid.

## Features
TODO: to be implemented

* Models:
    * FFT - Tessendorf,
    * Gerstner's waves
* Rendering:
    * Mesh based
    * Ray-marched

## Dependencies
* C++17
* CMake
* Vulkan SDK
### Libraries
* window abstraction: [GLFW3](https://github.com/glfw/glfw)
* logging library: [SPDLOG](https://github.com/gabime/spdlog)
* math library: [GLM](https://github.com/g-truc/glm)
* gui library: [ImGui](https://github.com/ocornut/imgui/)
* image loading library: [STB](https://github.com/nothings/stb)
* shader tools (e.g. compilation): [Shaderc](https://github.com/google/shaderc)


## How to compile
Tested on Ubuntu 20.04 LTS, in root dir:
```
$ mkdir build && cd build
$ cmake ..
$ make
$ ./water
```

## License

MIT license

