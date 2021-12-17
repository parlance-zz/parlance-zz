This project was an experimental software GPU renderer for point clouds with post-processing shaders, written in C++, CUDA, and Nvidia CG. This project contains work that is not my own, in some of the CG shaders and CG host code.
The included ObjToVTS tool was developed to convert 3D model data in OBJ format with hundreds of millions of polygons to quantized and packed point clouds in the format expected by the engine.

There is a screenshot demonstrating the kind of scene the software was designed for in the root directory of this project. At the time of development there wasn't enough memory or processing power on even high-end GPUs to make this a practical general rendering approach, but it is worth noting that some of the ideas are very similar to what would become Nanite in Unreal Engine 5.

