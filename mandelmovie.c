/**
 * @file mandelmovie.c
 * @brief This file contains the implementation of a program that sends a signal to a specified process ID.
 *        The program takes in a process ID and a signal number as command line arguments, and sends the specified signal to the process.
 *        Now allows for threads to be used to create images in parallel.
 * @author Zach Kohlman, CPE 2600/121
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "jpegrw.h"

static const int MAX_IMAGES = 50; // Num images to generate with processes
static int image_count = 0;       // Volatile because accessed in signal handler and main
static sem_t sem;                 // Semaphore to make sure only CONCURRENT_CHILDREN number of children make images at a time
int concurrent_children = 1;     
int num_threads = 1;

/**
 * This function initializes a signal handler for SIGUSR1 signal and spawns child processes to create images of the Mandelbrot set.
 * The function takes in command line arguments to specify the number of child processes to spawn and prints help information if requested.
 * The function uses semaphores to limit the number of concurrent child processes creating images to avoid deadlock.
 *
 * @param argc The number of command line arguments
 * @param argv An array of command line argument strings
 * @return 0 if the program runs successfully, 1 if there is an error
 */
int main(int argc, char *argv[])
{
    int scale = 0;
    char x_cord[100];
    char y_cord[100];
    char height[100];
    char width[100];
    char max[100];
    struct timespec start, end;

    // execv program arguments
    char *mandel_path = "./mandel"; // Should really be const, but compiler says gives warning "discards const"
    char scale_buffer[5];           // 5 character buffer to be safe. Used to convert int to string
    const int OUT_FILE_LEN = 15;    // "mandel##.jpg" is 12+1 characters long, 15 to be safe.
    char out_file[OUT_FILE_LEN];
    char num_threads[5] = "1"; // Changed to an array to store mutable string
    int c; // getopt returns each option character from each of the option elements

    for(int i = 0; i < argc; i++)
    {
        printf("argv[%i] = %s\n", i , argv[i]);
    }
    // Looks for -c #children and -h for help
    while ((c = getopt(argc, argv, "c:h:t:x:y:m:H:W:")) != -1)
    {
        switch (c)
        {
        case 't':
            // Number of threads to use. Mallocs internally
            strncpy(num_threads, optarg, sizeof(num_threads)-1);
            // num_threads = strdup(optarg);
            break;
        case 'c':
            // Number of children processes
            concurrent_children = atoi(optarg);
            break;
        case 'x':
            strncpy(x_cord, optarg, sizeof(x_cord)-1);
            printf("X_CORD: %s\n", x_cord);
            break;
        case 'y':
            strncpy(y_cord, optarg, sizeof(y_cord)-1);
            printf("Y_CORD: %s\n", y_cord);
            break;
        case 'm':
            strncpy(max, optarg, sizeof(max)-1);
            break;
        case 'H':
            strncpy(height, optarg, sizeof(height)-1);
            break;
        case 'W':
            strncpy(width, optarg, sizeof(width)-1);
            break;
        case 'h':
            // Help menu, exits
            printf("-h  To print some help\n");
            printf("-c  <num children> Number of children processes\n");
            exit(1);
            break;
        }
    }

        // Initalize semaphore
        sem_init(&sem, 0, concurrent_children);

        // Start clock
        clock_gettime(CLOCK_REALTIME, &start);

        printf("x-cord: %s y-cord: %s max: %s\n", x_cord, y_cord, max);

        // Make sure total_chlidren number of forks
        for (image_count = 0; image_count < MAX_IMAGES; image_count++)
        {
            // If we already have the images, don't make any more children.

            /**
             * Makes sure we stay within the number of current_children running.
             * KEY: sem_wait decresses sem value by 1, so WILL WAIT/BLOCK if sem value = 0 until the previous semaphore posts
             * indicating that one child is done and incrimenting semaphore value by 1.
             * So, this blocks if program already has CONCURRENT_CHILDREN number of processes running. Used to limit the number
             * of children processes making an image at the same time to avoid deadlock i.e. children waiting for a resource to clear
             */
            sem_wait(&sem);

            // Increase scale by 2 for each child process. After fork() child stack is a copy of parent stack, so scale is preserved.
            // Child process exits after execv with exit(0), so scale increases for each child process.
            scale++;

            // Spawn another child process. The return of fork() is child process is 0,
            // PID REFERS TO NEWLY CREATED CHILD. PID = 0 IF FORKED IN CHILD, PID > 0 IF FORKED IN PARENT
            pid_t pid = fork();

            // Parent process. Waits for all children to be finished.
            if (pid > 0)
            {
                while (wait(NULL) > 0)
                    ; // Wait for all children to finish
            }

            // Child process. Is where the image creation is done in parllel with other children by calling execv.
            else if (pid == 0)
            {
                // Save unique file name
                sprintf(out_file, "mandel%d.jpg", image_count);

                // Zoom in to out, so decrease scale based on image count
                sprintf(scale_buffer, "%d", MAX_IMAGES - scale);

                // Args for mandel program (command line). Execv requires NULL at the END!
                char *const mandel_args[] = {mandel_path, "-s", scale_buffer, "-x", x_cord, "-y", y_cord, "-t", num_threads, "-m", max, "-o", out_file, NULL};


                // Replace current process with mandel process
                if (execv(mandel_path, mandel_args) == -1)
                {
                    // Only returns if error has occured
                    perror("execv");
                    exit(EXIT_FAILURE);
                }
                // Close current semaphore. Wait to post until after the current child is done exiting.
                sem_destroy(&sem);

                // Exit child process to increase scale for next child process
                exit(0);
            }
            // Error forking
            else if (pid < 0)
            {
                // Print error and exit
                perror("fork");
                exit(EXIT_FAILURE);
            }
            // Post semaphore to signal to program that another child can spawn.
            sem_post(&sem);
        }
        // End clock
        clock_gettime(CLOCK_REALTIME, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Time taken: %f\n", time_taken);

        // if(t_typed == 1)
        // {
        //     free(num_threads);
        // }
        return 0;
    }

