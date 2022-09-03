# SkyRendering

## Requirements

* Windows 10
* Visual Studio 2019

## Build

```
git clone git@github.com:c52e/SkyRendering.git --single-branch
```
Open SkyRendering.sln.

Set SkyRendering as startup project.

Build and run.

## Screenshots (Real-time)

![screenshot1](https://c52e.github.io/SkyRendering/data/screenshot4.jpg)
![screenshot2](https://c52e.github.io/SkyRendering/data/screenshot2.jpg)
![screenshot3](https://c52e.github.io/SkyRendering/data/screenshot3.jpg)
[video](https://c52e.github.io/SkyRendering/data/godray.mp4) (higher quality than gif)  
![godray](https://github.com/c52e/SkyRendering/blob/gh-pages/data/godray.gif)

## Path Tracing Results

![screenshot4](https://c52e.github.io/SkyRendering/data/wdas1.jpg)
![screenshot5](https://c52e.github.io/SkyRendering/data/wdas2.jpg)

To make voxel material available, you need to install OpenVDB and apply user-wide integration:

```
vcpkg install openvdb:x64-windows
vcpkg integrate install
```

Download [vcpkg](https://github.com/microsoft/vcpkg)

Download high-resolution [Disney cloud](https://disneyanimation.com/resources/clouds/)