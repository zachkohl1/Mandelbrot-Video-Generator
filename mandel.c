///
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
// Modified by Zach Kohlman, CPE 2600/121
// 
///
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "jpegrw.h"

// local routines
static int iteration_to_color(int i, int max);
static int iterations_at_point(double x, double y, int max);
static void compute_image(imgRawImage *img, double xmin, double xmax, double ymin,double ymax, int max, int row_start, int row_end);
static void show_help();

// File-scope variables to share with threads
static pthread_mutex_t mutex;
static sem_t sem; // Semaphore to make sure thread_num number of children make images at a time

// Create thread array
static pthread_t *threads = NULL;
static int rows_per_thread;

// These are the default configuration values used
// if no command line arguments are given.
static const char *outfile = "mandel.jpg";
static double xcenter = 0;
static double ycenter = 0;
static double xscale = 4;
static double yscale = 0; // calc later
static int image_width = 1000;
static int image_height = 1000;
static int max = 1000;
static int num_threads = 1;
static imgRawImage *img = NULL;


/**
 * @brief This function is the entry point for each thread in the program. Uses a mutex to lock the critical section, 
 * and a semaphore to limit the number of threads running at once. The mutex ensures that only one thread can access the critical section at a time
 * to prevent race conditions. The semaphore ensures that only num_threads number of threads are running at once.
 * 
 * @param vp A pointer to the thread argument (not used in this function).
 * @return void* Always returns NULL.
 */
void *thread_process(void *vp)
{
	// Lock mutex
	pthread_mutex_lock(&mutex);

	// Critical section
	static int row_start = 0;
	int row_end = row_start + rows_per_thread;

	compute_image(img, xcenter - xscale / 2, xcenter + xscale / 2, ycenter - yscale / 2, ycenter + yscale / 2, max, row_start, row_end);
	
	row_start += rows_per_thread;

	// Unlock mutex
	pthread_mutex_unlock(&mutex);

	// Post to semaphore
	sem_post(&sem);

	return NULL;
}

int main(int argc, char *argv[])
{
	// For each command line argument given,
	// override the appropriate configuration value.
	int c;
	while ((c = getopt(argc, argv, "x:y:s:W:H:m:o:h:t:")) != -1)
	{
		switch (c)
		{
		case 'x':
			xcenter = atof(optarg);
			break;
		case 't':
			num_threads = atoi(optarg);
			break;
		case 'y':
			ycenter = atof(optarg);
			break;
		case 's':
			xscale = atof(optarg);
			break;
		case 'W':
			image_width = atoi(optarg);
			break;
		case 'H':
			image_height = atoi(optarg);
			break;
		case 'm':
			max = atoi(optarg);
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'h':
			show_help();
			exit(1);
			break;
		}
	}

	// Initalize mutex
	pthread_mutex_init(&mutex, NULL);

	// Initalize semaphore to have num_threads number of threads running
	sem_init(&sem, 0, num_threads);

	// Initalize thread array
	threads = malloc(sizeof(pthread_t) * num_threads);

	// Create a raw image of the appropriate size.
	img = initRawImage(image_width, image_height);

	// Fill it with a black
	setImageCOLOR(img, 0);

	// Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
	yscale = xscale / image_width * image_height;

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf xscale=%lf yscale=%1f max=%d outfile=%s\n", xcenter, ycenter, xscale, yscale, max, outfile);

	// Calculate the number of rows per thread
	rows_per_thread = (image_height % num_threads == 0) ? image_height / num_threads : image_height / num_threads + 1;

	// Create threads
	for (int index = 0; index < num_threads; index++)
	{

		sem_wait(&sem);
		int ret = pthread_create(&threads[index], NULL, thread_process, NULL);

		if (ret != 0)
		{
			printf("Error creating thread %d\n", index);
			exit(1);
		}
	}
	// Join threads, so main doesn't exit before threads are done
	for (int i = 0; i < num_threads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	
	// Save the image in the stated file.
	storeJpegImageFile(img, outfile);

	// Destroy mutex
	pthread_mutex_destroy(&mutex);

	// Destroy semaphore
	sem_destroy(&sem);

	// free the mallocs
	freeRawImage(img);
	
	// Free thread array
	free(threads);

	return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point(double x, double y, int max)
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while ((x * x + y * y <= 4) && iter < max)
	{

		double xt = x * x - y * y + x0;
		double yt = 2 * x * y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
Each thread computes n-rows

MODIFIED: Added row_start and row_end to compute n-rows per thread so that multiple threads can compute the image in parallel
*/

void compute_image(imgRawImage *img, double xmin, double xmax, double ymin, double ymax, int max, int row_start, int row_end)
{
	int i, j;

	// Width and height of the image in pixels
	int width = img->width;
	int height = img->height;

	// For every pixel in the image...
	for (j = row_start; j <= row_end; j++)
	{

		for (i = 0; i < width; i++)
		{

			// Determine the point in x,y space for that pixel.
			double x = xmin + i * (xmax - xmin) / width;
			double y = ymin + j * (ymax - ymin) / height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x, y, max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(img, i, j, iteration_to_color(iters, max));
		}
	}
}

/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
int iteration_to_color(int iters, int max)
{
	int color = 0xFFFFFF * iters / (double)max;
	return color;
}

// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels> Height of the image in pixels. (default=1000)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}
