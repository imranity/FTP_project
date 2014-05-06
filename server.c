//Header files needed
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <string.h> 
#include <dirent.h> 
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdlib.h>


#include <sys/ipc.h>
#include <sys/shm.h>


//Function declarations for server program
void InitializeMutex(pthread_mutex_t *pMutex); 	//Init. mutex for user
int DestroyMutex(pthread_mutex_t *pMutex);     	//pthread destroy mutex function
int LockMutex(pthread_mutex_t *pMutex);        	//pthread mutex locking 
int UnlockMutex(pthread_mutex_t *pMutex);      	//pthread unlocking mutex
int startServer();					  	//start server,define server socket variables
int runServer(int port);				  	//socket,bind,listen
int checkUser(int sock);				  	//to check if user is valid
int isUser(char *userName,char *userPassword); 	//check user based on uname,pwd
int clientOverFlow(int sock,int count);        	//clientMAX个 overflow
int get_clientThread();				  	//thread for each client
void back_clientThread(int i);			  	//backing thread for client
int get_port();					  	//function to get port
void back_port(int i);				  	//back,valid port
int get_path();					  	//path check for file
void back_path(int i);				  	//back,valid path
void int_to_stream(int a,char *b);		  	//integer to stream
int stream_to_int(char *b);			  	//stream conversion
void *createNewClientThread(char * clientThredInfor);	  
							  	//parameter based thread client
void makeUp_clientThreadInfor(char *a,char *b,char *c);
							  	//break the client thread 
void break_clientThreadInfor(char *a,char *b,char *c);
							  	//thread for command
void makeUp_cmdThreadInfor(char *a,char *b,char *c,char *d,char *e,char *f);
								//break cmd thread
void break_cmdThreadInfor(char *a,char *b,char *c,char *d,char *e,char *f);
								//based on socket created,com with client

int clientInteract(int msgSock);		 	
void get_arg(char *b,int a,char *c);			//client_cmd client_arg
void * cmd_pwd(char * cmdThreadInfor);			//pwd cmd
void * cmd_dir(char * cmdThreadInfor);			//dir cmd
void * cmd_cd(char * cmdThreadInfor);			//cd  cmd
void * cmd_cdBack(char * cmdThreadInfor);		//cd..cmd
void * cmd_get(char * cmdThreadInfor);			//Copy file from 
void * cmd_put(char * cmdThreadInfor);			//Copy file to
void * cmd_help(char * cmdThreadInfor);		//? help section
void * cmd_quit(char * cmdThreadInfor);		//quit
void * cmd_mkdir(char * cmdThreadInfor);		//mkdir makes dir
void * cmd_rmdir(char * cmdThreadInfor);        //rmdir
void * cmd_mget(char * cmdThreadInfor);         //mget multiple copies

void InitializeUsers();                         //all users info initialzd
void count_current();                           //count current users IPC based
void count_all();                               //all users counted
void list();
void kill_user();
void clean_up();
void quit();
//parameter definitions
#define clientListen 10
#define ftpPort 5666
#define clientMAX 2
#define portMAX 500
#define threadMAX 200
#define dataLen 1024
#define maxUser 3
//pthread mutex for client
pthread_mutex_t clientCounterMutex;            //pthread_mutex
int clientCounter = 0;
char buf[dataLen];
char help[]="#pwd:\tdisplay the current directory of client\n#dir:\tdisplay the files in the current directory of client\n#cd:\tchange the directory of client\n#cd..:\tback the upper level of directory of client\n#mkdir:\tcreate a folder in the directory of client\nget:\tdownload a file from server\nput:\tupload a file to server\npwd:\tdisplay the current directory of server\ndir:\tdisplay the files in the current directory of server\ncd:\tchange the directory of server\ncd..:\tback the upper level of directory of server\nmkdir:\tcreate a folder in the directory of server\n?:\tdisplay the whole command which equals 'help'\nquit:\texit\n";
int recv_succ,send_succ,exec_succ;
//int current=0;
int all=0;

//struct for client thread
struct clientThread{
	pthread_t client_thread;
	int valid;
}cThread[threadMAX];
struct portArray
{
	int port;
	int valid;
}port_array[portMAX];
struct serverFilePath{
	char currentFilePath[dataLen];
	int valid;
}filePath[clientMAX + 1];
struct user
{	
	char userName[50];
	char userPassword[50];
	int login;
	int msgSock;
}users[maxUser];

//Our Main function
int main()
{
	//initialze with all functions required at start
	memset(users,0,sizeof(struct user)*maxUser);
	memset(buf,0,dataLen);
	InitializeUsers();
	InitializeMutex(&clientCounterMutex);
	int i;
	for(i = 0;i < threadMAX;i ++)
		cThread[i].valid = 1;
	for(i = 0;i < portMAX;i ++)
	{
		port_array[i].port = 2000 + i;
		port_array[i].valid = 1;
	}
	for(i = 0;i < clientMAX + 1;i ++)
		filePath[i].valid = 1;
	
	printf("welcome to the ftp server!\n");
	count_current();//count current users
	all=0;
	count_all();
	list();
	clean_up();
	exec_succ = startServer();
	if(exec_succ == -1)
		printf("fail at startServer()...\n");
	DestroyMutex(&clientCounterMutex);
	count_current();
	list();
	return 1;
}


//mutex initialization
void InitializeMutex(pthread_mutex_t *pMutex){
	if(pMutex){
		pthread_mutex_init(pMutex,NULL);
	}
}

//destroy mutex
int DestroyMutex(pthread_mutex_t *pMutex){
	pthread_mutex_destroy(pMutex);
	return 0;
}

//lock mutex
int LockMutex(pthread_mutex_t *pMutex){
	if(pthread_mutex_lock(pMutex) != 0)
		return -1;
	else
		return 0;
}

//unlock mutex
int UnlockMutex(pthread_mutex_t *pMutex){
	if(pthread_mutex_unlock(pMutex) != 0)
		return -1;
	else
		return 0;
}

//start of server
int startServer(){
	
	int serverSocket = runServer(ftpPort);
	if(serverSocket == -1){
		printf("fail at runServer()...\n");
		return -1;
	}

	int msgSock;
	while(1){
printf("serverSocket=%d\n",serverSocket);
		printf("server launched.waiting for users...\n");
		int newClientThread_i;
		char msgSock_stream[5];
		char newClientThread_i_stream[5];
		char clientThreadInfor[10];
		

            pthread_t newThread;
		int kill_exec_succ=pthread_create(&newThread,NULL,kill_user,NULL);
		if(kill_exec_succ != 0){
			printf("fail at pthread_create()...\n");
			return -1;
		}

            pthread_t newThread_quit;
		int quit_exec_succ=pthread_create(&newThread_quit,NULL,quit,NULL);
		if(quit_exec_succ != 0){
			printf("fail at pthread_create()...\n");
			return -1;
		}

		msgSock = accept(serverSocket,(struct sockaddr*)0,(int*)0);
		printf("msgSock = %d\n",msgSock);
		if(msgSock == -1){
			printf("fail at accept()...\n");
			return -1;
		}
		int_to_stream(msgSock,msgSock_stream);
		printf("msgSock_stream = %s\n",msgSock_stream);
		
		//count_current();
		LockMutex(&clientCounterMutex);
		clientCounter ++;
		int isOverFlow = clientOverFlow(msgSock,clientCounter);
		clientCounter --;	
		if(isOverFlow == 0){			
			UnlockMutex(&clientCounterMutex);
			printf("there have been enough clients,request refused...!\n");
			close(msgSock);
			continue;
		}
		
		UnlockMutex(&clientCounterMutex);
		
		
		printf("main pid is %d\n",getpid());
		//new client thread 
		newClientThread_i = get_clientThread();
		printf("newClientThread_i = %d\n",newClientThread_i);
		if(newClientThread_i == -1){
			printf("fail at get_clientThread()...\n");
			return -1;
		}
		int_to_stream(newClientThread_i,newClientThread_i_stream);
		printf("newClientThread_i_stream = %s\n",newClientThread_i_stream);
		pthread_t newClientThread = cThread[newClientThread_i].client_thread;
		
		makeUp_clientThreadInfor(msgSock_stream,newClientThread_i_stream,clientThreadInfor);
		printf("clientThreadInfor %s\n",clientThreadInfor);
		
		exec_succ = pthread_create(&newClientThread,NULL,createNewClientThread,clientThreadInfor);
		if(exec_succ != 0){
			printf("fail at pthread_create()...\n");
			return -1;
		}
		
		

	}
	return 0;
}

//socket,bind,listen					  
int runServer(int port){
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock == -1){
		printf("fail at socket()...\n");
		return -1;
	}
	//int opt=1,ret;
	//ret = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	//if(ret == -1)
	//	ERR_EXIT("setsockopt");
	struct sockaddr_in tServerAddress;
	tServerAddress.sin_port = htons((unsigned short)port);
	tServerAddress.sin_family = AF_INET;
     // char serverAddress[30];
	//printf("Please input the ip of ftp-server:\n");
	//scanf("%s",serverAddress);
	tServerAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

	
	int bind_succ = bind(sock,(const struct sockaddr*)&tServerAddress,sizeof(struct sockaddr_in));
	if(bind_succ ==-1){
		printf("fail at bind()...\n");
		return -1;
	}
	
	int listen_succ = listen(sock,clientListen);
	if(listen_succ == -1){
		printf("fail at listen()...\n");
		return -1;
	}
	return sock;
}

//receive user info from client
int checkUser(int sock){
	char userName[dataLen],userPassword[dataLen];
	recv_succ = recv(sock,userName,dataLen,0);
	if(recv_succ == -1){
		printf("fail at recv()...\n");
		return -1;
	}
printf("received user name \n");
	recv_succ = recv(sock,userPassword,dataLen,0);
	if(recv_succ == -1){
		printf("fail at recv()...\n");
		return -1;
	}
printf("received user password \n");
	int is_user = isUser(userName,userPassword);
	if(is_user == -1){
		printf("fail at isUser()...\n");
		strcpy(buf,"-1");
		send_succ = send(sock,buf,dataLen,0);
		if(send_succ == -1){
			printf("fail at send()...\n");
			return -1;
		}
		return -1;
	}
	else
		if(is_user==-2)
		{
			printf("fail at isUser()...\n");
			strcpy(buf,"-2");
printf("buf is %s\n",buf);
			send_succ = send(sock,buf,dataLen,0);
			if(send_succ == -1){
			printf("fail at send()...\n");
				return -1;
			}
			return -1;
		}
	else
	{    

		char count[50];
		int_to_stream(all,count);
		strcpy(buf,count);
		send_succ = send(sock,buf,dataLen,0);
		if(send_succ == -1){
			printf("fail at send()...\n");
			return -1;
		}
	}	
	//count_current();
	return is_user;
}

//verify user infor				  
int isUser(char *userName,char *userPassword){
printf("%s\n",userName);
printf("%s\n",userPassword);
	int i;
	for(i=0;i<maxUser;i++)
	if(strcmp(userName,users[i].userName)==0)
		if(strcmp(userPassword,users[i].userPassword)==0)
		 	if(users[i].login==1)
				return -2;//no login
		 	else {users[i].login=1;return i;}//not worked
		else return -1;//uname pwd failed
	
	return -1;//error
}

//check there is no overflow of clientMAX个
int clientOverFlow(int sock,int count){
	if(count > clientMAX){
		strcpy(buf,"there have been too many clients,refused!");
		send_succ = send(sock,buf,dataLen,0);
		if(send_succ == -1){
			printf("fail at send()...\n");
			return -1;
		}
		return 0;
	}
	else{
		strcpy(buf,"pass");
		send_succ = send(sock,buf,dataLen,0);
		if(send_succ == -1){
			printf("fail at send()...\n");
			return -1;
		}
		return 1;
	}
}

//verify client thread
int get_clientThread(){
	int i;
	for(i = 1;i < threadMAX;i ++)
		if(cThread[i].valid == 1){
			cThread[i].valid = 0;
			return i;
		}
	return -1;
}				  

//reverse the thread
void back_clientThread(int i){
	cThread[i].valid = 1;
}

//access port
int get_port(){
	int i;
	for(i = 1;i < portMAX;i ++)
		if(port_array[i].valid == 1)
		{
			port_array[i].valid = 0;
			return i;
		}
	return -1;
}

//verify the port
void back_port(int i){
	port_array[i].valid = 1;			
}

//verify filepath
int get_path(){
	int i;
	for(i = 1;i <= clientMAX;i ++)
		if(filePath[i].valid == 1){
			filePath[i].valid = 0;
			return i;
		}
	return -1;
}

//valid file		  
void back_path(int i){
	filePath[i].valid = 1;	
}

//conversion
void int_to_stream(int a,char *b){
	int i = 0;
	int j;
	char temp;
	while(a != 0)
	{
		b[i] = '0' + a % 10;
		a = a / 10;
		i++;
	}
	for (j = i - 1;j >= i/2;j --)
	{
		temp = b[j];
		b[j] = b[i-j-1];
		b[i-j-1] = temp;
		
	}
	b[i] = '\0';
}

//conv int
int stream_to_int(char *b){
	int i = 0;
	int a = 0;
	while(b[i]!= '\0')
	{
		a += (int)(b[i] - '0');
		a *=10;
		i++;
	}
	a /= 10;
	return a;
}

//makeup thread clien
void makeUp_clientThreadInfor(char *a,char *b,char *c){
	strcpy(c,a);
	strcat(c,"/");
	strcat(c,b);
}

//break thread
void break_clientThreadInfor(char *a,char *b,char *c){
	int i;
	for(i = 0;c[i] != '/';i ++)
		a[i] = c[i];
	a[i] = '\0';
	
	int j;
	for(j = i + 1;c[j] != '\0';j ++)
		b[j - i - 1] = c[j];
	b[j - i - 1] = '\0';
}

//cmd check
void makeUp_cmdThreadInfor(char *a,char *b,char *c,char *d,char *e,char *f){
	strcpy(f,a);
	strcat(f,"/");
	strcat(f,b);
	strcat(f,"/");
	strcat(f,c);
	strcat(f,"/");
	strcat(f,d);
	strcat(f,"/");
	strcat(f,e);
}

//break cmd in parts
void break_cmdThreadInfor(char *a,char *b,char *c,char *d,char *e,char *f){
	int i;
	for(i = 0;f[i] != '/';i ++)
		a[i] = f[i];
	a[i] = '\0';
	
	int j;
	for(j = i + 1;f[j] != '/';j ++)
		b[j - i - 1] = f[j];
	b[j - i - 1] = '\0';
	
	int k;
	for(k = j + 1;f[k] != '/';k ++)
		c[k - j - 1] = f[k];
	c[k - j - 1] = '\0';
	
	int l;
	for(l = k + 1;f[l] != '/';l ++)
		d[l - k - 1] = f[l];
	d[l - k - 1] = '\0';
	
	int m;
	for(m = l + 1;f[m] != '\0';m ++)
		e[m - l - 1] = f[m];
	e[m - l - 1] = '\0';
}
															  
//a new user enters
void *createNewClientThread(char *clientThreadInfor){
	
	char msgSock_stream[5];
	char newClientThread_i_stream[5];
	
	break_clientThreadInfor(msgSock_stream,newClientThread_i_stream,clientThreadInfor);
	
	int msgSock = stream_to_int(msgSock_stream);
	int newClientThread_i = stream_to_int(newClientThread_i_stream);
	printf("msgSock = %d\n",msgSock);
	printf("newClientThread_i = %d\n",newClientThread_i);
	
	printf("thread_tid = %u\n",(unsigned int)pthread_self());
      	printf("thread pid is %d\n",getpid());
	
	//check the user valid or no
	int user_n=checkUser(msgSock);
	if(user_n != -1){
		printf("a friendly user...\n");
		clientCounter++;
		count_current();
      	all++;
		count_all();
		list();
		users[user_n].msgSock=msgSock;
		exec_succ = clientInteract(msgSock);//call func
		if(exec_succ == -1)
			printf("fail at clientInteract()...\n");
		/*else 
		      users[user_n].msgSock=msgSock;*/
	}	
	else
		printf("invalid userName or passWord...\n");
	

	back_clientThread(newClientThread_i);
	
	LockMutex(&clientCounterMutex);
	//clientCounter --;
	UnlockMutex(&clientCounterMutex);
	//count_current();
	
	close(msgSock);
	printf("I am out...\n");
	pthread_exit(NULL);
}	  

//msgsock and datasock
int clientInteract(int msgSock){
	//geth path of file
	int filePath_i = get_path();
	memset(filePath[filePath_i].currentFilePath,0,dataLen);
	printf("filePath_i = %d\n",filePath_i);
	//filePath[filePath_i].currentFilePath[dataLen];
	
	//command
	char client_cmd[dataLen];
	memset(client_cmd,0,dataLen);
	int dataSock;
	while(1){
		
		//command exec
		recv_succ = recv(msgSock,client_cmd,dataLen,0);
            	if(recv_succ == -1){   
                 	printf("reading command failed\n");
                 	return -1;   
             	}
           	printf("%s\n",client_cmd);
           	//sleep(5);
           	// dataPort
		int dataPort_i,dataPort;
		char dataPort_stream[dataLen];
		dataPort_i = get_port();
		printf("dataPort_i %d is get\n",dataPort_i);
		printf("dataPort_i = %d\n",dataPort_i);
		if(dataPort_i == -1){
			printf("fail at get_port()...\n");
			return -1;
		}
		dataPort = port_array[dataPort_i].port; 
		printf("dataPort = %d\n",dataPort);
		int_to_stream(dataPort,dataPort_stream);
		send_succ = send(msgSock,dataPort_stream,dataLen,0);
		if(send_succ == -1){
			printf("fail at send()...\n");
			return -1;
		}
           	
           	//dataSock
           	int dataSocket = runServer(dataPort);
           	if(dataSocket == -1){
           		printf("fail at runServer()...\n");
           		return -1;
           	}
           	dataSock = accept(dataSocket,(struct sockaddr*)0,(int*)0);
           	if(dataSock == -1){
           		printf("fail at accept()...\n");
           		return -1;
           	}
           	printf("dataSock = %d\n",dataSock);
           	//command thread
		int cmdThread_i = get_clientThread();
		printf("cmdThread_i = %d\n",cmdThread_i);
		if(cmdThread_i == -1){
			printf("fail at get_clientThread()...\n");
			return -1;
		}
           	pthread_t cmdThread = cThread[cmdThread_i].client_thread;
           	//socke
           	//funcions:filePath_i,dataPort_i,dataSock,cmdThread_i,dataSocket;
		
		char filePath_i_stream[10],dataPort_i_stream[10];
		char dataSock_stream[10],cmdThread_i_stream[10];
		char cmdThreadInfor[dataLen];
		
		int_to_stream(filePath_i,filePath_i_stream);
		int_to_stream(dataPort_i,dataPort_i_stream);
		int_to_stream(dataSock,dataSock_stream);
		int_to_stream(cmdThread_i,cmdThread_i_stream);
		
          	if(strcmp(client_cmd,"pwd")==0){ //ok
          		//cmd_arg = null
             	printf("cmmand pwd...\n");  
                   makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_pwd,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}  
                }   
             else if(strcmp(client_cmd,"ls") == 0){  	//ok	
             	//cmd_arg = null 
			printf("command ls...\n");   
                   	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_dir,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
            	}   
            	else if(strcmp(client_cmd,"cd..") == 0){   //ok
                 	printf("command cd back...\n");  
               	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_cdBack,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
              	} 
            	else if(strncmp(client_cmd,"cd",2) == 0){  //ok
                  	printf("command cd...\n");
                  	char cmd_arg[100];
                  	get_arg(client_cmd,3,cmd_arg);//client_cmdcmd_arg
                  	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_cd,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
                 }  
          	else if(strncmp(client_cmd,"get",3) == 0){  //ok
                	printf("command get...\n");
                	char cmd_arg[100];
                  	get_arg(client_cmd,4,cmd_arg);//client_cmdcmd_arg
                  	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_get,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			} 
            	}   
          	else if(strncmp(client_cmd,"put",3) == 0){
                	printf("command put...\n");
                	char cmd_arg[100];
                  	get_arg(client_cmd,4,cmd_arg);//client_cmdcmd_arg
                  	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_put,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
             	}  
		else if(strncmp(client_cmd,"mget",4) == 0){  //ok
                	printf("command get...\n");
                	char cmd_arg[100];
                  	get_arg(client_cmd,5,cmd_arg);//client_cmdcmd_arg
                  	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_mget,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			} 
            	}   
        	else if(strcmp(client_cmd,"?") == 0){
                 	printf("command ?...\n");  
               	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_help,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
              	}
          	else if(strncmp(client_cmd,"quit",4) == 0){  
                	printf("command quit...\n");  
			char cmd_arg[100];
           		get_arg(client_cmd,4,cmd_arg);//client_cmdcmd_arg
               	makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_quit,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
                	break;   
              	}
           	else if(strncmp(client_cmd,"mkdir",5) == 0){
            		printf("command mkdir...\n");
           		char cmd_arg[100];
           		get_arg(client_cmd,6,cmd_arg);//client_cmdcmd_arg
           		makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_mkdir,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
             	}     
		else if(strncmp(client_cmd,"rmdir",5) == 0){
            		printf("command rmdir...\n");
           		char cmd_arg[100];
           		get_arg(client_cmd,6,cmd_arg);//client_cmdcmd_arg
           		makeUp_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
			printf("cmdThreadInfor %s\n",cmdThreadInfor);
			exec_succ = pthread_create(&cmdThread,NULL,cmd_rmdir,cmdThreadInfor);
			if(exec_succ != 0){
				printf("fail at pthread_create()...\n");
				return -1;
			}
             	}    
           	else{   
                	printf("bad request!\n");
                	sleep(5);
                	printf("I am bad...\n");  
              	}
	}
	//back path
	back_path(filePath_i);
	return 0;
}

//client_cmdcmd_arg
void get_arg(char *b,int a,char *c){
	int i;
	for(i = a;b[i] != '\0';i ++)
		c[i - a] = b[i];
	c[i - a] = '\0';
}					 	

//pwd
void * cmd_pwd(char * cmdThreadInfor){

	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];//null
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);

	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	printf("file path is %s\n",filePath[filePath_i].currentFilePath);
     	send_succ = send(dataSock,filePath[filePath_i].currentFilePath,dataLen,0);
     	if(send_succ == -1){
     		printf("fail at send()...\n");
     		return;
     	}
     	printf("send_succ = %d\n",send_succ);
	printf("pwd is done...\n");
	
	
	
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}	

//dir
void * cmd_dir(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];//null
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	DIR *pdir;   
     	char fileName[30];  
     	char fileInfo[50];  
     	int i, fcounts = 0, len;  
     	struct dirent *pent;  
     	int fd;  
     	struct stat fileSta;  
     	char dir_filePath[dataLen];  
      
      	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	pdir = opendir(filePath[filePath_i].currentFilePath);  
     	pent = readdir(pdir);  
       
     	while(pent!=NULL){  
         	fcounts++;  
         	pent=readdir(pdir);  
     	}  	
   	char fcounts_stream[dataLen];
   	int_to_stream(fcounts,fcounts_stream);
     	
     	closedir(pdir);  
   
     	if(fcounts <= 0){
     		strcpy(buf,"0");
     		send(dataSock,buf,dataLen,0);
     	}  
     	else{
     		strcpy(buf,"1");
     		send(dataSock,buf,dataLen,0);
     		send_succ = send(dataSock,fcounts_stream,dataLen,0);  //send on socket
     		
     		if((int)filePath[filePath_i].currentFilePath[0] == 0)
     			getcwd(filePath[filePath_i].currentFilePath,dataLen);
         	pdir = opendir(filePath[filePath_i].currentFilePath);  
         	for(i = 0;i < fcounts;i ++){
         		pent=readdir(pdir);
         		memset(fileName,0,30);
         		memset(fileInfo,0,sizeof(fileInfo));
         		strcpy(fileName,pent->d_name); 
               
             	//check the file is a directory or a file 
             	memset(dir_filePath,0,dataLen);  
             	strcpy(dir_filePath,filePath[filePath_i].currentFilePath);  
             	strcat(dir_filePath,"/");  
             	strcat(dir_filePath,fileName);  
             	
             	fd = open(dir_filePath,O_RDONLY, S_IREAD);  
               		
             	fstat(fd,&fileSta);  
             	if(S_ISDIR(fileSta.st_mode)){  
                 		strcat(fileInfo,"dir\t");  
                 		strcat(fileInfo,fileName);  
             		}  
             	else{     
                 		strcat(fileInfo,"file\t");  
                 		strcat(fileInfo,fileName);  
             		}  
             	send(dataSock,fileInfo,dataLen,0);
         	}  
         	closedir(pdir);
         }
         
     	printf("dir is done...\n");
        //success
     	
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}		

//cd cmd	
void * cmd_cd(char * cmdThreadInfor){
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];//argument
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0;  
     	int flag = 0;  
     	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	pdir = opendir(filePath[filePath_i].currentFilePath); 
     	pent=readdir(pdir);  
     	while(pent!=NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     
     	closedir(pdir);  
   
     	if(fcounts <= 0){  
         	strcpy(buf,"0");
     		send(dataSock,buf,dataLen,0);  
     	}
     	else{  
     		strcpy(buf,"1");
     		send(dataSock,buf,dataLen,0);
     		
     		if((int)filePath[filePath_i].currentFilePath[0] == 0)
     			getcwd(filePath[filePath_i].currentFilePath,dataLen);
     		pdir = opendir(filePath[filePath_i].currentFilePath);  
         	for(i = 0;i < fcounts;i ++){
         		pent = readdir(pdir);
         		if(strcmp(pent->d_name,cmd_arg) == 0){  
                 		strcat(filePath[filePath_i].currentFilePath,"/");  
                 		strcat(filePath[filePath_i].currentFilePath,cmd_arg);  
                 		flag = 1;  
                 		break;  
             		}  
     		}
     		if(flag == 1){//copy to buffer
    			strcpy(buf,"1");
       		send(dataSock,buf,dataLen,0);  
             	
             	send(dataSock,filePath[filePath_i].currentFilePath,dataLen,0);  
       		}
       	else{
       		strcpy(buf,"0");
       		send(dataSock,buf,dataLen,0);	
       		} 
        	closedir(pdir); 
     	}
    	
     	printf("cd is done...\n");
     	
     
	
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}

//cd.. revert one directroy back			
void * cmd_cdBack(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];//null
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	int len;  
     	int i, record;  
   	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     			getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	
     	len = strlen(filePath[filePath_i].currentFilePath);  
       
     	for(i = len - 1;i >= 0;i --){
     		if(filePath[filePath_i].currentFilePath[i] == '/'){  
             	filePath[filePath_i].currentFilePath[i] = '\0';  
             	break;  
         	}  
         	filePath[filePath_i].currentFilePath[i] = '\0';  
     	}
     	send_succ = send(dataSock,filePath[filePath_i].currentFilePath,dataLen,0);
     	if(send_succ == -1){
     		printf("fail at send()...\n");
     		return;
     	}
     	
     	printf("cd.. is done...\n");
     	
     	
	 
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}

//get or copy to server		
void * cmd_get(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];
	char tran_type[20];
	int trans=0; 
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0;  
     	int flag=0;
     	char get_filePath[dataLen];
     	
     	int fd;
     	struct stat fileSta;  
     	long fileSize;
     	char fileSize_stream[dataLen];
     	  
     	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	pdir = opendir(filePath[filePath_i].currentFilePath); 
     	pent=readdir(pdir);  
     	while(pent!=NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     
     	closedir(pdir);  
   
     	if(fcounts <= 0){  
         	return;  
     	}
     	else{ 
     		if((int)filePath[filePath_i].currentFilePath[0] == 0)
     			getcwd(filePath[filePath_i].currentFilePath,dataLen);
     		pdir = opendir(filePath[filePath_i].currentFilePath); 
		int j,k,m;
		for(j=0;cmd_arg[j]!='\0';j++)     //ascii cmd check
			if(cmd_arg[j]=='-')
			{	
				k=j+1;m=0;
				if(cmd_arg[k]!='\0')
				for(;cmd_arg[k]!='\0';k++,m++)
						tran_type[m]=cmd_arg[j];
				tran_type[m]='\0';
				cmd_arg[j]='\0';
			}
		if(tran_type!=NULL)
			if(strcmp(tran_type,"binary")==0)     //binary
				trans=1;
         	//cmd_arg[]
         	for(i = 0;i < fcounts;i ++){
         		pent = readdir(pdir);
         		if(strcmp(pent->d_name,cmd_arg) == 0){
         			strcpy(get_filePath,filePath[filePath_i].currentFilePath);  
                 		strcat(get_filePath,"/");  
                 		strcat(get_filePath,cmd_arg); 
                 		//get_filePath
                 		flag = 1;  
                 		break;  
             		}  
      		}
      	}
      	closedir(pdir);
      	
	if(flag == 1){//start...
		strcpy(buf,"1");
		send(dataSock,buf,dataLen,0);
		//(get_filePath),
			if(trans==0)
      			fd = open(get_filePath,O_RDONLY, S_IREAD);   //open the filename
			else 
				fd= fopen(get_filePath,"rb");
     		if(fd != -1){
     			fstat(fd,&fileSta);  
      	   		fileSize = (long) fileSta.st_size;
      	   		int_to_stream(fileSize,fileSize_stream);  
      	   		send(dataSock,fileSize_stream,dataLen,0); //send the file now
      	   		 
      	   		memset(buf,0,dataLen);  
      	   		while(fileSize > dataLen){
      	   			read(fd,buf,dataLen);  
             		send(dataSock,buf,dataLen,0);  
             		fileSize = fileSize - dataLen;  
         		}  
           
         		read(fd,buf,fileSize);  
         		send(dataSock,buf,fileSize,0);  
         		close(fd);  
         		printf("transfer %s completed\n",get_filePath);  
     		}  
     		else
         		printf("open file %s failed\n",filePath);   
       	}
       else{//or other routine
       	strcpy(buf,"0");
       	send(dataSock,buf,dataLen,0);	
       	}  
    	
    	printf("get is done...\n");
    	

	
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
	
}

//put or move fle to 		
void * cmd_put(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];//argument
	char tran_type[20];
	int trans=0; 
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0;  
     	int flag = 1;
     	char put_filePath[dataLen];
     	
     	int fd;  
     	long fileSize;
     	char fileSize_stream[dataLen];
	
	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	pdir = opendir(filePath[filePath_i].currentFilePath); 
     	pent=readdir(pdir);  
     	while(pent != NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     	closedir(pdir);  
   
     	if(fcounts <= 0){  
         	return;  
     	}
     	else{  
     		if((int)filePath[filePath_i].currentFilePath[0] == 0)
     			getcwd(filePath[filePath_i].currentFilePath,dataLen);
     		pdir = opendir(filePath[filePath_i].currentFilePath); 

		int j,k,m;
		for(j=0;cmd_arg[j]!='\0';j++)     //ascii cmd
			if(cmd_arg[j]=='-')
			{	
				k=j+1;m=0;
				if(cmd_arg[k]!='\0')
				for(;cmd_arg[k]!='\0';k++,m++)
						tran_type[m]=cmd_arg[j];
				tran_type[m]='\0';
				cmd_arg[j]='\0';
			}
		if(tran_type!=NULL)
			if(strcmp(tran_type,"binary")==0)     //binary
				trans=1;
 
         	//cmd_arg[]
         	for(i = 0;i < fcounts;i ++){
         		pent = readdir(pdir);
         		if(strcmp(pent->d_name,cmd_arg) == 0){
                 		flag = 0;  
                 		break;  
             		}  
      		}
      	}
      	closedir(pdir);
	
	if(flag == 1){//start...
		strcpy(buf,"1");
		send(dataSock,buf,dataLen,0);
		//(put_filePath),
      		strcpy(put_filePath,filePath[filePath_i].currentFilePath);  
           	strcat(put_filePath,"/");  
             strcat(put_filePath,cmd_arg); 
             //put_filePath
      		
      		if(trans==0)
      			fd = open(put_filePath,O_RDWR|O_CREAT, S_IREAD|S_IWRITE);   //openfile
			else 
				fd= fopen(put_filePath,"wb");
     		if(fd != -1){
     			memset(buf,0,dataLen);  
         		recv(dataSock,fileSize_stream,dataLen,0);//receive
   			
   			fileSize = stream_to_int(fileSize_stream);
         		while(fileSize > dataLen){  
             		recv(dataSock,buf,dataLen,0);  
             		write(fd,buf,dataLen);  
             		fileSize = fileSize - dataLen;  
         		}  
           
         		recv(dataSock,buf,fileSize,0);  
         		write(fd,buf,fileSize);  
         		close(fd);  
         		printf("transfer completed\n");  
     		}  
     		else
         		printf("open or create file %s failed\n",put_filePath);  
       	}
     	else{//other...
       	strcpy(buf,"0");
       	send(dataSock,buf,dataLen,0);	
       	}  
	
	printf("put is done...\n");
	
	
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}

//?help section		
void * cmd_help(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];//null
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	send_succ = send(dataSock,help,dataLen,0);
	if(send_succ == -1){
		printf("fail at send()...\n");
		return;
	}
	
	printf("? is done...\n");
	
	
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}

//quit		
void * cmd_quit(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];//null
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	int i;
	for(i=0;i<maxUser;i++)
		if(strcmp(cmd_arg,users[i].userName)==0)
			{
				users[i].login=0;
				break;
			}
	//printf("%s,%s\n",cmd_arg,users[i].userName);
	strcpy(buf,"quit");
	send_succ = send(dataSock,buf,dataLen,0);
	if(send_succ == -1){
		printf("fail at send()...\n");
		return;
	}
	clientCounter --;
	count_current();
	list();
	printf("quit is done...\n");


	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}

//mkdir		
void * cmd_mkdir(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0;  
     	int flag = 1;
     	char mkdir_filePath[dataLen];
	
	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	pdir = opendir(filePath[filePath_i].currentFilePath); 
     	pent=readdir(pdir);  
     	while(pent != NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     	closedir(pdir);  
   
     	if(fcounts < 0){  
         	strcpy(buf,"0");
     		send_succ = send(dataSock,buf,dataLen,0);  
            printf("dataPort_i %d is back\n",dataPort_i);
		back_clientThread(cmdThread_i);
		close(dataSock);  
     	}
     	else{  
     		if((int)filePath[filePath_i].currentFilePath[0] == 0)
     			getcwd(filePath[filePath_i].currentFilePath,dataLen);
     		pdir = opendir(filePath[filePath_i].currentFilePath);  
         	//cmd_arg[]
         	for(i = 0;i < fcounts;i ++){
         		pent = readdir(pdir);
         		if(strcmp(pent->d_name,cmd_arg) == 0){
                 		flag = 0;  
                 		break;  
             		}  
      		}
      	}
      	
      	closedir(pdir);
      	
      	if(flag == 1){//chek directory
      		
      		strcpy(mkdir_filePath,filePath[filePath_i].currentFilePath);  
           	strcat(mkdir_filePath,"/");  
           	strcat(mkdir_filePath,cmd_arg); 
           	//mkdir_filePath
      		
      		int mkdir_succ = mkdir(mkdir_filePath,0);
      		if(mkdir_succ == 0){
      			strcpy(buf,"1");
      			send_succ = send(dataSock,buf,dataLen,0);
      		}
      		else{
      			strcpy(buf,"0");
      			send_succ = send(dataSock,buf,dataLen,0);
      		}		
      	}
      	else{//fail
      		strcpy(buf,"2");
      		send_succ = send(dataSock,buf,dataLen,0);
      	}
      
      	printf("mkdir is done...\n");
      

	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}		

//rmdir		
void * cmd_rmdir(char * cmdThreadInfor){
	
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[30];
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0,fcounts_1=0;  
     	int flag = 0;
     	char rmdir_filePath[dataLen];
	
	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	pdir = opendir(filePath[filePath_i].currentFilePath); 
     	pent=readdir(pdir);  
     	while(pent != NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     	closedir(pdir);  
   
     	if(fcounts < 0){  
         	strcpy(buf,"0");
     		send_succ = send(dataSock,buf,dataLen,0);  
            printf("dataPort_i %d is back\n",dataPort_i);
		back_clientThread(cmdThread_i);
		close(dataSock); return;
     	}
     	else{  
     		if((int)filePath[filePath_i].currentFilePath[0] == 0)
     			getcwd(filePath[filePath_i].currentFilePath,dataLen);
     		pdir = opendir(filePath[filePath_i].currentFilePath);  
         	//cmd_arg[]
         	for(i = 0;i < fcounts;i ++){
         		pent = readdir(pdir);
         		if(strcmp(pent->d_name,cmd_arg) == 0){
                 		flag = 1;  
                 		break;  
             		}  
      		}
      	}
      	closedir(pdir);
      	
      	if(flag == 1){//
      		strcpy(rmdir_filePath,filePath[filePath_i].currentFilePath);  
            	strcat(rmdir_filePath,"/");  
            	strcat(rmdir_filePath,cmd_arg); 
           	//rmdir_filePath
      		pdir = opendir(rmdir_filePath);
			pent = readdir(pdir);
			while(pent != NULL)
			{  
	 	        	fcounts_1 ++;  
	 	        	pent = readdir(pdir);  
	 	    	}
			if(fcounts_1 < 0)
			{
				strcpy(buf,"0");
      			send_succ = send(dataSock,buf,dataLen,0);  
     			}
			else
			if(fcounts_1>0)          //more file
			{
				strcpy(buf,"2");
      			send_succ = send(dataSock,buf,dataLen,0);
			}
			else
			{
      			int rmdir_succ = rmdir(rmdir_filePath);
				if(rmdir_succ==0)    
				{
					strcpy(buf,"1");
      				send_succ = send(dataSock,buf,dataLen,0);
				}
				else           
				{
					strcpy(buf,"0");
      				send_succ = send(dataSock,buf,dataLen,0);
				}
			}
		}
		else {//other 
      		      strcpy(buf,"3");
      			send_succ = send(dataSock,buf,dataLen,0);
      		}
      
      	printf("rmdir is done...\n");
      

	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}		

void InitializeUsers()
{
	strcpy(users[0].userName,"Alice");
      strcpy(users[0].userPassword,"000");
      strcpy(users[1].userName,"Bob");
      strcpy(users[1].userPassword,"111");
	strcpy(users[2].userName,"Catherine");
      strcpy(users[2].userPassword,"222");
	int i;
	for(i=0;i<maxUser;i++)
		{
			users[i].login=0;
			users[i].msgSock=9999;
		}
}
void count_current()
{
	int shm_id;
	key_t key;
	int *temp;
	char *name="."; 
	key=ftok(name,0);
	/*if(key==-1)
		perror("ftok error");*/
	shm_id=shmget(key,1024,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	temp=(int *)shmat(shm_id,NULL,0);
	*temp=clientCounter;
//printf("%d\n",shm_id);
//printf("/////////////////////////////////////////////////////////////////%d\n",*temp);
	return;
}				  

void count_all()
{
	int shm_id;
	key_t key;
	int *temp;
	char *name="/"; 
	key=ftok(name,0);
	/*if(key==-1)
		perror("ftok error");*/
	shm_id=shmget(key,1024,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	temp=(int *)shmat(shm_id,NULL,0);
	*temp=all;
//printf("%d\n",shm_id);
//printf("/////////////////////////////////////////////////////////////////%d\n",*temp);
	return;
}

void list()
{
	int shm_id;
	key_t key;
	struct user *temp;
	char *name="/"; 
	key=ftok(name,1);
	
	/*if(key==-1)
		perror("ftok error");*/
	shm_id=shmget(key,4096,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	temp=(struct user *)shmat(shm_id,NULL,0);
	//printf("%d\n",shm_id);
	memcpy(temp,users,sizeof(users));
	return;
}

void kill_user()
{
	int i;
	char buffer[dataLen];
	int shm_id;
	int shm_id_1;
	key_t key;
	key_t key_1;
	char *temp;
	char *temp_1;
	char *name="/"; 
	key=ftok(name,2);
	key_1=ftok(name,3);

	/*if(key==-1)
		perror("ftok error");*/
	shm_id=shmget(key,4096,IPC_CREAT);
	if(shm_id==-1)
	{
		perror("shmget error");
		return;
	}
	if(key_1==-1)
		perror("ftok error");
	shm_id_1=shmget(key_1,4096,IPC_CREAT);
	if(shm_id_1==-1)
	{
		perror("shmget error");
		return;
	}

	temp=(char *)shmat(shm_id,NULL,0);
	
	temp_1=(char *)shmat(shm_id_1,NULL,0);
	
	memset(temp,0,sizeof(temp));
	memset(temp_1,0,sizeof(temp_1));
	memset(buffer,0,sizeof(buffer));

	while(1)
		if(memcmp(temp,temp_1,sizeof(temp_1))!=0)
		{
			memcpy(buffer,temp,strlen(temp)+1);
			//printf("//////%s\n",buffer);
			for(i=0;i<maxUser;i++)
				if(strcmp(buffer,users[i].userName)==0)
					if(users[i].login==0)
						{
							list();
							clean_up();
							memset(buffer,0,sizeof(buffer));
							printf("This user has logged out\n");
							break;
						}
					else 
						{
							user_quit(buffer);
							list();
							clean_up();
							//sleep(10);
							memset(buffer,0,sizeof(buffer));
							break;
						}	
			if(i==maxUser)
				{
				printf("It's an invalid user\n");
				clean_up();
				memset(buffer,0,sizeof(buffer));
				}
			sleep(1);
		}	
	return;
}

void user_quit(char* userName)
{
	int i;
	for(i=0;i<maxUser;i++)
		if(strcmp(userName,users[i].userName)==0)
		{
			users[i].login=0;
			//printf("'''''''''''''''''''''''%d\n",users[i].msgSock);
			close(users[i].msgSock);
			clientCounter --;
			count_current();
		      printf("Quit user %s successfully!\n",users[i].userName);
			return;
		}
	if(i==maxUser)
	{
		printf("It's an invalid user\n");return;
	}
	return;
}

void clean_up()
{
	int i;
	int shm_id;
	key_t key;
	char *temp;
	char *name="/"; 
	key=ftok(name,2);
	/*if(key==-1)
		perror("ftok error");*/
	shm_id=shmget(key,4096,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	temp=(char *)shmat(shm_id,NULL,0);
	memset(temp,0,sizeof(temp));
}

void quit()
{
	int i;
	char buffer[dataLen];
	int shm_id;
	int shm_id_1;
	key_t key;
	key_t key_1;
	char *temp;
	char *temp_1;
	char *name="/"; 
	key=ftok(name,4);
	key_1=ftok(name,3);

	/*if(key==-1)
		perror("ftok error");*/
	shm_id=shmget(key,4096,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	//if(key_1==-1)
	//	perror("ftok error");
	shm_id_1=shmget(key_1,4096,IPC_CREAT);
	if(shm_id_1==-1)
	{
//		perror("shmget error");
		return;
	}

	temp=(char *)shmat(shm_id,NULL,0);
//	printf("%d\n",shm_id);
	temp_1=(char *)shmat(shm_id_1,NULL,0);
//	printf("%d\n",shm_id_1);
	memset(temp,0,sizeof(temp));
	memset(temp_1,0,sizeof(temp_1));
//	printf(":::::::::::::::::::::::::::::%d\n",memcmp(temp,temp_1,sizeof(temp_1)));
	while(1)
		if(memcmp(temp,temp_1,sizeof(temp_1))!=0)
		{
			memcpy(buffer,temp,strlen(temp)+1);
			if(strcmp(buffer,"quit")!=0)
				{
					printf("error in shm");
					break;
				}
			else
				{
					printf("Close the ftp server!\n");
					exit(0);
					
				}
		}
	return;
}

void * cmd_mget(char * cmdThreadInfor)
{
	char filePath_i_stream[10],dataPort_i_stream[10];
	char dataSock_stream[10],cmdThread_i_stream[10];
	char cmd_arg[100];//存放的是要发给客户端的文件
	char tran_type[20];
	int trans=0; 
	
	break_cmdThreadInfor(filePath_i_stream,dataPort_i_stream,dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	
	int filePath_i = stream_to_int(filePath_i_stream);
	int dataPort_i = stream_to_int(dataPort_i_stream);
	int dataSock = stream_to_int(dataSock_stream);
	int cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0;  
     	int flag=0;
     	char get_filePath[dataLen];
     	
     	int fd;
     	struct stat fileSta;  
     	long fileSize;
     	char fileSize_stream[dataLen];
     	  
     	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	pdir = opendir(filePath[filePath_i].currentFilePath); 
     	pent=readdir(pdir);  
     	while(pent!=NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     
     	closedir(pdir);  
   
     	if(fcounts <= 0){  
         	return;  
     	}
     	else{ 

		int j,k,m;
		for(j=0;cmd_arg[j]!='\0';j++)     //ascii
			if(cmd_arg[j]=='-')
			{	
				k=j+1;m=0;
				if(cmd_arg[k]!='\0')
				for(;cmd_arg[k]!='\0';k++,m++)
						tran_type[m]=cmd_arg[j];
				tran_type[m]='\0';
				cmd_arg[j]='\0';
			}
		if(tran_type!=NULL)
			if(strcmp(tran_type,"binary")==0)     //binary
				trans=1;
         	
         	
      		}
      	     	
	if((int)filePath[filePath_i].currentFilePath[0] == 0)
     		getcwd(filePath[filePath_i].currentFilePath,dataLen);
     	 
      	
	int index=0,index_str;
	char file[dataLen];
	while(cmd_arg[index]!='\0')
	{		
		if(index!=0)
			index++;
		index_str=0;
		while(cmd_arg[index]!='&' && cmd_arg[index]!='\0')
		{		
			file[index_str]=cmd_arg[index];
			index_str++;
			index++;
		}
		file[index_str]='\0';			
		//file[]
		pdir = opendir(filePath[filePath_i].currentFilePath);
		for(i = 0;i < fcounts;i ++){
         		pent = readdir(pdir);
         		if(strcmp(pent->d_name,file) == 0){
         			strcpy(get_filePath,filePath[filePath_i].currentFilePath);  
                 		strcat(get_filePath,"/");  
                 		strcat(get_filePath,file); 
                 		//get_filePath
                 		flag = 1;  
                 		break;  
             			}  
      			}	
	if(flag == 1){//flag set...
		flag=0;
		strcpy(buf,"1");
		send(dataSock,buf,dataLen,0);
		//(get_filePath),
			if(trans==0)
      			fd = open(get_filePath,O_RDONLY, S_IREAD);   //openfile
			else 
				fd= fopen(get_filePath,"rb");
     		if(fd != -1){
     			fstat(fd,&fileSta);  
      	   		fileSize = (long) fileSta.st_size;
      	   		int_to_stream(fileSize,fileSize_stream);  
      	   		send(dataSock,fileSize_stream,dataLen,0); //send data
      	   		 
      	   		memset(buf,0,dataLen);  
      	   		while(fileSize > dataLen){
      	   			read(fd,buf,dataLen);  
             		send(dataSock,buf,dataLen,0);  
             		fileSize = fileSize - dataLen;  
         		}  
           
         		read(fd,buf,fileSize);  
         		send(dataSock,buf,fileSize,0);  
         		close(fd);  
         		printf("file %s transfers completed\n",file);  
     		}  
     		else
         		printf("open file %s failed\n",get_filePath);   
       	}
       else{//other...
       	strcpy(buf,"0");
       	send(dataSock,buf,dataLen,0);	
       		}  
	memset(get_filePath,'0',sizeof(get_filePath));
			
	}

	closedir(pdir);
  
    	printf("mget is done...\n");
    	

	
	//back_port(dataPort_i);
	printf("dataPort_i %d is back\n",dataPort_i);
	back_clientThread(cmdThread_i);
	close(dataSock);
}

