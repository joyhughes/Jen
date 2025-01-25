# Jen
Image Processing, Generative Photography, and Cellular Automata

<!-- 

<p align="center">
 <img src="https://pbs.twimg.com/media/FRX06RAVIAAXjZ5?format=jpg&name=small" width="400">
</p> 

-->

## Purpose

"Jen" is short for "Genesis General Generator". She is designed to produce generative art from images, a technique I call "generative photography". Please see the [Medium article on generative photography](https://medium.com/@joyhughes/generative-photography-1318e25bbec1), which showcases the results of earlier implementations of Jen, rendered both by hand and by computer.

Jen defines a *generative space* - a set of rules that can produce an arbitrary number of randomly generated artworks. At the moment, output types include static images and simple animation loops. Ideally, all the results from each generative space will be artistically interesting and express a coherent visual aesthetic. It can also be used to create singular artwork that is deterministic - i.e., does not employ randomness.

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

# Jen-C (deprecated)

All the original C language implementations for each iteration of Jen are here.

# Lux

C++ implementation of the 2.5D compositing renderer and scene manager Lux Vitae. The renderer loads a scene file written in JSON and can render a single image, single-frame animation, or real-time animation. Lux can be run on the command line or through a React web interface called the Joyographic web app. 

## Purpose

Fun!

I want to recapture the sense of play and creativity of the early days of the Internet. I want to build a community of artists and developers to create and enjoy pieces of interactive art made from images that anyone can easily use.

## Description

<p align="center">
  <a href="https://www.youtube.com/watch?v=vN1ApdESIrc">
  <img src="https://github.com/joyhughes/joyhughes.github.io/blob/main/preview.jpg" alt="YouTube - Joyographic React App" width="560" height="310">
</a>
</p>

Lux Vitae ("Living Light" in Latin) is a compositing 2.5D renderer, effects engine, and scene manager written in C++20. It reads a JSON scene file and builds a representation of the scene in the scene object. 

Lux can run standalone in command line mode, in which case it saves one or more .jpg files of the result, or embedded in a web browser via Emscripten as WebAssembly, in which case the result is displayed in the browser window, animating in real time.

It's the intent of a full version of Lux to be able to output still images, real-time and single-frame animation, and collections of generative art which could be used for applications such as NFTs.

For an overview of the development history of the project see the [Jen README](https://github.com/joyhughes/Jen/blob/Update-documentation/README.md#development-history-of-jen)

## Emscripten

[Emscripten](https://emscripten.org/) embeds compiled C or C++ code into the browser using WebAssembly, which as a general rule is not human-readable. A program compiled into WebAssembly cannot access the web or the user's disk, but has a virtual filesystem. Preloaded scene files and images can be stored in the generated file lux.js 

Via Emscripten, C++ and JavaScript can have access to shared memory space. This allows the ImagePortCanvas to be rendered in 1-2 milliseconds once the image is rendered.

## API
<br><img src="https://github.com/joyhughes/joyhughes.github.io/blob/main/request_response.png" height = 400><br>
In order to display an image or load an external file, the C++ program must communicate with JavaScript. Lux operates as a virtual server within the browser using a request-response protocol. All communication within the Joyographic app is initiated by the UI module, and information such as JSON and images are returned by Lux to the UI. API functions are defined in [lux_web.cpp](https://github.com/joyhughes/Jen/blob/main/Lux/src/lux_web.cpp) and are listed at the bottom of the file. Emscripten [embind](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html?highlight=embind) functionality is used for API calls.

In Javascript, the WebAssembly is represented by the Module object. Calls are preceded by window.module. and then the function name. Example: `window.module.set_slider_value(json.name, newValue);` sets the value of a slider with a given name in the scene to a new value.

## Joyographic React App

The Joyographic app allows a user to interact with the scene using various controls. Here is an screen capture of a cellular automaton running within the app:
<br><img src="https://joyhughes.github.io/app_example.png" height = 400 align = center><br>
The area containing the image is called the image port and the controls to the right are called the control panel. In a window with portrait aspect ratio (such as a phone) the control panel is displayed below the image port. The image port contains an html canvas which displays the image.

The image port accesses the pixels of the image via Emscripten's [memory view](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html?highlight=memory%20view#memory-views) functionality, which exposes memory used by the C++ program to JavaScript.

In the top right is the media controller. This allows the user to restart, advance one frame, or pause the animation. More controls may be added to the media controller if needed - for instance saving an image, animation, or scene file, or randomly generating a scene. 

Below are widgets - each is contained within a widget container. The widgets displayed are defined within the scene file, grouped together in widget groups. In [ControlPanel.js](https://github.com/joyhughes/Jen/blob/main/Lux/lux_react/src/components/ControlPanel.js#L50), the function `get_panel_JSON()` is called via Embind, which gets a list of widget groups to be displayed. In [WidgetGroup.js](https://github.com/joyhughes/Jen/blob/main/Lux/lux_react/src/components/WidgetGroup.js#L25) the function `get_widget_JSON()` gets a specific description of the widget and its state.

When the user interacts with a widget, its function component will call the appropriate function in the API to modify the scene with its new state.

## Components of Lux C++

### Function objects and harnesses 
A function object (called a [functor](https://stackoverflow.com/questions/356950/what-are-c-functors-and-their-uses)) contains a function that can be applied to one of a variety of data types. 

A harness includes a value and a list of function objects which can be calculated as needed, and are used as data fields in various types of objects throughout Lux. The declaration of the harness functor in next_element.hpp is shown below. The * operator is overloaded as shorthand to reference the harness's value:

```
template< class U > struct harness {
    std::vector< any_fn< U > > functions;
    U val;

    void operator () ( element_context& context );

    U  operator *  () { return  val; }
    U* operator -> () { return &val; }
    harness< U >& operator = ( const U& u ) { val = u; return *this; }

    void add_function( const any_fn< U >& fn );

    harness();
    harness( const U& val_init );
    harness( const harness& h );
    ~harness();
};
```

### Effect
Effect modify the contents of and image. They can include warps, color changes, cellular automata, or rendering an element or cluster. They are represented by function objects and stored within the scene. Effects are run sequentially within effect lists. Many effects are defined within effect.hpp

### Element
An element consists of any image type mapped to the range -1.0 to 1.0 in x and y and optionally a mask of the same size and type as the image. It has a position, size, and orientation within the scene that can be controlled via harnesses by functions via harnesses including user interface tools. Element is defined within [scene.hpp](https://github.com/joyhughes/Jen/blob/main/Lux/src/scene.hpp#L32)

### Cluster
A cluster is a grouping of elements. A function object called next_element determines the position, orientation, scale, and color of each element, which is generated recursively. Some arrangements for clusters include circles, grids, random arrangements, and along the flow lines of a vector field. Clusters can branch, and the branches can branch, producing L-systems that can resemble natural phenomena. The aurora simulation pictured above is produced by an L-system with three levels. A cluster description is stored as part of a scene file. 

### Scene
A scene contains a complete description of how each image or animation frame is generated, including each element, cluster, function objects, effects, and image buffers within each of which might be used multiple times within the scene. Scenes are loaded from JSON scene files, which can be written by hand or generated by Lux.

## Images

### Implementation
An image is defined as an evenly spaced grid of values of given x and y dimensions. Double buffering is used for some effects - images are stored in buffer pair objects that include the back buffer if needed. Front and back buffers can be swapped after calculations are made. Image code includes functions for each image type including reading and writing from disc, simple imaging operations, sampling, masking, and "splatting" an element onto an image.

### FRGB pixel format
FRGB is a floating point color model. The range between 0.0 and 1.0 corresponds to the range between zero and 255 for each 8-bit color component. Colors can be oversaturated (above 1.0) or negative, expanding the range of possibilites in HDR, masking, and compositing. FRGB does not have an alpha channel - full FRGB masking is used instead. FRGB colors must be converted to standard 24-bit color in order to be displayed.

### UImage 
A UImage is a grid of 32-bit pixels represented in ARGB byte order. It is used for real time calculations and display on the web.

### FImage
FImage is a grid of FRGB values, representing an image in the FRGB color space. It also includes a bounding box in 2D floating point space that defines a coordinate for each pixel. Color values can be negative or greater than 1.0.  Calculations are generally slower but more accurate than UImage.

### Vector Field
VField is a grid of integer or floating point vectors representing a vector field, and a bounding box in 2D floating point Cartesian space. The VField code has a look up function that returns an interpolated vector based on the coordinate provided, and can move ("advect") a coordinate based on this result. There are a number of mathematical functions that can modify a single vector field or combine two fields together. There are also functions that can generate a vector field based on various mathematical formulae.

## Building Lux

##Command line
```
mkdir build
cd build
cmake ..
make
./lux < include arguments here >
make clean
```
##React + Emscripten
```
make
cd lux_react
npm start
^C to exit
```
See the [lux_react README](https://github.com/joyhughes/Jen/blob/main/Lux/lux_react/README.md) for instructions for installing create-react-app (which is scheduled to be replaced by Vite)

## Contributor Guide

There are two main ways to contribute to Lux - helping build the user interface in the React app, adding functions, elements, or cellular automata rules to the C++ portion. Contributors work with the mentors to choose which [issues](https://github.com/joyhughes/Jen/issues) to work on. All pull requests must be approved by a project admin.

### Contributing to the user interface (JavaScript)

Contributors are invited to work on UI design and layout and widgets in the Joyographic app in accordance with the UI design philosophy. You don't need to know C++ or Emscripten to contribute to the UI. 

[Issues labeled with UI](https://github.com/joyhughes/Jen/issues?q=is%3Aissue+is%3Aopen+label%3AUI) are suitable for UI contributors. You will find some issues labeled with both UI and C++ - in this case you might want to collaborate with another contributor or a project admin.

## UI design philosophy

### General audience

No technical knowledge or training should be required to use the Joyographic app. The user should feel safe making changes, and they should be easily reversed if desired.

### A sense of play

The app should be fun to use and require little or no training to get started. Using any of the controls should make interesting changes to the display. Names of controls should be fun rather than technical or mathematical.

### Simple user choices

The interface should display a small number of controls at any given time to avoid overwhelming the user with too many possibilities. Creators can have access to more powerful tools (such as editing scenes) if desired, but these will not be the first thing a casual user sees.

### Control panel as a generic container

The scene file specifies which widgets will be displayed in the control panel. These might change based on user choices - for instance, selecting a cellular automaton rule. The control panel must display manage these widgets, and handle situations where they overflow the display area. Some UI elements such as the media controller and tabs for brush and target image will always be displayed.

### Continuous interaction

The UI should not pause animated display in the image port unless the user presses the pause button. This will allow the app to potentially respond to audio or camera input, or to be used for visual performance, screen recording, or streaming online. 

### Keep image port uncovered

As much as possible, UI elements should not overlap the image port. This will improve the user experience and allow the app to be used for visual performance, screen recording, or streaming online. 

## Contributing a function, effect, or cellular automaton rule (C++)

Lux's system is designed to be extensible with new ways to create all kinds of cool behavior. Every function and effect is defined as a functor, a function object that can contain persistent data. If you'd like to contribute on the C++ side, please contact a mentor and we will give you instructions for integrating your new functionality.

Here is an example of a very simple functor:

```
template< MultipliableByFloat U > struct ratio {
    harness< float > r;
    
    U operator () ( U& u, element_context& context ) { 
        r( context ); 
        return *r * u; 
    }

    ratio( const float& r_init = 1.0f ) : r( r_init ) {}
};
```
It multiples its argument by the float value r, represented in this case by a harness so that its value can be defined by another functor.

# Roadmap

## Alpha release

Once the React UI is stable and a few interesting scenes are built a soft alpha release will be made, allowing users to test the software and provide feedback. A list of issues that must be closed for the release is provided in this [milestone](https://github.com/joyhughes/Jen/milestone/1). The app will be deployed on GitHub.

## Beta release

For a beta release, users should be able to register. A wider set of scenes should be available, and users should be able to load and save images, scene files, and animations. The app will be deployed on a dedicated server, which will include a website for sharing scenes, images, and video.


### Niffer (deprecated)

Original JavaScript, HTML, CSS front end for Lux. Generates functions, scene files, and generative spaces.

## Development History of Jen

### Life - Cellular Automata

**August 2021**

<p align="center">
 <img src="https://pbs.twimg.com/media/FRXuntLVgAAQKmc?format=jpg&name=small" height="200">
</p>

This work is based on the paper *Cellular Automata for Imaging, art and Video*, published in the book [*New Constructions in Cellular Automata*](https://global.oup.com/academic/product/new-constructions-in-cellular-automata-9780195137187?cc=us&lang=en&#) by the Santa Fe Institute.

A cellular automaton operates on a grid of pixels, each affected by its neighbors according to a set of rules. The original and best-known example is Conway's "Game of Life", which can create very complex patterns out of very simple rules on a one-bit image. The research extends the concept to 24-bit color images, nonlocal neighborhoods, warping using vector fields, and constrained application of the rules toward a given target image. This allows a rich space of possible aesthetics amenable to use in generative art.

### Still Life - Manual Generative Photography

**September 2021**

<p align="center">
 <img src="https://miro.medium.com/max/10944/1*V4Oa_Q4cxB3zbP8r2D7JuA.jpeg" height="200">
</p>

Still Life is the first implementation of a generative space. Each scene is composed of a vase, items within the vase arranged at specified angles, and items arranged outside the vase within a 2D rectangle. Each vase and item ("element") is selected at random from a set of probabilities that are themselves randomly determined. The position of each item is also randomly determined. The scene is then assembled by hand and photographed. Lux can be used to generate similar scenes digitally using photographic composition.

### Generative Genesis - Manual Environmental Art

**September 2021**

<p align="center">
 <img src="https://miro.medium.com/max/1400/1*d6pv0GgJkzDXMIc54xgG2Q.jpeg" width="200">
</p>

GG generates clusters of similar elements that vary in position, orientation, size, and color. They can be arranged in circles, grids, or along pathways determined by a vector field. The scene is then constructed by hand, typically in an outdoor setting, often with a scenic backdrop. Lux can be used to generate similar scenes digitally using photographic composition.

### Project Newton - Digitally Composited Light Painting

**October 2021**

<p align="center">
 <img src="https://miro.medium.com/max/1400/1*CkdWseaXWtkjHBLZ8mpq-A.jpeg" height="200">
</p>

By subtracting one image from another, Project Newton captures changes in lighting, either positive or negative. The resulting difference images can be composited with changes in translation, rotation, and color. By combining many difference images with varying lighting, interesting results may be obtained. Project Newton was the transition between the earlier manual construction and the fully automated system Lux Vitae.

### Lux Vitae - Scripted Compositing and Animation (C version)

**December 2021**

<p align="center">
 <img src="https://pbs.twimg.com/media/FRX06RAVIAAXjZ5?format=jpg&name=small" width="200">
</p>

With a Latin name meaning "Living Light", Lux Vitae became the main renderer for Jen, encompassing the features of all previous projects and providing a unified toolkit going forward. Each element is "splatted" into the scene and can be masked, rotated, scaled, colored, or modified using effects such as Life, Warp, Melt, Hyperspace, or Kaleido. A script file specifies the composition of the scene, and if desired, a simple animation loop. Clusters of elements can spawn subclusters, a mathematical construct known as an L-system.

### Warp, Melt, Hyperspace, and Kaleido - Generalized Functions

**February 2022**

<p align="center">
 <img src="https://pbs.twimg.com/media/FRb0YLTVIAAEkWv?format=jpg&name=small" width="200">
</p>

These simple programs explore visual effects and short animation loops based on a single image. Rather than using hardcoded functions, a function that defines each effect is stored in a file that describes how the image is to be modified. For each pixel, Warp chooses a pixel in the input image based on the function provided. Melt calculates a vector field based on the function, then applies it repeatedly, creating a flowy, melty effect. Hyperspace works like melt, but overlays each image on top of the previous one (slightly faded) creating trails reminiscent of the Millennium Falcon entering hyperspace. Kaleido is a set of functions used by Warp to simulate a kaleidoscope. Each of these effects, along with Life, will be rolled into Lux Vitae to modify elements within a scene.

### Lux Vitae - C++ Version

**2022 - 2023**

Migration to C++ happened over several months. The Lux C++ version features a JSON scene file and a more efficient method of composing functions over whole images. Everything is represented as an effect, including rendering elements and clusters, running cellular automata, and image-level effects such as warp. See the [README for Lux](https://github.com/joyhughes/Jen/blob/Update-documentation/Lux/README.md) for more details.

### Curly - L-systems

**2023**

<p align="center">
 <img src="https://github.com/joyhughes/joyhughes.github.io/blob/main/curly_crop.jpeg" height="200">
</p>

Clusters of elements can branch into other clusters. These tree-like structures are known as L-systems.

### Joyographic React App

**2024**

<p align="center">
  <a href="https://www.youtube.com/watch?v=vN1ApdESIrc">
  <img src="https://github.com/joyhughes/joyhughes.github.io/blob/main/preview.jpg" alt="YouTube - Joyographic React App" width="560" height="310">
</a>
</p>

See the [demo of the Joyographic React App](https://www.youtube.com/watch?v=vN1ApdESIrc) using the Lux Vitae renderer.

Lux Vitae ("Living Light" in Latin) is a compositing 2.5D renderer, effects engine, and scene manager written in C++20. It reads a JSON scene file and builds a representation of the scene in the scene object.

Lux can run standalone in command line mode, in which case it saves one or more .jpg files of the result, or embedded in a web browser via Emscripten as WebAssembly, in which case the result is displayed in the browser window, animating in real time.

It's the intent of a full version of Lux to be able to output still images, real-time and single-frame animation, and collections of generative art which could be used for applications such as NFTs.

See the [README for Lux](https://github.com/joyhughes/Jen/blob/Update-documentation/Lux/README.md) for more details.
