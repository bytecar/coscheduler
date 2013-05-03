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
#define Q_WAIT_TIME 900
#define MAX 200
using namespace std;

unsigned long int T1=0,T2=0,sumOfK=0,maxJob=0, bestK=0;
float c1=2,c2=1.25;

ofstream output;

//Job submission primitive
unsigned long int jobSubmission(int k)	{
	
	unsigned long int T=0;
	int returnValue=99;
	//Clear host.log
	ofstream clear1("host.log",ios::trunc);
	clear1.close();
	sleep(5);
	
	submission s1(k);
	returnValue=s1.submit();
	sleep(5); //Wait 5 seconds for log file to be written before its read.
	if(returnValue==0)	{
		s1.wait_calculate();
		T=s1.calculateWallClock();
	}
	else{
		cout<<"Error completing job! Exiting!";
		output<<"Error completing job! Exiting!";
		exit(1);
	}
	
	sleep(5);
	job_term_times.clear();
	execute_times.clear();
	wallClockTimes.clear();
	
	cout<<"T(average) "<<T<<" over "<<s1.count-1<<" Jobs"<<endl<<flush;
	output<<"T(average) "<<T<<" over "<<s1.count-1<<" Jobs"<<endl<<flush;
	
	//Clear host.log
	ofstream clear("host.log",ios::trunc);
	clear.close();
	sleep(5);
	
	return T;
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

void lastIteration(int c1, int k)	{
	//Last Iteration, Submit remaining jobs to complete exact set of input jobs.
	if((sumOfK+k) > maxJob)	{
		k = maxJob-sumOfK;
		jobSubmission(k);
		cout<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
		output<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
		exit(0);
	}
}


//Best k for a given range around degraded value
int optimalRun(int low, int high)	{
	
	int mid = (low+high)/2;
	unsigned long int tmp=0;
	
	if(maxJobLimit(mid))	{
		sumOfK+=mid;
		tmp = jobSubmission(mid);
		
		T2=tmp;
		cout<<"\nJobs Completed: "<<sumOfK<<endl;
		cout<<"Jobs Remaining: "<<(maxJob-sumOfK)<<endl;
		
		output<<"\nJobs Completed: "<<sumOfK<<endl;
		output<<"Jobs Remaining: "<<(maxJob-sumOfK)<<endl;
		
		if(tmp<c2*T1)	{
			T1=(T1+tmp)/2;
			if(low+2==high)	{
				bestK = mid;
				return(mid);
			}
			optimalRun(mid, high);
		}
		else{
			cout<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			output<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			
			if(low+2==high)	{
				
				//If the final value detected by binary search is degraded value, we choose a lower value
				bestK=mid-1;
				return(mid-1);
			}
			optimalRun(low, mid);
		}
	}
	else{
		lastIteration(c1,mid);
		return mid;
	}
	
}




int main(int argc, char **argv)      {
	
	//make sure count is zero here, for control flow purpose
	int k=1,degradation_high=1,degradation_low=1;
	unsigned long int tmp=0;
	maxJob = atoi(argv[1]);
	
	output.open("output.txt", ofstream::out);
	
	//Submit and wait
	sumOfK+=k;
	T1=jobSubmission(k);
	
	
    while(true)
	{
		cout.flush();
		
		//Normal execution, control block
		if(T2 < c2*T1)	{
			
			k=c1*k; // If there is no degradation increase number of jobs submitted
			
			sumOfK+=k;
			T2=jobSubmission(k);
			
			cout<<"\nJobs Completed: "<<sumOfK<<endl;
			cout<<"Jobs Remaining: "<<(atoi(argv[1])-sumOfK)<<endl;
			
			output<<"\nJobs Completed: "<<sumOfK<<endl;
			output<<"Jobs Remaining: "<<(atoi(argv[1])-sumOfK)<<endl;
			
			if(T2< c2*T1){
				T1=(T1+T2)/2;
			}
		}
		
		
		//Degradation control block
		if(T2> c2*T1){
			
			cout<<"T1: "<<T1<<"\tT2: "<<T2<<flush;
			output<<"T1: "<<T1<<"\tT2: "<<T2<<flush;
			
			degradation_high=k;
			degradation_low =k/c1;
			
			cout<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			output<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			
			
			//Find the optimal K for the
			if((degradation_high-degradation_low)>0)
				optimalRun(degradation_low,degradation_high);
			
			k=bestK;
			unsigned int time=0;
			
			while(time<10*T1)	{
				
				
				if(maxJobLimit(k))	{
					wallClockList.clear();
					sumOfK+=k;
					T2=jobSubmission(k);
					
					wallClockList.sort();
					time += wallClockList.back();
					
					if(time>10*T1)
						time=10*T1;
					
					cout<<"\nJobs Completed: "<<sumOfK<<endl;
					cout<<"Jobs Remaining: "<<(atoi(argv[1])-sumOfK)<<endl;
					cout<<"\n\nSeconds till search resumes:  "<<(10*T1-time)<<endl<<flush;
					
					output<<"\nJobs Completed: "<<sumOfK<<endl;
					output<<"Jobs Remaining: "<<(atoi(argv[1])-sumOfK)<<endl;
					output<<"\n\nSeconds till search resumes:  "<<(10*T1-time)<<endl<<flush;
					
				}
				else{
					lastIteration(c1,k);
				}
				
			}
			
		}
		
		
		
	}
	
	output.close();
	return 0;
}



