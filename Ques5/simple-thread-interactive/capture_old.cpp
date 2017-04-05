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

#define HRES 640        //Horizontal resolution i.e. width
#define VRES 480        //Vertical resolution i.e. height

#define NUM_THREADS       	3
#define CANNY_DETECTOR_THREAD   1
#define HOUGH_LINE_THREAD       2
#define HOUGH_CIRCLE_THREAD     3

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

struct timeval tv;
char str1[200];
int temp=0;

pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param nrt_param;

sem_t sem_canny,sem_hough_line, sem_hough_circle;

double readTOD(void)
{
    struct timeval tv;
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

/* Trackbar callback function */
void *CannyThreshold(void* threadID)
{
    sem_wait(&sem_canny);

    /* Start capturing frames from camera */
    capture = (CvCapture *)cvCreateCameraCapture(dev);
    /* Set height property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    /* Set width property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);
    
    temp++;
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

    /* Releases the CvCapture structure */ 
    cvReleaseCapture(&capture);

    /* Destroy created window */
    cvDestroyWindow(timg_window_name);
    
    /* Release the semaphore for hough line thread */
    sem_post(&sem_hough_line);
     
    pthread_exit(NULL);
}

void *HoughLineFunc(void *threadID)
{
    sem_wait(&sem_hough_line);    

    /* Start capturing frames from camera */
    capture = (CvCapture *)cvCreateCameraCapture(dev);
    /* Set height property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    /* Set width property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);
    
    /* Grab a frame from a camera or video file, decompress and return it */ 
    frame=cvQueryFrame(capture);

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
    
    /* Releases the CvCapture structure */ 
    cvReleaseCapture(&capture);

    /* Destroy created window */
    cvDestroyWindow(timg_window_name);
    
    sem_post(&sem_hough_circle);
    
    pthread_exit(NULL);
}

void *HoughCircleFunc(void *threadID)
{
    sem_wait(&sem_hough_circle);

    /* Start capturing frames from camera */
    capture = (CvCapture *)cvCreateCameraCapture(dev);
    /* Set height property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    /* Set width property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);
    
    /* Grab a frame from a camera or video file, decompress and return it */
    frame=cvQueryFrame(capture);

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

    /* Releases the CvCapture structure */ 
    cvReleaseCapture(&capture);

    /* Destroy created window */
    cvDestroyWindow(timg_window_name);
    
    sem_post(&sem_canny);
    
    pthread_exit(NULL);
}

void Create_Threads(void)
{
    struct timeval timeNow;
    int rc;
	
    pthread_attr_init(&rt_sched_attr[CANNY_DETECTOR_THREAD]);
    pthread_attr_setinheritsched(&rt_sched_attr[CANNY_DETECTOR_THREAD], PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&rt_sched_attr[CANNY_DETECTOR_THREAD], SCHED_FIFO);

    pthread_attr_init(&rt_sched_attr[HOUGH_LINE_THREAD]);
    pthread_attr_setinheritsched(&rt_sched_attr[HOUGH_LINE_THREAD], PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&rt_sched_attr[HOUGH_LINE_THREAD], SCHED_FIFO);

    pthread_attr_init(&rt_sched_attr[HOUGH_CIRCLE_THREAD]);
    pthread_attr_setinheritsched(&rt_sched_attr[HOUGH_CIRCLE_THREAD], PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&rt_sched_attr[HOUGH_CIRCLE_THREAD], SCHED_FIFO);

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rc=sched_getparam(getpid(), &nrt_param);

    if (rc)
    {
        printf("ERROR; sched_setscheduler rc is %d\n", rc);
        perror(NULL);
        exit(-1);
    }
    
    rt_param[CANNY_DETECTOR_THREAD].sched_priority = rt_max_prio;
    pthread_attr_setschedparam(&rt_sched_attr[CANNY_DETECTOR_THREAD], &rt_param[CANNY_DETECTOR_THREAD]);

    printf("Creating thread %d\n", CANNY_DETECTOR_THREAD);
    rc = pthread_create(&threads[CANNY_DETECTOR_THREAD], &rt_sched_attr[CANNY_DETECTOR_THREAD], CannyThreshold, (void *)CANNY_DETECTOR_THREAD);

    if (rc)
    {
        printf("ERROR; pthread_create() rc is %d\n", rc);
        perror(NULL);
        exit(-1);
    }
    
    gettimeofday(&timeNow, NULL);
    printf("Canny detector Thread spawned at %d sec, %d usec\n", timeNow.tv_sec, (double)timeNow.tv_usec);

    rt_param[HOUGH_LINE_THREAD].sched_priority = rt_max_prio;
    pthread_attr_setschedparam(&rt_sched_attr[HOUGH_LINE_THREAD], &rt_param[HOUGH_LINE_THREAD]);

    printf("Creating thread %d\n", HOUGH_LINE_THREAD);
    rc = pthread_create(&threads[HOUGH_LINE_THREAD], &rt_sched_attr[HOUGH_LINE_THREAD], HoughLineFunc, (void *)HOUGH_LINE_THREAD);

    if (rc)
    {
        printf("ERROR; pthread_create() rc is %d\n", rc);
        perror(NULL);
        exit(-1);
    }
    
    gettimeofday(&timeNow, NULL);
    printf("Hough Line Thread spawned at %d sec, %d usec\n", timeNow.tv_sec, (double)timeNow.tv_usec);

    rt_param[HOUGH_CIRCLE_THREAD].sched_priority = rt_max_prio;
    pthread_attr_setschedparam(&rt_sched_attr[HOUGH_CIRCLE_THREAD], &rt_param[HOUGH_CIRCLE_THREAD]);

    printf("Creating thread %d\n", HOUGH_CIRCLE_THREAD);
    rc = pthread_create(&threads[HOUGH_CIRCLE_THREAD], &rt_sched_attr[HOUGH_CIRCLE_THREAD], HoughCircleFunc, (void *)HOUGH_CIRCLE_THREAD);

    if (rc)
    {
        printf("ERROR; pthread_create() rc is %d\n", rc);
        perror(NULL);
        exit(-1);
    }
    
    gettimeofday(&timeNow, NULL);
    printf("Hough Circle Thread spawned at %d sec, %d usec\n", timeNow.tv_sec, (double)timeNow.tv_usec);	
    
    /* Initializing bianry semaphores */
    sem_init(&sem_canny, 0, 1);
    /* Deliberately setting the initial value of this semaphore to 0, so that canny detector thread 
       is executed first and releases the sem_hough_line semaphore */
    sem_init(&sem_hough_line, 0, 0); 
    
    /* Deliberately setting the initial value of this semaphore to 0, so that canny detector thread 
       is executed first, followed by hough line thread and hough line thread releases the 
       sem_hough_circle semaphore */
    sem_init(&sem_hough_circle, 0, 0); 
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


    Create_Threads();
	
    start = readTOD();
    stop = readTOD();
    
#if 0
    while(1)
    {
	

        stop=readTOD();
	
        //Press q to quit
        char q = cvWaitKey(33);
        if(stop - start >= 2000)
        {
            printf("got quit\n"); 
            break;
        }
    }
#endif

    Destroy_Threads();
 
};
