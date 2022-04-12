#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<math.h>
#include<time.h>
#include<wait.h>
#include<string.h>
pid_t coordinator_pid=0;
int sensors_nb=0;
int child1=-1,child2=-1;
float temp=0,avg=0;
int count=0;
int t[2],p[2],r[2],st[2],mess[2];
int level;
pid_t *sen;
char type;
time_t c1StartTime=0,c2StartTime=0,c1EndTime=0,c2EndTime=0;

void receive()
{
	if(getpid()==coordinator_pid)
	{
		pid_t sid=0;
		read(p[0],&sid,sizeof(pid_t));
		read(t[0],&temp,sizeof(float));
		avg+=temp;
		printf("coordinator pid : %d received temp : %f of %d\n",getpid(),temp,sid);
		count++;
		if(count==sensors_nb)
		{
			avg/=sensors_nb;
			if(avg<30 && avg>-30)
			{
				kill(child1,SIGUSR1);
				kill(child2,SIGUSR1);
			} 
			else
			{
				char res[]="ALERT";
				for(int i=0;i<sensors_nb;i++)
				{
					write(r[1],res,5);
				}	
			}
			count=0;
			avg=0;
		}
	}
	else if(type=='r')
	{
		pid_t sid=0;
		read(p[0],&sid,sizeof(pid_t));
		read(t[0],&temp,sizeof(float));
		if(sid==child1)
		{
			c1EndTime=time(NULL);
			int temps=c1EndTime-c1StartTime;
			if(temps>25)
			{
				write(p[1],&child1,sizeof(pid_t));
				kill(coordinator_pid,SIGBUS);
				sensors_nb--;
			}
			c1StartTime=time(NULL);
		}
		else if(sid==child2)
		{
			c2EndTime=time(NULL);
			int temps=c2EndTime-c2StartTime;
			if(temps>25)
			{
				write(p[1],&child2,sizeof(pid_t));
				kill(coordinator_pid,SIGBUS);
				sensors_nb--;
			}
			c2StartTime=time(NULL);
		}
		printf("router pid : %d received temp : %f of %d\n",getpid(),temp,sid);
		write(p[1],&sid,sizeof(pid_t));
		write(t[1],&temp,sizeof(float));
		kill(getppid(),SIGPIPE);
		printf("router pid : %d sends temp : %f to %d\n",getpid(),temp,getppid());
	}
}

int max(int sensors_nb)
{
	int max=1;
	while(max<sensors_nb)
		max*=2;
	return max;	
}

int levels(int sensors_nb)
{
	int x=1;
	int count=0;
	while(x!=max(sensors_nb))
	{
		x*=2;
		count++;
	}
	return count;
}

void Menu()
{
	if(type=='c'){
	printf("\nEnter 1 to largest path\n");
	printf("Enter 2 to send msgs between routers\n");
	int ans;
	scanf("%d",&ans);
	if(ans==1)
	{
		if(type=='c')
		{
			printf("\n%d\n",getpid());
			if(child1!=0)
			{
				kill(child1,SIGABRT);
			}
		}
	}
	else if(ans==2)
	{
		printf("Enter the pid of the router\n");
		int pid;
		scanf("%d",&pid);
		kill(pid,SIGTRAP);
	}}
}

void Path()
{
	printf("%d\n",getpid());
	if(child1!=0)
	{
		kill(child1,SIGABRT);
	}
}

void stopProcess()
{
	if(type=='c')
	{
		printf("Enter the pid of the sensor : ");
		int pid;
		scanf("%d",&pid);
		printf("Enter the time : ");
		int time;
		scanf("%d",&time);
		write(st[1],&time,sizeof(int));
		kill(pid,SIGUSR2);
	}
}

void stopFor()
{
	if(type=='s')
	{
		int time;
		read(st[0],&time,sizeof(int));
		printf("sensor %d stoped for %d s\n",getpid(),time);
		for(int i=0;i<time;i++)
		{
			sleep(1);
		}
		printf("sensor %d resumed\n",getpid());
	}
}

void childTerm()
{
	if(type=='c')
	{
		sensors_nb--;
		pid_t sid;
		read(p[0],&sid,sizeof(pid_t));
		printf("coordinator : sensor %d terminated\n",sid);
		kill(sid,SIGKILL);
	}
}

void coordinator()
{
	signal(SIGPIPE,receive);
	signal(SIGBUS,childTerm);
	printf("coordinator pid : %d started\n",getpid());
	if(child1!=-1)
	{
		waitpid(child1,NULL,0);
	}
	if(child2!=-1)
	{
		waitpid(child2,NULL,0);
	}
}

void result()
{
	char res[]="ACK  ";
	for(int i=0;i<sensors_nb;i++)
	{
		write(r[1],res,5);
	}
}

void recMsg()
{
	char m[100];
	read(mess[0],m,100);
	printf("router %d received : %s\n",getpid(),m);
}

void sendMsg()
{
	if(type=='r')
	{
		printf("router %d,Enter the pid of the receiver router\n",getpid());
		int pid;
		scanf("%d",&pid);
		printf("router %d,Enter the msg\n",getpid());
		char m[100];
		scanf("%s",m);
		write(mess[1],m,100);
		kill(pid,SIGURG);
	}
}

void router()
{
	signal(SIGURG,recMsg);
	signal(SIGTRAP,sendMsg);
	signal(SIGUSR1,result);
	signal(SIGPIPE,receive);
	printf("router pid : %d started\n",getpid());
	if(child1!=-1)
	{
		waitpid(child1,NULL,0);
	}
	if(child2!=-1)
	{
		waitpid(child2,NULL,0);
	}
}

int getTemperature()
{	
	srand(getpid()*rand());
	return rand()%41-rand()%41;
}

int nextTemp()
{
	srand(getpid()*rand());
	return rand()%30+1;
}

void newTemp()
{
	temp*=count;
	temp+=getTemperature();
	count++;
	temp/=count;
	printf("sensor pid : %d updates his temp=%f\n",getpid(),temp);
	int time=nextTemp();
	alarm(time);
}

void sensor(){
	
	signal(SIGALRM,newTemp);
	char res[5];
	newTemp();
	alarm(2);
	while(1){
		sleep(1);
		pid_t sid=getpid();
		write(t[1],&temp,sizeof(float));
		write(p[1],&sid,sizeof(pid_t));
		kill(getppid(),SIGPIPE);
		printf("sensor pid : %d sends temp=%f to pid : %d\n",getpid(),temp,getppid());
		read(r[0],&res,5);
		printf("sensor pid : %d received %s\n",getpid(),res);
		for(int i=0;i<19;i++)
		{
			sleep(1);
		}
	}
}


void make_routers_sensors(int sensors_nb)
{
	if(sensors_nb==0)
	{
		return;
	}
	else if(sensors_nb==1)
	{
		child1=fork();
		if(child1==0)
		{
			type='s';
			printf("sensor pid : %d started\n",getpid());
			sensor();
		}
		else
		{
			if(getpid()==coordinator_pid)
			{
				type='c';
				coordinator();
			}
			else
			{
				type='r';
				router();
			}
		}
	}
	else if(sensors_nb==2)
	{	
		
		child1=fork();
		if(child1==0)
		{
			type='s';
			printf("sensor pid : %d started\n",getpid());
			sensor();
		}
		else
		{
			child2=fork();
			if(child2==0)
			{
				type='s';
				printf("sensor pid : %d started\n",getpid());
				sensor();
			}
			else
			{
				if(getpid()==coordinator_pid)
				{	
					type='c';
					coordinator();
				}
				else
				{
					type='r';
					router();
				}
			}
		}
	}
	else
	{
		child1=fork();
		if(child1==0)
		{
			sensors_nb=max(sensors_nb)/2;
			make_routers_sensors(sensors_nb);
		}
		else
		{
			child2=fork();
			if(child2==0)
			{
				sensors_nb=sensors_nb-max(sensors_nb)/2;
				make_routers_sensors(sensors_nb);		
			}
			else
			{
				if(getpid()==coordinator_pid)
				{
					type='c';
					coordinator();
				}
				else
				{
					type='r';
					router();
				}
			}
		}	
	}
}

int main()
{
	c1StartTime=c2StartTime=time(NULL);
	signal(SIGQUIT,stopProcess);
	signal(SIGUSR2,stopFor);
	signal(SIGINT,Menu);
	signal(SIGABRT,Path);
	pipe(t),pipe(p),pipe(r),pipe(st),pipe(mess);
	coordinator_pid=getpid();
	printf("Use ^c to know the largest path\n");
	printf("Use ^\\ to stop a process for a limited time\n");
	printf("Enter the number of sensors : ");
	scanf("%d",&sensors_nb);
	make_routers_sensors(sensors_nb);
	return 0;
}
