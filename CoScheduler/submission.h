//
//  submission.h
//  CoScheduler
//
//  Created by Kartik Vedalaveni on 4/26/13.
//  Copyright (c) 2013 Kartik Vedalaveni. All rights reserved.
//

//Interfaces talking to condor scheduler

#ifndef CoScheduler_submission_h
#define CoScheduler_submission_h
#include <map>
#include <list>
#include <iostream>
#include "threaded.h"
#define MAXSITES 10
using namespace std;

map<string,unsigned long int> execute_times[MAXSITES];
map<string,unsigned long int> job_term_times[MAXSITES];
map<string,unsigned long int> wallClockTimes[MAXSITES];
list<unsigned long int> wallClockList[MAXSITES];
pthread_mutex_t mymutex1 = PTHREAD_MUTEX_INITIALIZER;
int return1=0;

extern ofstream output[MAXSITES];
extern unsigned long int maxJob;
class submission	{
	
	int N;
public:
	int count;
	
public: submission(int k)	{
	N=k;
	count=1; //count of jobs starts from 1
}
	
	int submit(string geninfoScript, args *data)	{
		char tmp[100];
        
        if(N==0 || N>maxJob)    {
            
            output[data->tid]<<"Done! Exiting!"<<endl<<flush;
            pthread_exit(&return1);
            
        }
		sprintf(tmp,"./%s %d > %s.script",geninfoScript.c_str(),N,geninfoScript.c_str());
		system(tmp);
		mywait(3);
        sprintf(tmp,"condor_submit %s.script",geninfoScript.c_str());
		system(tmp);
		
        output[data->tid]<<N<<" Job(s) Submitted!"<<endl<<flush;
        
		return 0;
	}
	
    
	void wait_calculate(string hostFile, args *data, int nSites)	{
		
		FILE *fp;
		int return_value=99;
        
		int avgRuntime=0;
		fp=fopen(hostFile.c_str(), "r");
		ReadUserLog reader(fp,false,false);
		ULogEvent *event = NULL;
		
		if(reader.isInitialized())	{
			cout<<"Initialized reading log file\n"<<flush;
			output[data->tid]<<"Initialized reading log file\n"<<flush;
		}
		else{
			cout<<"ERROR! reading log file host.log"<<flush;
		}
        
        char tmp[100];
        sprintf(tmp,"condor_wait -num 1 %s",hostFile.c_str());
        return_value=system(tmp);
        output[data->tid]<<"1 Job(s) Done!"<<endl<<flush;
        mywait(5);
        
        while(reader.readEvent(event)==ULOG_OK)	{
            
            ostringstream cluster_proc;
            
            if((*event).eventNumber==ULOG_EXECUTE )	{
                
                
                ExecuteEvent *exec = static_cast<ExecuteEvent*>(event);
                
                //	cout<<"Cluster proc: "<<endl;
                cluster_proc<<exec->cluster<<"."<<exec->proc<<"."<<exec->subproc;
                //	cout<<cluster_proc.str();
                //	cout<<"Event time: "<<endl;
                struct tm *tmp=&exec->eventTime;
                //cout<<timegm(tmp)<<endl;
                time_t epoch_time = timegm(tmp);
                
                for(int t=0;t<nSites;t++)   {
                    
                    if(t==data->tid)    {
                        
                        // Pre-store all events that are ULOG_EXECUTE and store time/date of execution in a map.
                        execute_times[t].insert(std::pair<string,unsigned long int>(cluster_proc.str(),epoch_time));
                        
                    }
                }
                mywait(2);
                
                count++;
                //condor_wait -num K, where K is the amount of jobs completed till the wait.
                if(count<=N)	{
                    char tmp[100];
                    sprintf(tmp,"condor_wait -num %d %s",count,hostFile.c_str());
                    return_value=system(tmp);
                    output[data->tid]<<count<<" Job(s) Done!"<<endl<<flush;
                    mywait(2);
                }
                
                
                continue;
            }
            
            if((*event).eventNumber==ULOG_JOB_TERMINATED)	{
                
                JobTerminatedEvent *term = static_cast<JobTerminatedEvent*>(event);
                
                if(term->normal)	{
                    unsigned long total_time=term->run_remote_rusage.ru_utime.tv_sec+term->run_remote_rusage.ru_stime.tv_sec +
                    term->run_local_rusage.ru_utime.tv_sec+term->run_local_rusage.ru_stime.tv_sec;
                    
                    struct tm *tmpTime = &term->eventTime;
                    
                    time_t epoch_time_term = timegm(tmpTime);
                    ostringstream cluster_proc_s;
                    cluster_proc_s<<term->cluster<<"."<<term->proc<<"."<<term->subproc;
                    
                    for(int t=0;t<nSites;t++)   {
                        
                        if(t==data->tid)    {
                            
                            
                            job_term_times[t].insert(std::pair<string,unsigned long int> (cluster_proc_s.str(),epoch_time_term));
                            
                        }
                    }
                    avgRuntime+=total_time;
                    //	output<<"Total time of job("<<count<<") is "<<total_time<<endl<<flush;
                    
                }
                
            }
        }
        
		fclose(fp);
	}
	unsigned long int calculateWallClock(int nSites, args *data)	{
		
		unsigned long int avg=0;
		unsigned long int wallClock=0;
        int t=data->tid;
        
		map<string,unsigned long int>::iterator test;
        for(std::map<string,unsigned long int>::iterator it=execute_times[t].begin(); it!=execute_times[t].end(); ++it)
        {
            //cout << x.first<<" : "<<x.second;
            
            test = job_term_times[t].find(it->first);
            
            if(test!=job_term_times[t].end())	{
                wallClock=job_term_times[t].at(it->first) - it->second;
                wallClockTimes[t].insert(std::pair<string,unsigned long int> (it->first,wallClock));
                avg+=wallClock;
            }
            
            
            cout<<"WallClock: "<<wallClock<<endl;
            output[data->tid]<<"WallClock: "<<wallClock<<endl;
            wallClockList[t].push_back(wallClock);
		}
		if((job_term_times[t].size())!=0)
			return avg/(job_term_times[t].size());
	}
	
};


#endif
