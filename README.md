# Jen
Image processing, generative photography, cellular automata

## Purpose
"Jen" is short for "Genesis General Generator". She is designed to produce generative art from images, a technique I call "generative photography". Please see my [Medium article on generative photography](https://medium.com/@joyhughes/generative-photography-1318e25bbec1), which shows the results of earlier implementations of Jen, rendered by hand and by computer.

Jen defines a *generative space* - a set of rules that can produce an arbitrary number of randomly generated artworks. At the moment, output types include static images and simple animation loops. Ideally, all the results from each generative space will be artistically interesting and express a coherent visual aesthethic. It can also be used to create singular artwork that is deterministic - i.e. does not employ randomness.

## Folders
The code for Jen is stored in three folders in the repo, organized by language. 

### docker
The easiest way to get started for developers is generally to build and
use the developer docker image available with:

```
./build-docker-image
```

Then enter the container with:
```
./rund
```
The container has all necessary dependencies to build, run, and test the
entire project including C++ drawing engine and web browser frontend GUI.

### Windows-specific steps

1. Install WSL by
  a. entering PowerShell by typing "powershell" in the search box and running it.
  b. witin PowerShell type:
  ```
  wsl --install
  ```

2. Install Docker Desktop https://www.docker.com/products/docker-desktop/
3. Sign in to docker desktop.
4. Enable "Host Networking" checkbox in Docker Desktop settings.
5. Follow normal Linux instructions within your WSL shell to get into docker. Then inside the docker container everything is normal. Use `./rund` to start it within your WSL.

### Jen-C (deprecated)
All the original C language implementations for each iteration of Jen are here.

### Lux
C++ implementation of the 2.5D compositing renderer and scene manager Lux Vitae. The renderer loads a scene file written in JSON, and can render a single image, single-frame animation, or real-time animation. Lux can be run on the command line or through a React web interface called the Joyographic web app. See the [Lux REAMDE](https://github.com/joyhughes/Jen/blob/Update-documentation/Lux/README.md) for more details.

### Niffer (deprecated)
Original Javascript, HTML, CSS front end for Lux. Generates functions, scene files, and generative spaces.

## Development History of Jen

### Life - Cellular Automata
August 2021 <br>
<img src="https://pbs.twimg.com/media/FRXuntLVgAAQKmc?format=jpg&name=small" height = 200> <br>
This work is based on my paper *Cellular Automata for Imaging, art and Video*, in the book [*New Constructions in Cellular Automata*](https://global.oup.com/academic/product/new-constructions-in-cellular-automata-9780195137187?cc=us&lang=en&#), published by the Santa Fe Institute.
A cellular automaton operates on a grid of pixels, each affected by its neighbors by a set of rules. The original and best known example is Conway's "Game of Life", which can create very complex patterns out of very simple rules on a one-bit image. My research extends the concept to 24-bit color images, nonlocal neighborhoods, warping using vector fields, and constrained application of the rules toward a given target image. This allows a rich space of possible aesthetics amenable to use in generative art.

### Still Life - Manual Generative Photography
September 2021 <br>
<img src = "https://miro.medium.com/max/10944/1*V4Oa_Q4cxB3zbP8r2D7JuA.jpeg" height = 200 > <br>
Still Life is my first implementation of a generative space. Each scene is composed of a vase, items within the vase arranged at specified angles, and items arranged outside the vase within a 2D rectangle. Each vase and item ("element") is selected at random from a set of probabilities that are themselves randomly determined. The position of each item is also randomly determined. The scene is then assembled by hand and photographed. Lux can be used to generate similar scenes digitally using photographic composition.

### Generative Genesis - Manual Environmental Art
September 2021 <br>
<img src = "https://miro.medium.com/max/1400/1*d6pv0GgJkzDXMIc54xgG2Q.jpeg" width = 200 > <br>
GG generates clusters of similar elements that vary in position, orientation, size, and color. They can be arranged in circles, grids, or along pathways determined by a vector field. The scene is then constructed by hand, typically in an outdoor setting, often with a scenic backdrop. Lux can be used to generate similar scenes digitally using photographic composition.

### Project Newton - Digitally Composited Light Painting
October 2021 <br>
<img src = "https://miro.medium.com/max/1400/1*CkdWseaXWtkjHBLZ8mpq-A.jpeg" height = 200 > <br>
By subtracting one image from another, Project Newton captures changes in lighting, either positive or negative. The resulting difference images can be composited with changes in translation, rotation, and color. By combining many difference images with varying lighting, interesting results may be obtained. Project Newton was the transition between the earlier manual construction and the fully automated system Lux Vitae.

### Lux Vitae - Scripted Compositing and Animation (C version)
December 2021 <br>
<img src = "https://pbs.twimg.com/media/FRX06RAVIAAXjZ5?format=jpg&name=small" width = 200 > <br>
With a Latin name meaning "Living Light", Lux Vitae became the main renderer for Jen, encompassing the features of all previous projects and providing a unified toolkit going forward. Each element is "splatted" into the scene and can be masked, rotated, scaled, colored, or modified using effects such as Life, Warp, Melt, Hyperspace, or Kaleido. A script file specified the composition of the scene, and if desired a simple animation loop. Clusters of elements can spawn subclusters, a mathematical construct known as an L-system.

### Warp, Melt, Hyperspace, and Kaleido - Generalized Functions
February 2022 <br>
<img src = "https://pbs.twimg.com/media/FRb0YLTVIAAEkWv?format=jpg&name=small" width = 200 > <br>
These simple programs explore visual effects and short animation loops based on a single image. Rather than using hardcoded functions, a function that defines each effect is stored in a file that describes how the image is to be modified. For each pixel, Warp chooses a pixel in the input image based on the function provided. Melt calculates a vector field based on the function, then applies it repeatedly, creating a flowy, melty effect. Hyperspace works like melt, but overlays each image on top of the previous one (slightly faded) creating trails reminiscent of the Millennium Falcon entering hyperspace. Kaleido is a set of functions used by Warp to simulate a kaleidoscope. Each of these effects, along with Life, will be rolled into Lux Vitae to modify elements within a scene.

### Lux Vitae - C++ Version
2022 - 2023
Migration to C++ happened over a number of months. Lux C++ version features a JSON scene file and an more efficient method of composing functions over whole images. Everything is represented as an effect, including rendering elements and clusters, running cellular automata, and image level effects such as warp. See the [README for Lux](https://github.com/joyhughes/Jen/blob/Update-documentation/Lux/README.md) for more details.

### Curly - L-systems
2023
<br><img src = "https://github.com/joyhughes/joyhughes.github.io/blob/main/curly_crop.jpeg" height = 200><br>
Clusters of elements can branch into other clusters. These tree-like structures are known as L-systems.

### Joyographic React app
2024

Lux Vitae ("Living Light" in Latin) is a compositing 2.5D renderer, effects engine, and scene manager written in C++20. It reads a JSON scene file and builds a representation of the scene in the scene object.

Lux can run standalone in command line mode, in which case it saves one or more .jpg files of the result, or embedded in a web browser via Emscripten as WebAssembly, in which case the result is displayed in the browser window, animating in real time.

It's the intent of a full version of Lux to be able to output still images, real-time and single-frame animation, and collections of generative art which could be used for applications such as NFTs.

See the [README for Lux](https://github.com/joyhughes/Jen/blob/Update-documentation/Lux/README.md) for more details.


