//
//  threaded.h
//  CoScheduler
//
//  Created by Kartik Vedalaveni on 5/8/13.
//  Copyright (c) 2013 Kartik Vedalaveni. All rights reserved.
//

#ifndef CoScheduler_threaded_h
#define CoScheduler_threaded_h
#include<iostream>


class resource	{
	
public:
	int capacity;
	int avgTurnaroundTime;
	unsigned int T1;
    unsigned int T2;
    unsigned int bestKValue;
	float c1;
    float c2;
    unsigned int kValue;
    int degradation_high;
    int degradation_low;
    int tid;
    
	resource()	{
		
	}
	
};

typedef struct args	{
	int kValue;
	std::string hostInfo;
	std::string genInfo;
    int tid;
	
}args;


//Wait condotion for thread, defining a conditional varible(adapted from stackoverflow.com)
void mywait(int timeInSec)
{
	
	pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;
	
	struct timespec timeToWait;
	struct timeval now;
	int rt;
	
	gettimeofday(&now,NULL);
	
	timeToWait.tv_sec = now.tv_sec + timeInSec;
	timeToWait.tv_nsec = now.tv_usec*1000;
	
	pthread_mutex_lock(&fakeMutex);
	rt = pthread_cond_timedwait(&fakeCond, &fakeMutex, &timeToWait);
	pthread_mutex_unlock(&fakeMutex);
	
}


#endif
