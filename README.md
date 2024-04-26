# Mandelbrot Image Generator

This program generates a series of JPEG images depicting the Mandelbrot set, a famous fractal pattern. The images are created by zooming in on different regions of the Mandelbrot set, providing a visually stunning exploration of its intricate patterns.

## Features

- Generates multiple JPEG images of the Mandelbrot set
- Allows specifying the center point, scale, image dimensions, and maximum iterations for each image
- Utilizes multi-threading for efficient parallel computation of image rows
- Provides command-line options for customizing the image generation process

## Usage

The program is controlled through command-line arguments. To run the program, execute the following command: `./mandelmovie [options]`

The available options are:

- `-c <num_children>`: Number of child processes to spawn for image generation (default: 1)
- `-t <num_threads>`: Number of threads to use for each child process (default: 1)
- `-x <x_coord>`: X-coordinate of the image center point (default: 0)
- `-y <y_coord>`: Y-coordinate of the image center point (default: 0)
- `-m <max_iterations>`: Maximum number of iterations per point (default: 1000)
- `-H <height>`: Height of the image in pixels (default: 1000)
- `-W <width>`: Width of the image in pixels (default: 1000)
- `-h`: Show help information

## Example

To generate 50 images with a maximum of 10 concurrent child processes using 4 threads per child, centered at (-0.5, -0.5) with a scale of 0.2, run the following command: `./mandelmovie -c 10 -t 4 -x -0.5 -y -0.5 -s 0.2`

This will create 50 JPEG files named `mandel0.jpg`, `mandel1.jpg`, ..., `mandel49.jpg` in the current directory.

## Dependencies

This program requires the following libraries:

- `libjpeg`: For JPEG image handling
- `pthread`: For multi-threading support

On most Unix-based systems, these libraries should be available by default. If not, you may need to install them using your system's package manager.

## License

This program is released under the [MIT License](LICENSE).

## Credits

This program is based on example code from the following sources:

- [libjpeg example](https://www.tspi.at/2020/03/20/libjpegexample.html)
- [Mandelbrot set computation](https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html)

Modifications and improvements have been made by Zach Kohlman for the CPE 2600/121 course.
