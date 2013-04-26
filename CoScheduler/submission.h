//
//  submission.h
//  CoScheduler
//
//  Created by Kartik Vedalaveni on 4/26/13.
//  Copyright (c) 2013 Kartik Vedalaveni. All rights reserved.
//

#ifndef CoScheduler_submission_h
#define CoScheduler_submission_h
#include <map>
#include <iostream>
using namespace std;

map<string,unsigned long int> execute_times;
map<string,unsigned long int> job_term_times;
map<string,unsigned long int> wallClockTimes;

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
                
                if((*event).eventNumber==ULOG_EXECUTE || (*event).eventNumber==ULOG_JOB_TERMINATED)	{
                    
                    //return_value=system("condor_wait -num 1 host.log");
                    //sleep(5);
                    
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

                    if((*event).eventNumber==ULOG_EXECUTE)   {
                        sleep(2);
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
                            job_term_times.insert(std::pair<string,unsigned long int> (cluster_proc_s.str(),epoch_time_term));
                            
                            avgRuntime+=total_time;
                            //	output<<"Total time of job("<<count<<") is "<<total_time<<endl<<flush;
                            
                            //condor_wait -num K, where K is the amount of jobs completed till the wait.
                            if(count<=N)	{
                                delete event;
                                char tmp[100];
                                sprintf(tmp,"condor_wait -num %d host.log",count);
                                return_value=system(tmp);
                                sleep(2);
                            }
                            count++;
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


#endif
