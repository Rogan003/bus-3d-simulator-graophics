# Bus Route Tracker Graphics

## Project for Computer Graphics subject on Faculty of Technical Sciences
## Student: Veselin Roganovic SV 36/2022

## Short description
This is a 2D graphical application to track a bus route.

The screen shows the path along which the bus travels. It is made up of curved red lines between stops that form an irregular shape. The stops are marked as red circles with a white number inside. There are 10 stops, numbered 0 through 9. The bus is shown with a bus icon and travels the route stopping at the stations in order from 0 to 9. The route is closed, that is, in addition to each pair of neighboring stops, stop 0 is also connected to stop 9.

The bus moves from stop to stop at a constant speed. When it arrives at a stop, it pauses. In any chosen corner of the screen display an icon of doors that are closed while the bus is moving and open for 10 seconds while it is at a stop (during this time the bus icon also remains at the stop).

In another chosen corner of the screen keep the number of passengers on the bus, which starts at 0. While the bus doors are open at a stop, a left mouse click anywhere on the screen adds one passenger (the number increases by 1). A right mouse click anywhere on the screen removes one passenger (the number decreases by 1). The number of passengers is limited to the range 0 to 50.

While the bus doors are open, pressing the K key lets an inspector enter the bus. The passenger count then increases by 1, and in a third chosen corner of the screen an inspector marker is shown (for example, an image of a traffic officer from Monopoly) until the bus reaches the next stop, at which point the inspector leaves and the passenger count decreases by 1. At the stop where the inspector leaves, generate a random value between 0 and (number of passengers − 1) representing how many passengers received a fine (−1 because one of the passengers was the inspector), and add this to the total number of fines collected. This total starts at 0 and should always be displayed next to the current passenger count. It is possible for another inspector to enter at the same stop where the previous inspector just left.

The cursor should look like a yellow diamond-shaped traffic sign (like a BUS STOP sign), with the sign itself positioned in the upper-left of the cursor image and the metal pole drawn diagonally across the image.

## How to run
I used CLion on my MacBook to run all this.
First you need to install GLEW, GLFW3 and FreeType using brew (`brew install glew glfw3 freetype`).
Then you need to build and run project (it will load executable files and dependencies in CMakeLists.txt).

It is probably possible to run it on Windows too, but you will need to install GLEW, GLFW3 and FreeType, and change paths to them in CMakeLists.txt.