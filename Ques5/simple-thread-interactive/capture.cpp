/*
 *
 *  Example by Sam Siewert 
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <unistd.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <sys/time.h>
#include <time.h>
#include <bits/stdc++.h>

using namespace cv;
using namespace std;


#define NUM_THREADS       		3
#define CANNY_DETECTOR_THREAD   	1
#define HOUGH_LINE_THREAD       	2
#define HOUGH_CIRCLE_THREAD     	3

#ifdef RES_640_480
#define HRES 640        //Horizontal resolution i.e. width
#define VRES 480        //Vertical resolution i.e. height

#define CANNY_SOFT_DEADLINE_MS      	130    /* Corresponding to an average frame rate of 8.7 fps + ~20 ms margin */
#define HLINE_SOFT_DEADLINE_MS      	190    /* Corresponding to an average frame rate of 5.85 fps + ~20 ms margin */
#define HCIRCLE_SOFT_DEADLINE_MS      	170    /* Corresponding to an average frame rate of 6.5 fps + ~20 ms margin */
#define COMBINED_SOFT_DEADLINE_MS	500  
#endif

#ifdef RES_320_240   
#define HRES 320        //Horizontal resolution i.e. width
#define VRES 240        //Vertical resolution i.e. height

#define CANNY_SOFT_DEADLINE_MS      	70     /* Corresponding to an average frame rate of 18 fps + ~20 ms margin */
#define HLINE_SOFT_DEADLINE_MS      	110    /* Corresponding to an average frame rate of 10.3 fps + ~20 ms margin */
#define HCIRCLE_SOFT_DEADLINE_MS      	85     /* Corresponding to an average frame rate of 14.7 fps + ~20 ms margin */
#define COMBINED_SOFT_DEADLINE_MS	265  
#endif

#ifdef RES_160_120
#define HRES 160        //Horizontal resolution i.e. width
#define VRES 120        //Vertical resolution i.e. height

#define CANNY_SOFT_DEADLINE_MS      	65     /* Corresponding to an average frame rate of 20.65 fps + ~20 ms margin */
#define HLINE_SOFT_DEADLINE_MS      	80     /* Corresponding to an average frame rate of 15.2 fps + ~20 ms margin */
#define HCIRCLE_SOFT_DEADLINE_MS      	70     /* Corresponding to an average frame rate of 19 fps + ~20 ms margin */
#define COMBINED_SOFT_DEADLINE_MS	215  
#endif

#define NUM_ITERATIONS			100

// Transform display window
char timg_window_name[] = "Edge Detector Transform";

/* Variable declarations that will be used by canny edge detector, hough line and circle threads */
int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
int dev=0;
double start=0, stop=0;

Mat canny_frame, cdst, timg_gray, timg_grad, gray;
vector<Vec4i> lines;
vector<Vec3f> circles;
CvCapture* capture;
IplImage* frame; /* C structure to store the image in the memory */

pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param nrt_param;

sem_t sem_canny;
sem_t sem_hough_line;
sem_t sem_hough_circle;
sem_t sem_display_timestamp;

struct timeval tv;
char str1[200];
int temp=0;
double temp_time=0, prev_frame_time=0;
double ave_framedt=0.0, ave_frame_rate=0.0, fc=0.0, framedt=0.0;

double canny_start_time = 0, canny_stop_time = 0;
double hline_start_time = 0, hline_stop_time = 0;
double hcircle_start_time = 0, hcircle_stop_time = 0;

double readTOD(void)
{
    double ft=0.0;
    if ( gettimeofday(&tv, NULL) != 0 )
    {
        perror("readTOD");
        return 0.0;
    }
    else
    {
        ft = ((double)(((double)tv.tv_sec) + (((double)tv.tv_usec) /1000000.0)));
   	ft = ft*1000;
    }
    return ft;
}

void display_timestamp()
{
    sem_wait(&sem_display_timestamp);    

    if(temp > 2)
    {
   	fc=(double)temp;
	ave_framedt=((fc-1.0)*ave_framedt + framedt)/fc;
	ave_frame_rate=1.0/(ave_framedt/1000.0);
    }
 
    framedt=temp_time - prev_frame_time;
    prev_frame_time=temp_time;
    printf("Frame @ %u sec, %lu usec, dt=%5.2lf msec, avedt=%5.2lf msec, rate=%5.2lf fps, Jitter = %d ms\n", 
           (unsigned)tv.tv_sec, 
           (unsigned long)tv.tv_usec,
           framedt, ave_framedt, ave_frame_rate,
	   (int)(COMBINED_SOFT_DEADLINE_MS - ave_framedt));
}

/* Trackbar callback function */
void *CannyThreshold(void* threadID)
{
    while(temp < NUM_ITERATIONS)
    {
    	canny_start_time = readTOD();

	sem_wait(&sem_canny);

	Mat mat_frame(frame);
    
	cvtColor(mat_frame, timg_gray, CV_RGB2GRAY);	//Converts RGB image to gray scale image

	/// Reduce noise with a kernel 3x3
	blur( timg_gray, canny_frame, Size(3,3) );	

	/// Canny detector
	Canny( canny_frame, canny_frame, lowThreshold, lowThreshold*ratio, kernel_size );

	/// Using Canny's output as a mask, we display our result
	timg_grad = Scalar::all(0); //Fills the timg_grad matrix with all 0s

	mat_frame.copyTo( timg_grad, canny_frame);	//copies canny_frame to timg_grad

	imshow( timg_window_name, timg_grad );	//displays the timg_grad image

	/* Release the semaphore for hough line thread */
	sem_post(&sem_hough_line);

	canny_stop_time = readTOD();     

	printf("Canny Detector Thread execution time: %d ms, Soft Deadline: %d ms, Jitter: %d ms\n", 
		(int)(canny_stop_time - canny_start_time), CANNY_SOFT_DEADLINE_MS, 
		((int)(canny_stop_time - canny_start_time) - CANNY_SOFT_DEADLINE_MS)); 
    }
    
    pthread_exit(NULL);
}

void *HoughLineFunc(void *threadID)
{
    while (temp < NUM_ITERATIONS)
    {	
    	hline_start_time = readTOD();

	sem_wait(&sem_hough_line);    

	Mat mat_frame(frame); /* Copy Constructor */

	/* Canny Edge detector with thresholds (50 (low) and 200 (high)) and sobel kernel size 3 */
	Canny(mat_frame, canny_frame, 50, 200, 3);

	//cvtColor(canny_frame, cdst, CV_GRAY2BGR);
	//cvtColor(mat_frame, gray, CV_BGR2GRAY);

	/* Apply probabilistic hough transform */
	HoughLinesP(canny_frame, lines, 1, CV_PI/180, 50, 50, 10);

	/* Display the result by drawing lines detected by hough transform */
	for( size_t i = 0; i < lines.size(); i++ )
	{
	     Vec4i l = lines[i];
	     line(mat_frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
	}

	cvShowImage("Capture Example", frame);

	sem_post(&sem_hough_circle);

	hline_stop_time = readTOD();

	printf("Hough Line Thread execution time: %d ms, Soft Deadline: %d ms, Jitter: %d ms\n", 
		(int)(hline_stop_time - hline_start_time), HLINE_SOFT_DEADLINE_MS,
		((int)(hline_stop_time - hline_start_time) - HLINE_SOFT_DEADLINE_MS)); 
    
    }

    pthread_exit(NULL);
}

void *HoughCircleFunc(void *threadID)
{
    while (temp < NUM_ITERATIONS)
    {
    
    	hcircle_start_time = readTOD();
    
	sem_wait(&sem_hough_circle);

	/* Copy constructor */
	Mat mat_frame(frame);

	/* RGB to GRAY color conversion */
	cvtColor(mat_frame, gray, CV_BGR2GRAY);

	/* Blur the image using gaussian filtering to avoid false detections */
	GaussianBlur(gray, gray, Size(9,9), 2, 2);

	/* Find circles in the image using hough transform */
	HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8, 100, 50, 0, 0);

	printf("circles.size = %d\n", circles.size());

	for( size_t i = 0; i < circles.size(); i++ )
	{
	    Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
	    int radius = cvRound(circles[i][2]);
	    // circle center
	    circle( mat_frame, center, 3, Scalar(0,255,0), -1, 8, 0 );
	    // circle outline
	    circle( mat_frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
	}

	cvShowImage("Capture Example", frame);

	sem_post(&sem_canny);

	sem_post(&sem_display_timestamp);

	hcircle_stop_time = readTOD();    

	printf("Hough Circle Thread execution time: %d ms, Soft Deadline: %d ms, Jitter: %d ms\n", 
		(int)(hcircle_stop_time - hcircle_start_time), HCIRCLE_SOFT_DEADLINE_MS,
		((int)(hcircle_stop_time - hcircle_start_time) - HCIRCLE_SOFT_DEADLINE_MS)); 
 
    }

    pthread_exit(NULL);
}

void Create_Threads(void)
{
    struct timeval timeNow;
    int rc;
	
    printf("Creating thread %d\n", CANNY_DETECTOR_THREAD);
    rc = pthread_create(&threads[CANNY_DETECTOR_THREAD], NULL, CannyThreshold, (void *)CANNY_DETECTOR_THREAD);

    if (rc)
    {
        printf("ERROR; pthread_create() rc is %d\n", rc);
        perror(NULL);
        exit(-1);
    }
    
    gettimeofday(&timeNow, NULL);
    printf("Canny detector Thread spawned at %d sec, %d usec\n", timeNow.tv_sec, (double)timeNow.tv_usec);

    printf("Creating thread %d\n", HOUGH_LINE_THREAD);
    rc = pthread_create(&threads[HOUGH_LINE_THREAD], NULL, HoughLineFunc, (void *)HOUGH_LINE_THREAD);

    if (rc)
    {
        printf("ERROR; pthread_create() rc is %d\n", rc);
        perror(NULL);
        exit(-1);
    }
    
    gettimeofday(&timeNow, NULL);
    printf("Hough Line Thread spawned at %d sec, %d usec\n", timeNow.tv_sec, (double)timeNow.tv_usec);

    printf("Creating thread %d\n", HOUGH_CIRCLE_THREAD);
    rc = pthread_create(&threads[HOUGH_CIRCLE_THREAD], NULL, HoughCircleFunc, (void *)HOUGH_CIRCLE_THREAD);

    if (rc)
    {
        printf("ERROR; pthread_create() rc is %d\n", rc);
        perror(NULL);
        exit(-1);
    }
    
    gettimeofday(&timeNow, NULL);
    printf("Hough Circle Thread spawned at %d sec, %d usec\n", timeNow.tv_sec, (double)timeNow.tv_usec);	
    
}

void Destroy_Threads(void)
{
    if(pthread_join(threads[CANNY_DETECTOR_THREAD], NULL) == 0)
        printf("Canny Detector Thread done\n");
    else
        perror("Canny Detector Thread");

    if(pthread_join(threads[HOUGH_LINE_THREAD], NULL) == 0)
        printf("Hough Line Thread done\n");
    else
        perror("Hough Line Thread");

    if(pthread_join(threads[HOUGH_CIRCLE_THREAD], NULL) == 0)
        printf("Hough Circle Thread done\n");
    else
        perror("Hough Circle Thread");
}

int main( int argc, char** argv )
{
    /* Create a window - with CV_WINDOW_AUTOSIZE, the window size is automatically adjusted to fit the displayed image */
    namedWindow( timg_window_name, CV_WINDOW_AUTOSIZE );
    
    if(argc > 1)
    {
        sscanf(argv[1], "%d", &dev);
        printf("using %s\n", argv[1]);
    }
    else if(argc == 1)
        printf("using default\n");

    else
    {
        printf("usage: capture [dev]\n");
        exit(-1);
    }

    /* Start capturing frames from camera */
    capture = (CvCapture *)cvCreateCameraCapture(dev);
    /* Set height property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    /* Set width property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

    /* Initializing bianry semaphores */
    sem_init(&sem_canny, 0, 0);
    /* Deliberately setting the initial value of this semaphore to 0, so that canny detector thread 
       is executed first and releases the sem_hough_line semaphore */
    sem_init(&sem_hough_line, 0, 0); 
    
    /* Deliberately setting the initial value of this semaphore to 0, so that canny detector thread 
       is executed first, followed by hough line thread and hough line thread releases the 
       sem_hough_circle semaphore */
    sem_init(&sem_hough_circle, 0, 0); 

    sem_init(&sem_display_timestamp, 0, 1);
    
    Create_Threads();
	
    /* Grab a frame from a camera or video file, decompress and return it */
   
    while (temp < 100)
    { 
  	temp_time = readTOD();
        display_timestamp();
        frame=cvQueryFrame(capture);
    	temp++;
        sem_post(&sem_canny);
    }

    Destroy_Threads();
 

    /* Releases the CvCapture structure */ 
    cvReleaseCapture(&capture);

    /* Destroy created window */
    cvDestroyWindow(timg_window_name);
};
