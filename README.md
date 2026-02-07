# Bus 3D Simulator Graphics

## Project for Computer Graphics subject on Faculty of Technical Sciences

## Student: Veselin Roganovic SV 36/2022

## Short description

This is a 3D bus simulator made in OpenGL. It is modeled with the same rules as the previous 2D project.
Camera is placed at the driver's seat and looks at the scene from a perspective.
Mouse controls the camera movement. Camera can't move more than 180 degrees in any direction.
In front of the driver's seat is a wheel that occasionally moves left and right. 
Next to it on the right side is a control panel, that is actually a 2D project that was already implemented.
Above all of that is the windshield, a slightly darkened transparent block (through which you can see a tree).
Doors are visible on the right side of the camera. It is important to add/remove an appropriate number of passengers each stop.
There is also a light source that illuminates the scene.
On top of all this, the bus is jogging and the driver smokes a cigar every few seconds.

## How to run

I used CLion on my MacBook to run all this. 
First you need to install GLEW, GLFW3, GLM and FreeType using brew (brew install glew glfw3 glm freetype).
Then you need to build and run project (it will load executable files and dependencies in CMakeLists.txt).

It is probably possible to run it on Windows too, but you will need to install GLEW, GLFW3, GLM and FreeType,
and change paths to them in CMakeLists.txt.