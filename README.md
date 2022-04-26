# Jen
Image processing, generative photography, cellular automata

# Purpose
"Jen" is short for "Genesis General Generator". She is designed to produce generative art from images, a technique I call "generative photography". Please see my [Medium article on generative photography](https://medium.com/@joyhughes/generative-photography-1318e25bbec1), which shows the results of earlier implementations of Jen, rendered by hand and by computer.

Jen defines a *generative space* - a set of rules that can produce an arbitrary number of randomly generated artworks. At the moment, output types include static images and simple animation loops. Ideally, all the results from each generative space will be artistically interesting and express a coherent visual aesthethic. It can also be used to create singular artwork that is deterministic - i.e. does not employ randomness.

# Iterations of Jen
## Life - Cellular Automata
August 2021
This work is based on my paper *Cellular Automata for Imaging, art and Video*, in the book [New Constructions in Cellular Automata](https://global.oup.com/academic/product/new-constructions-in-cellular-automata-9780195137187?cc=us&lang=en&#), published by the Santa Fe Institute.
A cellular automaton operates on a grid of pixels, each affected by its neighbors by a set of rules. The original and best known example is Conway's "Game of Life", which can create very complex patterns out of very simple rules on a one-bit image. My research extends the concept to 24-bit color images, nonlocal neighborhoods, warping using vector fields, and constrained application of the rules toward a given target image. This creates a rich space of possible aesthetics amenable to use in generative art.

