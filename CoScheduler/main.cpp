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
unsigned long int maxJob=0,sumOfK=0, T=0,ret1;
int numOfSites=0;
args arguments[MAXSITES];
resource stats[MAXSITES];
string gridSites[MAXSITES], genFiles[MAXSITES], hostFiles[MAXSITES];
ofstream output[MAXSITES];

int optimalRun(int,int,resource*);
bool maxJobLimit(int);
void lastIteration(int, int, resource*);
unsigned int jobSubmission(int,args *);


void capacityDetection(resource *data)	{
	
	int k = data->kValue;
	unsigned int T1 = data->T1;
	unsigned int T2 = data->T2;
	float c1 = data->c1;
	float c2 = data->c2;
	int thread_id = data->tid;
	int degradation_high = data->degradation_high;
	int degradation_low = data->degradation_low;
	
	while(true)
	{
		cout.flush();
		
		//Normal execution, control block
		if(T2 < c2*T1)	{
			
			k=c1*k;
			
			if(maxJobLimit(k))	{
				 // If there is no degradation increase number of jobs submitted
				
				arguments[thread_id].kValue=k;
				jobSubmission(k,&arguments[thread_id]);
				
				//taken care of T2 initialization in jobSubmission function
				T2 = data->T2;
				
				cout<<"\nJobs Completed: "<<sumOfK<<endl;
				cout<<"Jobs Remaining: "<<(maxJob-sumOfK)<<endl;
				
				output[thread_id]<<"\nJobs Completed (or in Progress) : "<<sumOfK<<endl;
				output[thread_id]<<"Jobs Remaining                    : "<<(maxJob-sumOfK)<<endl;
				
				if(T2< c2*T1){
					T1=(T1+T2)/2;
				}
			}
			else{
				lastIteration(c1, k, data);
			}
		}
		
		//Re-read T1's updated data
		data->T1 = T1;
		
		//Degradation control block
		if(T2> c2*T1){
			
			cout<<"T1: "<<T1<<"\tT2: "<<T2<<flush;
			output[thread_id]<<"T1: "<<T1<<"\tT2: "<<T2<<flush;
			
			degradation_high=k;
			data->degradation_high = k;
			
			degradation_low =k/c1;
			data->degradation_low = k/c1;
			
			cout<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			output[thread_id]<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			
			
			//Find the optimal K for the
			if((degradation_high-degradation_low)>0)
				optimalRun(degradation_low,degradation_high, data);
			
			//Get best value here
			k=data->bestKValue;
			unsigned int time=0;
			
			while(time<10*T1)	{
				
				if(maxJobLimit(k))	{
					wallClockList[thread_id].clear();
					arguments[thread_id].kValue=k;
					jobSubmission(k,&arguments[thread_id]);
					
					T2 = data->T2;
					
					wallClockList[thread_id].sort();
					time += wallClockList[thread_id].back();
					
					if(time>10*T1)
						time=10*T1;
					
					cout<<"\nJobs Completed(or In progress): "<<sumOfK<<endl;
					cout<<"Jobs Remaining                  : "<<maxJob-sumOfK<<endl;
					cout<<"\n\nSeconds till search resumes :  "<<(10*T1-time)<<endl<<flush;
					
					output[thread_id]<<"\nJobs Completed (or In progress): "<<sumOfK<<endl;
					output[thread_id]<<"Jobs Remaining                   : "<<maxJob-sumOfK<<endl;
					output[thread_id]<<"\n\nSeconds till search resumes  :  "<<(10*T1-time)<<endl<<flush;
					
				}
				else{
					lastIteration(c1,k,data);
				}
				
			}
			
		}
		
		
		
	}
	
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
	
	submission s1(k);
	returnValue=s1.submit(geninfo, data);
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
		cout<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
		output[data->tid]<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
		pthread_exit(&ret1);
	}
}


//Best k for a given range around degraded value
int optimalRun(int low, int high,resource *data)	{
	
	int mid = (low+high)/2;
	unsigned long int tmp=0;
	
	if(maxJobLimit(mid))	{
		/*pthread_mutex_lock (&mymutex);
		 sumOfK+=mid;
		 pthread_mutex_unlock (&mymutex);*/
		
		arguments[data->tid].kValue=mid;
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
	
	
	//Check "sites" file and create that many hosts
	ifstream sites("sites");
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
		ifstream src("submitScript");
		sprintf(genScript,"genScript%d",i);
		sprintf(hostFile,"host%d.log",i);
		genFiles[i] = genScript;
		hostFiles[i] = hostFile;
		ofstream dst(genScript,ios::out);
		
	    dst<<src.rdbuf();
		dst<<"echo \"grid_resource=gt5 "<<gridSites[i]<<"\""<<endl;
		dst<<"echo \"log= "<<hostFile<<"\""<<endl;
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
	
	for(int i=0;i<numOfSites;i++)	{
		pthread_join(threads[i], NULL);
		output[i].close();
		
	}
	
	
	return 0;
}


