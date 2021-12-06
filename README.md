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

All programs expect to be run from the main source directory.

Options:

-s, --scene
> Specifies the location of the .scn file to use as input.
> Default: -s data/scene.scn
-o, --output
> Specifies the location of the .tga file into which to write the output.
> Default: -o output/render.tga
-r, --resolution
> Specifies the vertical resolution of the output image. The horizontal resolution is calculated from the aspect ratio of the surface specified in the input scene.
> Default: -r 512
-p, --samples
> Specifies the number of samples into which to divide each pixel vertically and horizontally. The total samples per pixel will be the square of this number.
> Default: -p 16
-b, --bounces
> Specifies the maximum number of bounces per ray.
> Default: -b 4
-ns, --no-spatial-partition\n");
> Boolean flag that, if present, turns off the use of the spatial partition and reverts to a flat list of all scene objects.
-ol, --objects-per-leaf
> Specifies the maximum number of objects per leaf in the spatial partition.
> Default: -ol 8
-ld, --leaf-depth\n"
> Specifies the maximum depth of a leaf in the spatial partition.
> Default: -ld 32
-di, --distance
> Specifies the maximum distance from the camera to use as bounds for the spatial partition.
> Default: -di FLT_MAX
"-d, --debug\n");
> Boolean flag that, if present, turns on printing of debug information.