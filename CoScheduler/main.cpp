//
//  main.cpp
//  CoScheduler
//
//  Created by Kartik Vedalaveni on 5/8/13.
//  Copyright (c) 2013 Kartik Vedalaveni. All rights reserved.
//


#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <fstream>
#include "user_log.c++.h"
#include <cmath>
#include <map>
#include <sstream>
#include "submission.h"
#include <ctime>
#include <pthread.h>
#include "threaded.h"
#define MAXSITES 10
using namespace std;

pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t syncMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t synchronize_cv = PTHREAD_COND_INITIALIZER;

unsigned long int maxJob=0,sumOfK=0, T=0,ret1;
int numOfSites=0;
args arguments[MAXSITES];
resource stats[MAXSITES];
string gridSites[MAXSITES], genFiles[MAXSITES], hostFiles[MAXSITES];
ofstream output[MAXSITES];

//For multiple sites algorithm
list<int> multipleSitesTime;
map<int,int> tid_multipleSiteTime;

int optimalRun(int,int,resource*);
bool maxJobLimit(int);
void lastIteration(int, int, resource*);
unsigned int jobSubmission(int,args *);
void updateJobPropagationConstant(int,resource *);

int updateJobPropagationConstant(resource *data)	{
	int high=0, low=0, average=0;
	
	multipleSitesTime.sort();
	low = multipleSitesTime.front();
	high = multipleSitesTime.back();
	
	for (std::list<int>::iterator it = multipleSitesTime.begin(); it != multipleSitesTime.end(); it++)	{
		average+=*it;
	}
	
	average = average/multipleSitesTime.size();
	
	int it = tid_multipleSiteTime.at(data->tid);
	if(it==low)	{
		pthread_mutex_lock (&mymutex);
		data->c1=(data->c1)*4;
		pthread_mutex_unlock (&mymutex);
		output[data->tid]<<"\nC1 updated to "<<data->c1<<endl;
		
		return 0;
	}
	else if(it==high)	{
		pthread_mutex_lock (&mymutex);
		data->c1=((data->c1)/2)>1?((data->c1)/2):1;
		pthread_mutex_unlock (&mymutex);
		output[data->tid]<<"\nC1 updated to "<<data->c1<<endl;
		
		return 0;
	}
	else if(it<average)	{
		pthread_mutex_lock (&mymutex);
		data->c1=(data->c1)*2;
		pthread_mutex_unlock (&mymutex);
		output[data->tid]<<"\nC1 updated to "<<data->c1<<endl;
		
		return 0;
	}
	
	/*map<int,int>::iterator test;
	for(std::map<int,int>::iterator it=tid_multipleSiteTime.begin(); it!=tid_multipleSiteTime.end(); ++it)
	{
		
	}*/
	
	
	
}

void capacityDetection(resource *data)	{
	
	int thread_id = data->tid;
	int degradation_high = data->degradation_high;
	int degradation_low = data->degradation_low;
	
	while(true)
	{
		cout.flush();
		
		//Normal execution, control block
		if(data->T2 < data->c2*data->T1)	{
			
			// If there is no degradation increase number of jobs submitted
			data->kValue=(data->c1);
			
			if(maxJobLimit(data->kValue))	{
				
				
				//arguments[thread_id].kValue=k;
				jobSubmission(data->kValue,&arguments[thread_id]);
				
				//taken care of T2 initialization in jobSubmission function
				//T2 = data->T2;
				
				cout<<"\nJobs Completed: "<<sumOfK<<endl;
				cout<<"Jobs Remaining: "<<(maxJob-sumOfK)<<endl;
				
				output[thread_id]<<"\nJobs Completed (or in Progress) : "<<sumOfK<<endl;
				output[thread_id]<<"Jobs Remaining                    : "<<(maxJob-sumOfK)<<endl;
				
				if(data->T2< data->c2*data->T1){
					data->T1=(data->T1+data->T2)/2;
					
				}
				
				if(!multipleSitesTime.empty())	{
					pthread_mutex_lock (&mymutex);
					int tmp=tid_multipleSiteTime.at(data->tid);
					multipleSitesTime.remove(tmp);
					
					multipleSitesTime.push_back(stats[thread_id].T2);
					pthread_mutex_unlock (&mymutex);
				}
				
				if(!tid_multipleSiteTime.empty())	{
				pthread_mutex_lock (&mymutex);
				tid_multipleSiteTime.erase(data->tid);
				tid_multipleSiteTime.insert(std::pair<int,int>(data->tid,data->T2));
				pthread_mutex_unlock (&mymutex);
				}
				
				mywait(3);
				
				//cout<<"Number Of Sites: ="<<numOfSites<<" MultipleSitesTimeSize ="<<multipleSitesTime.size();
				
				if(multipleSitesTime.size()>0)	{
					updateJobPropagationConstant(data);
				}
				
				
				
			}
			else{
			lastIteration(data->c1, data->kValue, data);
			}
		}
		
		//Re-read T1's updated data
		//data->T1 = T1;
		
		//Degradation control block
		if(data->T2> data->c2*data->T1){
			
			cout<<"T1: "<<data->T1<<"\tT2: "<<data->T2<<flush;
			output[thread_id]<<"T1: "<<data->T1<<"\tT2: "<<data->T2<<flush;
			
			degradation_high=data->kValue;
			data->degradation_high = data->kValue;
			
			degradation_low =((data->kValue)/(data->c1)==0?1:(data->kValue/data->c1));
			data->degradation_low = ((data->kValue)/(data->c1)==0?1:(data->kValue/data->c1));
			
			cout<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			output[thread_id]<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			
			
			//Find the optimal K for the
			if((degradation_high-degradation_low)>0)
				optimalRun(degradation_low,degradation_high, data);
			
			//Get best value here
			data->kValue=data->bestKValue;
			unsigned int time=0;
			
			while(time<10*data->T1)	{
				
				if(maxJobLimit(data->kValue))	{
					wallClockList[thread_id].clear();
					
					//arguments[thread_id].kValue=k;
					jobSubmission(data->kValue,&arguments[thread_id]);
					
					//T2 = data->T2;
					
					wallClockList[thread_id].sort();
					time += wallClockList[thread_id].back();
					
					if(time>10*data->T1)
						time=10*data->T1;
					
					cout<<"\nJobs Completed(or In progress): "<<sumOfK<<endl;
					cout<<"Jobs Remaining                  : "<<maxJob-sumOfK<<endl;
					cout<<"\n\nSeconds till search resumes :  "<<(10*data->T1-time)<<endl<<flush;
					
					output[thread_id]<<"\nJobs Completed (or In progress): "<<sumOfK<<endl;
					output[thread_id]<<"Jobs Remaining                   : "<<maxJob-sumOfK<<endl;
					output[thread_id]<<"\n\nSeconds till search resumes  :  "<<(10*(data->T1)-time)<<endl<<flush;
					
				}
				else{
					lastIteration(data->c1,data->kValue,data);
				}
			}
		}
		
		
		
	}
	
}

void* watchCode(void *watchArgs)	{
	
	pthread_mutex_lock(&syncMutex);
	if(numOfSites==multipleSitesTime.size())	{
		pthread_cond_signal(&synchronize_cv);
	}
	pthread_mutex_unlock(&syncMutex);
	
	pthread_exit(&ret1);
}

//Job submission primitive
void* jobSubmissionThreaded(void *threadArg)	{
	
	args *data = (args *) threadArg;
	int k = data->kValue;
	string host = data->hostInfo;
	string geninfo = data->genInfo;
	int thread_id = data->tid;
	
	int returnValue=99;
	//Clear host.log
	ofstream clear1(host.c_str(),ios::trunc);
	clear1.close();
	//sleep(5);
	mywait(5);
	
	submission s1(k);
	returnValue=s1.submit(geninfo, data);
	//sleep(5); //Wait 5 seconds for log file to be written before its read.
	mywait(5);
	
	if(returnValue==0)	{
		s1.wait_calculate(host,data,numOfSites);
		stats[thread_id].T1=s1.calculateWallClock(numOfSites,data);
	}
	else{
		cout<<"Error completing job! Exiting!";
		output[thread_id]<<"Error completing job! Exiting!";
		pthread_exit(&ret1);
		
	}
	
	//sleep(5);
	mywait(5);
	
	
	//cout<<"Number Of Sites: ="<<numOfSites<<" MultipleSitesTimeSize ="<<multipleSitesTime.size();
	
/*	if(data->tid==0)	{
		
			
		pthread_mutex_lock(&syncMutex);
		if(numOfSites==multipleSitesTime.size())	{
			pthread_cond_signal(&synchronize_cv);
		}
		pthread_cond_broadcast(&synchronize_cv);
		pthread_mutex_unlock(&syncMutex);

	}
	else{
		
		pthread_mutex_lock(&syncMutex);
		while(multipleSitesTime.size()<numOfSites)	{
			pthread_cond_wait(&synchronize_cv, &syncMutex);
		}
		pthread_mutex_unlock(&syncMutex);
	}
*/
	pthread_mutex_lock(&syncMutex);
    
	multipleSitesTime.push_back(stats[thread_id].T1);
	
	tid_multipleSiteTime.insert(std::pair<int,int>(data->tid,stats[thread_id].T1));
	
    
    if ( multipleSitesTime.size() < numOfSites )
    {
        pthread_cond_wait(&synchronize_cv, &syncMutex);
    }
    else
    {
		
        pthread_cond_broadcast(&synchronize_cv);
    }
    
    pthread_mutex_unlock(&syncMutex);



	T = stats[thread_id].T1;
	job_term_times[thread_id].clear();
	execute_times[thread_id].clear();
	wallClockTimes[thread_id].clear();
	
	cout<<"T(average) "<<T<<" over "<<s1.count-1<<" Jobs"<<endl<<flush;
	output[thread_id]<<"T(average) "<<T<<" over "<<s1.count-1<<" Jobs"<<endl<<endl<<flush;
	
		//Clear host.log
	ofstream clear(host.c_str(),ios::trunc);
	clear.close();
	//sleep(5);
	mywait(5);
	
	
	//Form resource datastructure here and submit to capacityDetection
	
	
	stats[thread_id].T2=0;
	stats[thread_id].kValue=k;
	stats[thread_id].tid = thread_id;
	stats[thread_id].c1 = 2;
	stats[thread_id].c2 = 1.25;
	stats[thread_id].degradation_low=1;
	stats[thread_id].degradation_high=1;
	
	capacityDetection(&stats[thread_id]);
}


//Job submission primitive
unsigned int jobSubmission(int kValue,args *data)	{
	
	int k = kValue;
	string host = data->hostInfo;
	string geninfo = data->genInfo;
	int thread_id = data->tid;
	
	int returnValue=99;
	//Clear host.log
	ofstream clear1(host.c_str(),ios::trunc);
	clear1.close();
	//sleep(5);
	mywait(5);
	
	if(k==0)	{
		k=1;
	}
	submission s1(k);
	returnValue=s1.submit(geninfo, data);
	mywait(3);
	pthread_mutex_lock (&mymutex);
	sumOfK+=k;
	pthread_mutex_unlock (&mymutex);
	//sleep(5); //Wait 5 seconds for log file to be written before its read.
	mywait(5);
	
	if(returnValue==0)	{
		s1.wait_calculate(host,data,numOfSites);
		stats[thread_id].T2=s1.calculateWallClock(numOfSites,data);
	}
	else{
		cout<<"Error completing job! Exiting!";
		output[thread_id]<<"Error completing job! Exiting!";
		pthread_exit(&ret1);
		
	}
	
	//sleep(5);
	mywait(5);
	T=stats[thread_id].T2;
	job_term_times[thread_id].clear();
	execute_times[thread_id].clear();
	wallClockTimes[thread_id].clear();
	
	cout<<"T(average) "<<T<<" over "<<s1.count-1<<" Jobs"<<endl<<endl<<flush;
	output[thread_id]<<"T(average) "<<T<<" over "<<s1.count-1<<" Jobs"<<endl<<endl<<flush;
	
	//Clear host.log
	ofstream clear(host.c_str(),ios::trunc);
	clear.close();
	//sleep(5);
	mywait(8);
	
}


//Check for MaxJob limit
bool maxJobLimit(int k)	{
	
	if((sumOfK+k)>=(maxJob))	{
		return false;
	}
	else{
		return true;
	}
}

void lastIteration(int c1, int k, resource* data)	{
	//Last Iteration, Submit remaining jobs to complete exact set of input jobs.
	if((sumOfK+k) > maxJob)	{
		k = maxJob-sumOfK;
		arguments[data->tid].kValue=k;
		jobSubmission(k,&arguments[data->tid]);
		//		cout<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
		//		output[data->tid]<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
		
		cout<<"\n\n\nJobs Completed!" <<endl<<flush;
		output[data->tid]<<"\n\n\nJobs Completed!" <<endl<<flush;
		pthread_exit(&ret1);
	}
}


//Best k for a given range around degraded value
int optimalRun(int low, int high,resource *data)	{
	
	int mid = (low+high)/2;
	unsigned long int tmp=0;
	
	if(maxJobLimit(mid))	{
		
		arguments[data->tid].kValue=mid;
		//sumofk is updated in job submission, not required here
		jobSubmission(mid,&arguments[data->tid]);
		
		tmp=data->T2;
		data->T2=tmp;
		cout<<"\nJobs Completed: "<<sumOfK<<endl;
		cout<<"Jobs Remaining: "<<(maxJob-sumOfK)<<endl;
		
		output[data->tid]<<"\nJobs Completed: "<<sumOfK<<endl;
		output[data->tid]<<"Jobs Remaining: "<<(maxJob-sumOfK)<<endl;
		
		if(tmp < (data->c2)*(data->T1))	{
			data->T1=((data->T1)+tmp)/2;
			if(low+2==high)	{
				data->bestKValue = mid;
				return(mid);
			}
			optimalRun(mid, high, data);
		}
		else{
			cout<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			output[data->tid]<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			
			if(low+2==high)	{
				
				//If the final value detected by binary search is degraded value, we choose a lower value
				data->bestKValue=mid-1;
				return(mid-1);
			}
			optimalRun(low, mid,data);
		}
	}
	else{
		lastIteration(data->c1,mid, data);
		return mid;
	}
	
}




int main(int argc, char **argv)      {
	
	
	int k=1;
	maxJob = atoi(argv[1]);
	
	//take argv2 input as sites file and create that many hosts
	ifstream sites(argv[2]);
	int i=0;
	if(!sites.is_open())	{
		cout<<"\nUnable to open File sites, Exiting!"<<endl;
		pthread_exit(&ret1);
	}
	else{
		
		while(getline(sites, gridSites[i]))	{
			i++;
		}
		
	}
	
	numOfSites = i;
	sites.close();
	
	//Init condor submit scripts
	//So that we have separate log file for each cluster submission.
	i=0;
	
	
	while(i<numOfSites)	{
		char genScript[100], hostFile[100];
		
		//take argv3 input as submitScript file
		ifstream src(argv[3]);
		sprintf(genScript,"genScript%d",i);
		sprintf(hostFile,"host%d.log",i);
		genFiles[i] = genScript;
		hostFiles[i] = hostFile;
		ofstream dst(genScript,ios::out);
		
	    dst<<src.rdbuf();
		dst<<"echo \"grid_resource=gt5 "<<gridSites[i]<<"\""<<endl;
		dst<<"echo \"log="<<hostFile<<"\""<<endl;
		dst<<"echo \"queue $1\""<<endl;
		
		dst.close();
		src.close();
		i++;
	}
	
	//Give execute permissions to generated scripts
	system("chmod +x gen*");
	
	//Submit and wait
	
	i=0;
	int rc;
	char tmp[100];
	pthread_t threads[MAXSITES];
	while(i<numOfSites)	{
		sprintf(tmp,"output%d.txt",i);
		output[i].open(tmp, ofstream::out);
		sumOfK+=k;
		arguments[i].kValue=k;
		arguments[i].hostInfo = hostFiles[i];
		arguments[i].genInfo = genFiles[i];
		arguments[i].tid = i;
		
		rc = pthread_create(&threads[i], NULL, jobSubmissionThreaded, (void *) &arguments[i]);
		i++;
	}
	
	output[i].close();
	
	for(int i=0;i<numOfSites;i++)	{
		pthread_join(threads[i], NULL);
	}
	
	
	return 0;
}


