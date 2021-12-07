# ray

This is a simple ray tracer that takes a .scn scene file as input and produces an uncompressed .tga image.

## Build instructions - Cori

Load the correct environment modules:

% module swap PrgEnv-intel PrgEnv-gnu
% module load cmake

After downloading, cd into the main source directory, then:

% mkdir build
% cd build
% cmake ../
% make
% cd ..

## Running the programs

All programs expect to be run from the main source directory. There are three programs included: ray, imagewriter, and scenewriter.

### ray

Performs ray tracing on a .scn scene file and outputs a .tga image.

Options:

	-s, --scene
		Specifies the location of the .scn file to use as input.
		Default: -s data/scene.scn
	-o, --output
		Specifies the location of the .tga file into which to write the output.
		Default: -o output/render.tga
	-r, --resolution
		Specifies the vertical resolution of the output image.
		The horizontal resolution is calculated from the aspect ratio of the
		surface specified in the input scene.
		Default: -r 512
	-p, --samples
		Specifies the number of samples into which to divide each pixel
		horizontally and vertically.
		The total samples per pixel will be the square of this number.
		Default: -p 16
	-b, --bounces
		Specifies the maximum number of bounces per ray.
		Default: -b 4
	-ns, --no-spatial-partition
		Boolean flag that, if present, turns off the use of the spatial
		partition and reverts to a flat list of all scene objects.
	-ol, --objects-per-leaf
		Specifies the maximum number of objects per leaf in the spatial
		partition.
		Default: -ol 8
	-ld, --leaf-depth\n"
		Specifies the maximum depth of a leaf in the spatial partition.
		Default: -ld 32
	-di, --distance
		Specifies the maximum distance from the camera to use as bounds for the
		spatial partition.
		Default: -di FLT_MAX
	-d, --debug
		Boolean flag that, if present, turns on printing of debug information.

Example usage:

% build/ray -s data/my_scene.scn -o output/my_render.tga -p 32 -b 8

Options can be displayed by using

% build/ray -h

### imagewriter

Writes some texture files in uncompressed .tga format into the data directory. Should not need to be run. Inputs and outputs are hardcoded, thus this will need to be recompiled in order to change anything. Image data can be edited by modifying imagedata.h. Usage:

% build/imagewriter

### scenewriter

Writes a randomly generated scene to a specified .scn file. The number of objects and the side length of the scene bounds must be specified. Usage:

% build/scenewriter \<destfile.scn\> \<num objects\> \<scene size\> [\<rng seed\>]

E.g.:

% build/scenewriter data/rand_scene.scn 200 30 1123581321
