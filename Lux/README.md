# Purpose

Fun!

I want to recapture the sense of play and creativity of the early days of the Internet. I want to build a community of artists and developers to create and enjoy pieces of interactive art made from images that anyone can easily use.

# Description

<p align="center">
  <a href="https://www.youtube.com/watch?v=vN1ApdESIrc">
  <img src="https://i.imgur.com/IL1uUOg.jpeg" alt="YouTube - Joyographic React App" width="560" height="310">
</a>
</p>

Lux Vitae ("Living Light" in Latin) is a compositing 2.5D renderer, effects engine, and scene manager written in C++20. It reads a JSON scene file and builds a representation of the scene in the scene object. 

Lux can run standalone in command line mode, in which case it saves one or more .jpg files of the result, or embedded in a web browser via Emscripten as WebAssembly, in which case the result is displayed in the browser window, animating in real time.

It's the intent of a full version of Lux to be able to output still images, real-time and single-frame animation, and collections of generative art which could be used for applications such as NFTs.

For an overview of the development history of the project see the [Jen README](https://github.com/joyhughes/Jen/blob/Update-documentation/README.md#development-history-of-jen)

# Emscripten

[Emscripten](https://emscripten.org/) embeds compiled C or C++ code into the browser using WebAssembly, which as a general rule is not human-readable. A program compiled into WebAssembly cannot access the web or the user's disk, but has a virtual filesystem. Preloaded scene files and images can be stored in the generated file lux.js 

Via Emscripten, C++ and JavaScript can have access to shared memory space. This allows the ImagePortCanvas to be rendered in 1-2 milliseconds once the image is rendered.

## API
<br><img src="https://github.com/joyhughes/joyhughes.github.io/blob/main/request_response.png" height = 400><br>
In order to display an image or load an external file, the C++ program must communicate with JavaScript. Lux operates as a virtual server within the browser using a request-response protocol. All communication within the Joyographic app is initiated by the UI module, and information such as JSON and images are returned by Lux to the UI. API functions are defined in [lux_web.cpp](https://github.com/joyhughes/Jen/blob/main/Lux/src/lux_web.cpp) and are listed at the bottom of the file. Emscripten [embind](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html?highlight=embind) functionality is used for API calls.

In Javascript, the WebAssembly is represented by the Module object. Calls are preceded by window.Module. and then the function name. Example: `window.Module.set_slider_value(json.name, newValue);` sets the value of a slider with a given name in the scene to a new value.

# Joyographic React App

The Joyographic app allows a user to interact with the scene using various controls. Here is an screen capture of a cellular automaton running within the app:
<br><img src="https://joyhughes.github.io/app_example.png" height = 400 align = center><br>
The area containing the image is called the image port and the controls to the right are called the control panel. In a window with portrait aspect ratio (such as a phone) the control panel is displayed below the image port. The image port contains an html canvas which displays the image.

The image port accesses the pixels of the image via Emscripten's [memory view](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html?highlight=memory%20view#memory-views) functionality, which exposes memory used by the C++ program to JavaScript.

In the top right is the media controller. This allows the user to restart, advance one frame, or pause the animation. More controls may be added to the media controller if needed - for instance saving an image, animation, or scene file, or randomly generating a scene. 

Below are widgets - each is contained within a widget container. The widgets displayed are defined within the scene file, grouped together in widget groups. In [ControlPanel.js](https://github.com/joyhughes/Jen/blob/main/Lux/lux_react/src/components/ControlPanel.js#L50), the function `get_panel_JSON()` is called via Embind, which gets a list of widget groups to be displayed. In [WidgetGroup.js](https://github.com/joyhughes/Jen/blob/main/Lux/lux_react/src/components/WidgetGroup.js#L25) the function `get_widget_JSON()` gets a specific description of the widget and its state.

When the user interacts with a widget, its function component will call the appropriate function in the API to modify the scene with its new state.

# Components of Lux C++

## Function objects and harnesses 
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

## Effect
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

# Building Lux

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

# Contributor Guide

There are two main ways to contribute to Lux - helping build the user interface in the React app, adding functions, elements, or cellular automata rules to the C++ portion. Contributors work with the mentors to choose which [issues](https://github.com/joyhughes/Jen/issues) to work on. All pull requests must be approved by a project admin.

## Contributing to the user interface (JavaScript)

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

## Alpha release - early Summer 2024

Once the React UI is stable and a few interesting scenes are built a soft alpha release will be made, allowing users to test the software and provide feedback. A list of issues that must be closed for the release is provided in this [milestone](https://github.com/joyhughes/Jen/milestone/1). The app will be deployed on GitHub.

## Beta release - date TBD

For a beta release, users should be able to register. A wider set of scenes should be available, and users should be able to load and save images, scene files, and animations. The app will be deployed on a dedicated server, which will include a website for sharing scenes, images, and video.

# License

Lux is licensed under the MIT license

Copyright (c) 2024 Joy Hughes

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
