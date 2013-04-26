#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <fstream>
#include "user_log.c++.h"
#include <cmath>
#include <map>
#include <sstream>
#define Q_WAIT_TIME 900
#define MAX 200
using namespace std;


int progression[MAX];
ofstream output;

map<string,unsigned long int> execute_times;
map<string,unsigned long int> job_term_times;
map<string,unsigned long int> wallClockTimes;
/*
 * Long time jobs(struck), needs to be held and released to restart them
 *
 *
 *
 */
 

class submission	{
	
	int N;
public:
	int count;
	
public: submission(int k)	{
	N=k;
	count=1; //count of jobs starts from 1
}
	
	int submit()	{
		char tmp[100];
		sprintf(tmp,"./genCondorScript %d > condor.script",N);
		system(tmp);
		sleep(1);
		system("condor_submit condor.script");
		
		return 0;
	}
	

	int wait_calculate()	{
		
		FILE *fp;
		int return_value=99;
	
		int avgRuntime=0;
		fp=fopen("host.log", "r");
		ReadUserLog reader(fp,false,false);
		ULogEvent *event = NULL;
		
		if(reader.isInitialized())	{
			cout<<"Initialized reading log file\n"<<flush;
		}
		else{
			cout<<"ERROR! reading log file host.log"<<flush;
		}
								
		while (1) {
		
		if(reader.readEvent(event)==ULOG_OK)	{
			
			ostringstream cluster_proc;
			
			if((*event).eventNumber==ULOG_EXECUTE)	{
				
				return_value=system("condor_wait -num 1 host.log");
				sleep(5);
				
				ExecuteEvent *exec = static_cast<ExecuteEvent*>(event);
				
			//	cout<<"Cluster proc: "<<endl;
				cluster_proc<<exec->cluster<<"."<<exec->proc<<"."<<exec->subproc;
			//	cout<<cluster_proc.str();
			//	cout<<"Event time: "<<endl;
				struct tm *tmp=&exec->eventTime;
				//cout<<timegm(tmp)<<endl;
				time_t epoch_time = timegm(tmp);
				
			//	cout<<epoch_time<<endl;
				// Pre-store all events that are ULOG_EXECUTE and store time/date of execution in a map.
				execute_times.insert(std::pair<string,unsigned long int>(cluster_proc.str(),epoch_time));
				
				sleep(2);
				continue;
							
				if((*event).eventNumber==ULOG_JOB_TERMINATED)	{
					
					JobTerminatedEvent *term = static_cast<JobTerminatedEvent*>(event);
					
					if(term->normal)	{
						unsigned long total_time=term->run_remote_rusage.ru_utime.tv_sec+term->run_remote_rusage.ru_stime.tv_sec +
						term->run_local_rusage.ru_utime.tv_sec+term->run_local_rusage.ru_stime.tv_sec;
						
						struct tm *tmpTime = &term->eventTime;
						
						time_t epoch_time_term = timegm(tmpTime);
						ostringstream cluster_proc_s;
						cluster_proc_s<<term->cluster<<"."<<term->proc<<"."<<term->subproc;
						job_term_times.insert(std::pair<string,unsigned long int> (cluster_proc_s.str(),epoch_time_term));
						
						avgRuntime+=total_time;
						//	output<<"Total time of job("<<count<<") is "<<total_time<<endl<<flush;
						count++;
						
						//condor_wait -num K, where K is the amount of jobs completed till the wait.
						if(count<=N)	{
							delete event;
							char tmp[100];
							sprintf(tmp,"condor_wait -num %d host.log",count);
							return_value=system(tmp);
							sleep(2);
						}
					}
					
				}
			}
			
		}
		
		else {
			return 0;
		}
			
	}
		fclose(fp);	
	}
	unsigned long int calculateWallClock()	{
		
		unsigned long int avg=0;
		unsigned long int wallClock=0;
		map<string,unsigned long int>::iterator test;
		for(std::map<string,unsigned long int>::iterator it=execute_times.begin(); it!=execute_times.end(); ++it)
		{
			//cout << x.first<<" : "<<x.second;
			
			test = job_term_times.find(it->first);
			
			if(test!=job_term_times.end())	{
			wallClock=job_term_times.at(it->first) - it->second;
			wallClockTimes.insert(std::pair<string,unsigned long int> (it->first,wallClock));

			avg+=wallClock;
			}
			
			cout<<"WallClock: "<<wallClock<<endl;
		}
		
		
		if((job_term_times.size())!=0)
			return avg/(job_term_times.size());
		
	}
	
};


//For QOS!
int arrayProgression(int r,int a,int sn)	{
		
	int i=0;
	int numberOfIterations=0, sum=0;
	for(int i=0;i<200;i++)	{
		progression[i] = 0;
	}
	
	//Calculate number of theoretically possible iterations, they're in GP
	numberOfIterations = floor(log10(((sn*(r-1)/a) +1)) / log10(r));
	progression[0] = a;
	sum+=a;
		for( i=1;i<numberOfIterations;i++)	{
			progression[i] = a * pow(r,i);
			sum+=progression[i];
		}
			
	progression[i] = sum;
	return i;
}

int main(int argc, char **argv)      {
	
	//make sure count is zero here, for control flow purpose
	int k=3,count=0, returnValue=0, sumOfK=0,size=0;
	float c1=2,c2=1.20;
	unsigned long int T1=0,T2=0,tmp=0;
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
			T2=tmp;
			//T1=(T1+tmp)/2;
			
			cout<<"\nJobs Completed: "<<sumOfK<<endl;
			cout<<"\nJobs Remaining: "<<(atoi(argv[1])-sumOfK)<<endl;
			count++;
		}
		
		if(T2> c2*T1){
			
			cout<<"T1: "<<T1<<"\tT2: "<<T2<<flush;
			
				k =  k/(c1*c1);
				count=0;
				
				cout<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
				output<<"\n***** Degradation detected! Reducing number of jobs. *****"<<endl<<flush;
			
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
