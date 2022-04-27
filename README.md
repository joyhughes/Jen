# Jen
Image processing, generative photography, cellular automata

## Purpose
"Jen" is short for "Genesis General Generator". She is designed to produce generative art from images, a technique I call "generative photography". Please see my [Medium article on generative photography](https://medium.com/@joyhughes/generative-photography-1318e25bbec1), which shows the results of earlier implementations of Jen, rendered by hand and by computer.

Jen defines a *generative space* - a set of rules that can produce an arbitrary number of randomly generated artworks. At the moment, output types include static images and simple animation loops. Ideally, all the results from each generative space will be artistically interesting and express a coherent visual aesthethic. It can also be used to create singular artwork that is deterministic - i.e. does not employ randomness.

## Folders
The code for Jen is stored in three folders in the repo, organized by language. 
### Jen-C
All the original C language implementations for each iteration of Jen are here.
### Lux
C++ implementation of the 2.5D compositing renderer Lux Vitae. Work in progress.
### Niffer
Javascript, HTML, CSS front end for Lux. Generates functions, scene files, and generative spaces.

## History of Jen
### Life - Cellular Automata
August 2021
This work is based on my paper *Cellular Automata for Imaging, art and Video*, in the book [*New Constructions in Cellular Automata*](https://global.oup.com/academic/product/new-constructions-in-cellular-automata-9780195137187?cc=us&lang=en&#), published by the Santa Fe Institute.
A cellular automaton operates on a grid of pixels, each affected by its neighbors by a set of rules. The original and best known example is Conway's "Game of Life", which can create very complex patterns out of very simple rules on a one-bit image. My research extends the concept to 24-bit color images, nonlocal neighborhoods, warping using vector fields, and constrained application of the rules toward a given target image. This allows a rich space of possible aesthetics amenable to use in generative art.
### Still Life - Manual Generative Photography
September 2021
![Still Life image](https://miro.medium.com/max/10944/1*V4Oa_Q4cxB3zbP8r2D7JuA.jpeg)
Still Life is my first implementation of a generative space. Each scene is composed of a vase, items within the vase arranged at specified angles, and items arranged outside the vase within a 2D rectangle. Each vase and item ("element") is selected at random from a set of probabilities that are themselves randomly determined. The position of each item is also randomly determined. The scene is then assembled by hand and photographed.
### Flow - Manual Environmental Art
September 2021
Flow generates clusters of similar elements that vary in position, orientation, size, and color. They can be arranged in circles, grids,
