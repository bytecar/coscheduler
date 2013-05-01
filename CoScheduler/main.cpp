#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <fstream>
#include "user_log.c++.h"
#include <cmath>
#include <map>
#include <sstream>
#include "submission.h"
#define Q_WAIT_TIME 900
#define MAX 200
using namespace std;


int progression[MAX];
ofstream output;


int main(int argc, char **argv)      {
	
	//make sure count is zero here, for control flow purpose
	int k=3,count=0, returnValue=0, sumOfK=0, degradation=1, dCount=0;
	float c1=2,c2=1.20;
	unsigned long int T1=0,T2=0,tmp=0, oldT2=0;
	output.open("output.txt", std::ofstream::out | std::ofstream::app);
	
	//Submit and wait
	sumOfK+=k;
	
	//Clear host.log
	ofstream clear1("host.log",ios::trunc);
	clear1.close();
	sleep(5);
	
	submission s1(k);
	returnValue=s1.submit();
	sleep(5); //Wait 5 seconds for log file to be written before its read.
	if(returnValue==0)	{
		s1.wait_calculate();
		T1=s1.calculateWallClock();
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
	
	cout<<"T1(average) "<<T1<<" over "<<s1.count-1<<" Jobs"<<endl<<flush;
	output<<"T1(average) "<<T1<<" over "<<s1.count-1<<" Jobs"<<endl<<flush;
	
	//Clear host.log
	ofstream clear("host.log",ios::trunc);
	clear.close();
	sleep(5);
	
    while(1)
	{
		cout.flush();
		
		//to handle the initial case we include a control flow for count==0
		if(T2 < c2*T1 || count==0)	{
			
			k=c1*k; // If there is no degradation increase number of jobs submitted
			
			
			
			
			if(k==degradation)	{

				k = k/c1;
				if(k<1)
					k=1;
			}

			sumOfK+=k;
			submission s2(k);
			returnValue=s2.submit();
			sleep(5);
			if(returnValue==0)	{
				s2.wait_calculate();
				tmp=s2.calculateWallClock();
				sleep(5);
			}
			else{
				cout<<"Error completing job! Exciting!";
				output<<"Error completing job! Exciting!";
				exit(2);
			}
			
			job_term_times.clear();
			execute_times.clear();
			wallClockTimes.clear();

			cout<<"T2(average) "<<tmp<<" over "<<s2.count-1<<" Jobs"<<endl<<flush;
			
			//Clear host.log
			ofstream clear1("host.log",ios::trunc);
			clear1.close();
			
			sleep(5);

			//After every two rounds of run, calculate global average time T1
			//T2=tmp;
			//T1=(T1+tmp)/2;
			
			cout<<"\nJobs Completed: "<<sumOfK<<endl;
			cout<<"Jobs Remaining: "<<(atoi(argv[1])-sumOfK)<<endl;


			//Think a logic for degradation and average T1
			if(count!=0)	{
								
					T1=(T1+T2)/2;
			}

			if(tmp < c2*T1)
				T2=tmp;
			else{
				T2=T1;
			}
			
			count++;
		}
		
		if(T2> c2*T1){
			
			cout<<"T1: "<<T1<<"\tT2: "<<T2<<flush;
			
			degradation=k;
				k =  k/(c1*c1);
				
			if(k<1)	{
				k=1;
			}
				count=0;
				
				cout<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
				output<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			
			dCount++;
						
		}
		
		
		if(dCount==5)	{
			degradation=0;
			dCount=0;
		}
		
		
		if((sumOfK+c1*k) > atoi(argv[1]))	{

			k = atoi(argv[1])-sumOfK;
			submission s3(k);
			returnValue=s3.submit();
			sleep(5);
			if(returnValue==0)	{
				s3.wait_calculate();
				tmp=s3.calculateWallClock();
			}
			sleep(5);

			job_term_times.clear();
			execute_times.clear();
			wallClockTimes.clear();

			cout<<"T2(average) "<<tmp<<" over "<<k<<" Jobs"<<endl<<flush;
			output<<"T2(average) "<<tmp<<" over "<<k<<" Jobs"<<endl<<flush;
			
			cout<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
			output<<"\n\n\nJobs Completed!" <<"Count of jobs: "<<sumOfK+k<<endl<<flush;
			exit(0);
		}
	}
	
	output.close();
	return 0;
}
