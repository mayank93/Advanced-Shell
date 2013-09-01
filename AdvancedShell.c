#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<dirent.h>
#include<sys/types.h>
#include<unistd.h>
#include<errno.h>
#include<signal.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
struct proc_list{
	int pid;
	int is_running;
	char name[100];
};
int proc_count=0;  // count of process 
int background_process; // stores if process is background or foreground
char *home;
int no_arg=0;   // no of args in a command
int no_cmd=0;   // no of cmd in a statement
struct proc_list process[100]; // stores all the process & their corresponding pid
char **in_file,**out_file;
int in=0,out=0; // stores no of input & output file

//-----------------------------------------------------------------------------------------------------------------------

void print_promt()
{
	char *pwd,*host,*user;
	pwd=(char *)malloc(sizeof(char)*100); // gives present working dir
	host=(char *)malloc(sizeof(char)*100); // gives host name
	user=(char *)malloc(sizeof(char)*100); // gives currently logged in user
	if(getlogin_r(user,100)){
		printf("Get login method failed. Exiting\n");
		exit(1);
	}
	if(gethostname(host,100)){
		printf("Get host name method failed. Exiting\n");
		exit(1);
	}
	if(getcwd(pwd,100)==NULL){
		printf("Get current working directory method failed. Exiting\n");
		exit(1);
	}
	char*temp;
	int len_home=strlen(home);
	temp=strdup(pwd);
	if(strncmp(pwd,home,len_home)==0)  // check if pwd is home if it contain home replace it by "~"
	{
		pwd=strcpy(pwd,"~");
		pwd=strcat(pwd,&temp[len_home]);
	}
	fflush(stdout);
	printf("<%s@%s:%s>",user,host,pwd); // printing required prompt
	fflush(stdout);
	return;

}

//---------------------------------------------------------------------

int split_command(char **cmd,char *delim,char*str_tmp)
{
	// split command according to delimeter
	char *token;
	int count=0;
	token = strtok(str_tmp,delim);
	while(token != NULL){
		cmd[count]=strdup(token);
		token = strtok(NULL,delim);
		count++;
	}
	return count;
}

//-------------------------------------------------------------------------------------------------------------------------------

void sigchld_handler_background(int sig){
	// handle bacground process
	pid_t pid;
	int status;
	pid = waitpid(-1,&status,WNOHANG);
	int i;
	for(i=0;process[i].pid!=pid && i<proc_count;i++);
	if(i<proc_count){
		process[i].is_running=0;
		fflush(stdout);
		if(WIFEXITED(status)!=0)
			printf("\n%s %d exited normally\n",process[i].name,process[i].pid);
		fflush(stdout);
		print_promt();
		fflush(stdout);
	}
	return;
}

//-----------------------------------------------------------------------------------------------------------------

void get_history(char **str,int n,char *str_temp)
{
	// user defined history command
	int i=0,len,arg,temp;
	len=strlen(str_temp);
	if(strncmp("hist",str_temp,4)==0){
		if(len==4){
			for(i=1;i<=n;i++)
				printf("%d. %s\n",i,str[i]);
		}
		else{
			arg=0;
			for(i=4;i<len;i++){
				temp=str_temp[i]-'0';
				fflush(stdout);
				if(temp>=0&&temp<=9)
					arg=arg*10+temp;
				else{
					printf("Invalid argument to hist\n");
					return;
				}}
			fflush(stdout);
			if(arg>n){
				for(i=1;i<=n;i++){
					printf("%d. %s\n",i,str[i]);
					fflush(stdout);
				}}
			else{
				int count=1;
				for(i=n-arg+1;i<=n;i++){
					printf("%d. %s\n",count,str[i]);
					fflush(stdout);
					count++;
				}}}}
	return;
}

//------------------------------------------------------------------------------------------------------------------------------

void sig_handler(int signum){ // handles signal Singint and Sigtstp to prevent quiting from ctrl-c & ctrl-z
	if(signum == 2 || signum == 20){
		fflush(stdout);
		printf("\n");
		print_promt();
	}   
	signal(SIGINT, sig_handler);
	signal(SIGTSTP, sig_handler);
	return;
}

//-----------------------------------------------------------------------------------------------------------------------

int main(int argc,char **argv)
{
	signal(SIGINT,  sig_handler);
	signal(SIGTSTP, sig_handler);
	int flag=0;
	char *tmp_path;
	home=(char *)malloc(sizeof(char)*100);
	tmp_path=(char *)malloc(sizeof(char)*100);   
	if(getcwd(home,100)==NULL){ // stores the location from where program was evoked
		printf("Get current working directory method failed. Exiting\n");
		exit(1);
	}
	strcpy(tmp_path,home);
	strcat(tmp_path,"/.tmp");                      // absolute path for ".tmp" file
	FILE *temp_pass=fopen(tmp_path,"w");
	if(temp_pass)
	{
		fprintf(temp_pass,"%d\n",flag);
		fclose(temp_pass);
	}
	signal(SIGCHLD,sigchld_handler_background);  // to catch sigchild signal
	char **str;
	str=(char **)malloc(sizeof(char*)*100);  // stores all command executed
	str[0]="\0";
	int N=0;
	proc_count=0;
	while(1)
	{
		background_process=0;
		char *str_tmp,*token,*temp_c,**cmd_full,**cmd_one;
		str_tmp=(char *)malloc(sizeof(char)*1024);  // stores input from user
		cmd_full=(char **)malloc(sizeof(char*)*100);
		cmd_one=(char **)malloc(sizeof(char*)*100);
		int x=0,i=0,ii=0;
		in=0; 
		out=0;
		int flag=0;
		temp_pass=fopen(tmp_path,"r");
		if(temp_pass)
		{
			fscanf(temp_pass,"%d",&flag);
			fclose(temp_pass);
		}
		//		print_promt();
		//		printf("\n");
		if (flag==0)
		{
			print_promt(); 
			fflush(stdout);  
			gets(str_tmp);  // get input from user if previous command was not "!histn"
		}
		else
		{
			//		printf("\n");
			strcpy(str_tmp,str[flag]);   // if previous command is !histn then copy the command pointed by it to str_tmp
			char *tt=strdup(str[N]);
			for(i=0;i<strlen(tt);i++)       // check if !histn has any argument
			{
				if(tt[i]==' ')
					break;
			}
			if(i!=strlen(tt))
				strcat(str_tmp,&tt[i]);         // if !histn contains any argument append at the end of str_tmp
			flag=0;                
			temp_pass=fopen(tmp_path,"w");
			if(temp_pass)
			{
				fprintf(temp_pass,"%d\n",flag);
				fclose(temp_pass);
			}
		}

		fflush(stdin);
		if(strlen(str_tmp)==0) // checks if previous input was null
			continue;
		// check previous cmd same as new one
		if(strcmp(str[N],str_tmp)!=0)
		{
			N++;
			str[N]=strdup(str_tmp);
		}
		no_cmd=split_command(cmd_full,"|",str_tmp);
		char *last=strdup(cmd_full[no_cmd-1]);
		int last_element=strlen(last);
		if(strcmp(cmd_full[0],"quit")==0)   // if input is quit exit from program
			exit(0);
		for(i=0;i<last_element;i++)
		{
			if(last[i]=='&'){               // check if process is background or foreground
				background_process=1;
				break;
			}}
		if(strncmp("cd",cmd_full[0],2)==0){   // user defined cd command
			x=split_command(cmd_one," \t",cmd_full[0]);  // split command seperated by space or tab
			//			printf("check %s\n",cmd_one[1]);
			if(x==1){
				if(chdir((const char*)home))
					printf("cd failed to execute\n");
			}
			else{
				if(chdir((const char*)cmd_one[1]))
					printf("Invaid path or argument to cd\n");
			}
			fflush(stdout);
		}
		else
		{
			int cmd_no=0, status;
			pid_t  pidp;
			int fd[no_cmd][2];

			pidp=fork();                          // forking to execute command in child
			if (pidp < 0){
				printf("ERROR: forking child process failed\n");
				exit(1);
			}
			else if(pidp==0){
				pid_t  pid[no_cmd];
				while(no_cmd){                        // run till all command seperated by pipe not executed
					int error=0,n1,i,n,out_f,in_f;
					if(pipe(fd[cmd_no])<0)        // calling pipe command to communicate between processes
					{
						perror("pipe error\n");
					}

					char **cmd,*cmd_tmp;
					cmd=(char **)malloc(sizeof(char*)*100);
					cmd_tmp=(char *)malloc(sizeof(char)*1024);
					in_file=(char **)malloc(sizeof(char*)*100);
					out_file=(char **)malloc(sizeof(char*)*100);
					in=0;
					out=0;
					n=strlen(cmd_full[cmd_no]);
					strcpy(cmd_tmp,cmd_full[cmd_no]);
					no_cmd--;
					x=split_command(cmd," \t",cmd_tmp);  // split command seperated by space or tab
					cmd[x]=NULL;                         // puting last element of array to be NULL

					// stores the name of all input and  file and replace "<" & ">"by NULL so execvp doesn't execute ahead of it
					for(ii=0;ii<x;ii++){                
						if(strcmp(cmd[ii],"<")==0){
							cmd[ii]=NULL;
							in_file[in]=cmd[ii+1];
							in++;
						}
						else if(strcmp(cmd[ii],">")==0){
							cmd[ii]=NULL;
							out_file[out]=cmd[ii+1];
							out++;
						}}			
					if(in>0){
						for(i=0;i<in-1;i++){
							in_f= open(in_file[i], O_RDONLY);   // emptying all file except the first
							if(in_f<0){
								perror("error in open in file");
								error=1;
								_exit(1);
								break;
							}
							close(in_f);
						}
						in_f = open(in_file[in-1], O_RDONLY);
						if(in_f<0){
							perror("error in open in file");
							error=1;
							_exit(1);
						}
						if(error!=0)
							_exit(1);
					}
					if(out>0){
						for(i=0;i<out-1;i++){
							out_f = open(out_file[i], O_WRONLY | O_CREAT,0664); // creating all files
							if(out_f<0)
							{
								perror("error in open in file");
								error=1;
								_exit(1);
							}
							close(out_f);
						}
						out_f = open(out_file[out-1], O_WRONLY|O_CREAT,0664);
						if(out_f<0)
						{
							perror("error in open in file");
							error=1;
							_exit(1);
						}
					}
					if(strcmp(cmd[x-1],"&")==0){
						background_process=1;
						cmd[x-1]=NULL;
					}
					int  status;
					// forking for each comand seperated pipe
					pid[cmd_no]=fork();
					if (pid[cmd_no] < 0){
						printf("ERROR: forking child process failed\n");
						exit(1);
					}
					else if (pid[cmd_no] == 0){   // if input from file replace stdin with file 
						if(in>0)		
							dup2(in_f, 0);
						else
						{
							if(cmd_no==0){
								close(fd[0][0]);
							}

							else{
								dup2(fd[cmd_no-1][0],0);
							}
						}
						if(out>0)        // if out is to be send to a file replace stdout with file
							dup2(out_f, 1);
						else{
							if(no_cmd==0)
							{
								close(fd[cmd_no][1]);
							}
							else{
								dup2(fd[cmd_no][1],1);
							}
						}
						for(i=0;i<=cmd_no;i++)	{
							close(fd[i][0]);
							close(fd[i][1]);
						}
						if(strncmp("hist",cmd[0],4)==0){         // user defined hist command
							if(x==1||cmd[1]==NULL)
								get_history(str,N,cmd[0]);
							else
								printf("Invalid arguments to hist\n");
						}
						else if (strncmp("!hist",cmd[0],5)==0){    // user defined !histn command
							int arg=0;
							int temp,len=strlen(cmd[0]);
							for(i=5;i<len;i++){   
								temp=cmd[0][i]-'0';
								if(temp>=0&&temp<=9)
									arg=arg*10+temp;
								else{   
									printf("Invalid arguments to hist\n");
									fflush(stdout);
									break;
								}}   
							if(arg>N)
								printf("argument exceded no. of commands in history\n");
							else{ 
								temp_pass=fopen(tmp_path,"w");  // saves the argument n of !histn in a tmp file
								if(temp_pass)
								{
									fprintf(temp_pass,"%d\n",arg);
									fclose(temp_pass);
								}
							}
							fflush(stdout);
						}
						else if(strcmp(cmd[0],"pid")==0){   // user defined pid command
							if(x==1){
								printf("command name: %s process id: %d\n",cmd[0],getpid());
							}	
							else{
								if(strcmp(cmd[1],"all")==0){
									printf("List of all process spawned by the shell\n");
									for(i=0;i<proc_count;i++){
										printf("command name: %s process id: %d\n",process[i].name,process[i].pid);
									}}
								else if(strcmp(cmd[1],"current")==0){
									printf("List of currently executing processes spawned by the shell\n");
									for(i=0;i<proc_count;i++){
										if(process[i].is_running){
											printf("command name: %s process id: %d\n",process[i].name,process[i].pid);
										}}}
								else{
									printf("Invaid argument to pid\n");
								}}
							fflush(stdout);
						}
						else{
							if (execvp(*cmd,cmd) < 0){    // run command which are not defined by user
								printf("%s: command not found\n",cmd[0]);
								//	perror("command not found");
								fflush(stdout);
								_exit(0);  // to kill a child if execvp fails
							}}
						_exit(0);  // to kill a child if execvp was not executed
					}
					else{
						if(no_cmd!=0){
							cmd_no++;
							continue;
						}
						for(i=0;i<=cmd_no;i++)	{  // close all pipes when all commands seperated by pipes had executed
							close(fd[i][0]);
							close(fd[i][1]);
						}
						for (i = 0; i <=cmd_no; i++)    //wait for all child processes created
							waitpid(pid[i], NULL, 0);
					}
					if(out>0)
						close(out_f);    // close file descriptor
					if(in>0)
						close(in_f);    // close file descriptor
					free(cmd);
					free(cmd_tmp);
					free(out_file);
					free(in_file);
					_exit(1);         // to kill a child
				}}

			else{
				process[proc_count].pid=pidp;     // stores pid of each prosess
				process[proc_count].is_running=1;   // initalize each process to running
				strcpy(process[proc_count].name,str[N]);  // stores each command 
				proc_count++;                             // no of command executed
				if(background_process==0){			// if not backgroung process wait
					signal(SIGCHLD,NULL);
					wait(NULL);
					signal(SIGCHLD,sigchld_handler_background);
					process[proc_count-1].is_running=0;
				}
				else                          // if background process print status and don't wait
					printf("command %s pid %d\n",process[proc_count-1].name,process[proc_count-1].pid);
			}}
		free(cmd_full);
		free(str_tmp);
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------------------
