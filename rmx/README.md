This is a random tilemap generator for Unity that will take a set of sample tilemaps and generate new similar maps while satisfying tile adjacency constraints.
The algorithm is a variation on wave-function-collapse, essentially a CSP solver with bitfields and fast intrinsics with a few extra heuristics to increase generation quality.

* rmxLib.cpp - Does the heavy lifting for the performance intensive parts of the generator. Enables efficient use of bitfields and intrinsics not available in C#.
* RemixMap.cs - Unity component representing a map to be generated. Contains helper functions for validation and post-processing for host applications.
* RemixMapGenerator.cs - Unity component representing a 'generator' as a compiled set of sample maps. Uses rmxLib to do the actual map generation.

