### Overview
This guide describes how to build TbaMUD in the Visual Studio through the new experimental CMake environment.

### Prerequisites
* [Visual Studio 2022+](https://visualstudio.microsoft.com/ru/vs/)
* [CMake 3.27+](https://cmake.org/)

### Build Steps
1. Goto the folder `src` and copy `conf.h.win` to `conf.h`.

2. Goto the folder `build` and execute `create_solution.bat`.

3. Open `build/circle.sln` in Visual Studio.

4. Compile and run.