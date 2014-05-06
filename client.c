//All Header Files needed 
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
#include <arpa/inet.h>

//Important multithreading wrapper functions and user info functions declarations
void InitializeMutex(pthread_mutex_t *pMutex); 	//initialize mutex threads
int DestroyMutex(pthread_mutex_t *pMutex);     	//destroy mutex
int LockMutex(pthread_mutex_t *pMutex);        	//lock mutex
int UnlockMutex(pthread_mutex_t *pMutex);      	//unlock mutex
char *getusrPassword(char *userInfo);
char *getusrName(char *userInfo);
int check_ip_port(struct sockaddr_in address,char *userInfo);
int connectServer(struct sockaddr_in address);	//connect to server
int checkUser(int sock,char *,char *);					//verify user
int isOverFlow(int sock);					//client overflow check
int stream_to_int(char *b);				//stream to int 
void int_to_stream(int a,char *b);		  	//int to stream
void get_arg(char *b,int a,char *c);			//client_cmd cmd_arg
int get_cmdThread();				  	//get cmd func
void back_cmdThread(int i);			  	//back/valid cmd
void makeUp_cmdThreadInfor(char *a,char *b,char *c,char *d);
								//make thread for client
void break_cmdThreadInfor(char *a,char *b,char *c,char *d);
								//break from thread					
void cmd_pwd_client();					//pwd client
void cmd_dir_client();					//dir client
void cmd_cdback_client();					//cdback
void cmd_cd_client(char *cmd_arg);			//on cd 
void cmd_mkdir_client(char *cmd_arg);			//mkdir client
void cmd_rmdir_client(char *cmd_arg);            //rmdir on client side

void *cmd_pwd_server(char *cmdThreadInfor);		//pwd on server
void *cmd_dir_server(char *cmdThreadInfor);		//dir cmd on server
void *cmd_cdback_server(char *cmdThreadInfor);	//cdback
void *cmd_cd_server(char *cmdThreadInfor);		//cd cmd
void *cmd_get_server(char *cmdThreadInfor);	//transfer from server
void *cmd_put_server(char *cmdThreadInfor);	//transfer file to server
void *cmd_help_server(char *cmdThreadInfor);		//--help cmd
void *cmd_quit_server(char *cmdThreadInfor);		//quit client
void *cmd_mkdir_server(char *cmdThreadInfor);		//mkdir server
void *cmd_rmdir_server(char *cmdThreadInfor);         //rmdir server cmd
void * cmd_mget_server(char * cmdThreadInfor);        //multiple files get from server
int get_file(char *cmd_arg);                           //get file from server
int ip_identify(char serverAddress[30]);
//parameters defined 
#define dataLen 1024
#define threadMAX 200
//declare global variables
pthread_mutex_t printMutex;            		//mutex lock define
char buf[dataLen];
int recv_succ,send_succ,exec_succ;
char currentFilePath[dataLen];				//current filepath
char currentFilePath_server[dataLen];			//give currentfilepath on server
char client_cmd[dataLen];

struct cmdThread{
	pthread_t cmd_thread;
	int valid;
}cThread[threadMAX];

int main()
{
	
	printf("welcome to the ftp client!\n");
	
	//initialization functions called
	InitializeMutex(&printMutex);
	char client_cmd[dataLen],cmd_arg[dataLen];
	memset(currentFilePath,0,sizeof(currentFilePath));
	memset(currentFilePath_server,0,sizeof(currentFilePath_server));
	memset(buf,0,dataLen);
	memset(client_cmd,0,dataLen);
	int i;
	for(i = 0;i < threadMAX;i ++)
		cThread[i].valid = 1;
	
	
	//any user connect to server
	char serverAddress[30];
	printf("Please input the ip of ftp-server:\n");
	scanf("%s",serverAddress);
	//if(ip_identify(serverAddress)!=0)
		//strcpy(serverAddress,"127.0.0.1");//???? testing on localhost
	int ftpPort = 5656;///????
printf("good port for ftp=21 \n");
	
	struct sockaddr_in tServerAddress;
	//printf("success\n");
	tServerAddress.sin_port = htons((unsigned short)ftpPort);
	//printf("success\n");
	tServerAddress.sin_addr.s_addr = inet_addr(serverAddress);
	//printf("success\n");
	tServerAddress.sin_family = AF_INET;
	//printf("success\n");
	
	//printf("success 6\n");
	/*int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock == -1){
		perror("fail at socket()...\n");
		return -1;
	}
	printf("success 7\n");
	int connect_succ = connect(sock,(struct sockaddr*)&address,sizeof(struct sockaddr_in));
	if(connect_succ == -1){
		perror("fail at connect()...\n");
		return -1;
	}
	printf("connected to %s :%d\n",inet_ntoa(address.sin_addr.s_addr),address.sin_port);
	return sock;*/




	int msgSock = connectServer(tServerAddress);
	printf("success\n");
	if(msgSock == -1){
		perror("fail at connectServer()...\n");
		return -1;
	}
		//printf("connected to %s : %d \n",&(tServerAddress.sin_addr.s_addr),htons(&(tServerAddress.sin_port)));

	if(isOverFlow(msgSock) != 1){
		return -1;
	}
	
	char userInfo[100];
	char userName[50];
	char userPassword[50];
	printf("myftp ");
	scanf("%s",userInfo);
//printf("%s\n",userInfo);
	strcpy(userName,getusrName(userInfo));
	strcpy(userPassword,getusrPassword(userInfo));
//printf("%s\n",userName);
//printf("%s\n",userPassword);
	if(check_ip_port(tServerAddress,userInfo)==0)
	{
		printf("invalid server ip or server port\n");
		return -1;		
	}
	//check if valid user AND IP+port entered
	int flag=checkUser(msgSock,userName,userPassword);
	if( flag == -1){
		printf("username doesn't exist or password is error!\n");
		return -1;
	}
	else
		if(flag==-2)
	{
		printf("you can't log in with the same user name!\n");
		return -1;
	}
	else
		printf("you are client %d\n",flag+1);
	//get ftp login status to user-enter commands now
	while(1){
	
		LockMutex(&printMutex);
		printf("myftp>");
		scanf("%s",client_cmd);
		UnlockMutex(&printMutex);
		
		//all client side commands
		if(client_cmd[0] == 'l'&& client_cmd[1]!='s'){
			if(strcmp(client_cmd,"lpwd") == 0){			//lpwd client
				cmd_pwd_client();
				printf("lpwd done...\n");
			}
			else if(strcmp(client_cmd,"ldir") == 0){		//ldir client
				cmd_dir_client();
				printf("ldir done...\n");
			}
			else if(strcmp(client_cmd,"lcd..") == 0){		//lcd up by 1
				cmd_cdback_client();
				printf("lcd.. done...\n");
			}
			else if(strncmp(client_cmd,"lcd",3) == 0){		//lcd display client folder
				//cmd_arg
				char cmd_arg[30];
				get_arg(client_cmd,4,cmd_arg);
				cmd_cd_client(cmd_arg);
				printf("lcd done...\n");
			}
			else if(strncmp(client_cmd,"lmkdir",6) == 0){//lmkdir client
				//cmd_arg
				char cmd_arg[30];
				get_arg(client_cmd,7,cmd_arg);
				cmd_mkdir_client(cmd_arg);
				printf("lmkdir done...\n");
			}
                  else if(strncmp(client_cmd,"lrmdir",6) == 0){//lrmdir client
				//cmd_arg
				char cmd_arg[30];
				get_arg(client_cmd,7,cmd_arg);
				cmd_rmdir_client(cmd_arg);
				printf("lrmdir done...\n");
			}
			else
				printf("bad request,please input again...\n");
		}
		//put cmd
		else{
			if(strncmp(client_cmd,"put",3)!=0 && strncmp(client_cmd,"mput",4)!=0)
			{
				if(strcmp(client_cmd,"quit")==0)
					strcat(client_cmd,userName);							
			send_succ = send(msgSock,client_cmd,dataLen,0);
			if(send_succ == -1){
				printf("fail at send cmd to server...\n");
				break;
			}
			
			//data port receive
			char dataPort_stream[dataLen];
			recv_succ = recv(msgSock,buf,dataLen,0);
			strcpy(dataPort_stream,buf);
			if(recv_succ == -1){
				printf("fail at recv()...\n");
				break;
			}
printf("check \n");
			int dataPort = stream_to_int(dataPort_stream);
			printf("check 1\n");
			//socket param
			struct sockaddr_in tServerAddressData;
			tServerAddressData.sin_port = htons((unsigned short)dataPort);
			tServerAddressData.sin_addr.s_addr = inet_addr(serverAddress);
			tServerAddressData.sin_family = AF_INET;
			int dataSock = connectServer(tServerAddressData);
                        printf("check  \n");
			if(dataSock == -1){
				printf("fail at connectServer() for data...\n");
				break;
			}
			//cmd thread
			int cmdThread_i = get_cmdThread();
			if(cmdThread_i == -1){
				printf("fail at get_cmdThread()...\n");
				return -1;
			}
           		pthread_t cmdThread_server = cThread[cmdThread_i].cmd_thread;
           		//datasocket
           		//:dataSock,cmdThread_i;
           		
           		char dataSock_stream[20],cmdThread_i_stream[20];
           		char cmdThreadInfor[dataLen];
           		
           		int_to_stream(dataSock,dataSock_stream);
			int_to_stream(cmdThread_i,cmdThread_i_stream);
			
			if(strcmp(client_cmd,"pwd") == 0){ 			//pwd cmd 
				//cmd_arg = null  
                   		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
                   		printf("cmdThreadInfor = %s\n",cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_pwd_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else if(strcmp(client_cmd,"ls") == 0){	//ls cmd
				//cmd_arg = null  
                   		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
                   		
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_dir_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else if(strcmp(client_cmd,"cd..") == 0){	//cd cmd
				//cmd_arg = null  
                   		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_cdback_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else if(strncmp(client_cmd,"cd",2) == 0){	//cd command
				char cmd_arg[100];
                  		get_arg(client_cmd,3,cmd_arg);//client_cmdcmd_arg
                  		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_cd_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else if(strncmp(client_cmd,"get",3) == 0){	
				//
				char cmd_arg[100];
                  		get_arg(client_cmd,4,cmd_arg);//client_cmdcmd_arg
                  		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_get_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			/*else if(strncmp(client_cmd,"put",3) == 0){
				//
				char cmd_arg[100];
                  		get_arg(client_cmd,4,cmd_arg);//client_cmdcmd_arg
                  		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_put_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}*/
			else if(strcmp(client_cmd,"?") == 0){		//help function
				//cmd_arg = null  
                   		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_help_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else if(strncmp(client_cmd,"quit",4) == 0){	//for quit client
				//cmd_arg = null  
                   		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,"abc",cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_quit_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
				break;
			}
			else if(strncmp(client_cmd,"mkdir",5) == 0){
				//cmd argument 
				char cmd_arg[100];
                  		get_arg(client_cmd,6,cmd_arg);//client_cmd cmd_arg
                  		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_mkdir_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else if(strncmp(client_cmd,"rmdir",5) == 0){
				//md argument
				char cmd_arg[100];
                  		get_arg(client_cmd,6,cmd_arg);//client_cmd cmd_arg
                  		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_rmdir_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else if(strncmp(client_cmd,"mget",4) == 0){	
				//cmd argument 
				char cmd_arg[100];
                  		get_arg(client_cmd,5,cmd_arg);//client_cmd cmd_arg
                  		makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
				exec_succ = pthread_create(&cmdThread_server,NULL,cmd_mget_server,cmdThreadInfor);
				if(exec_succ != 0){
					printf("fail at pthread_create()...\n");
					return -1;
				}
			}
			else{//bad request
				LockMutex(&printMutex);
				printf("a bad request...\nplease input ? to get help...\n");
				UnlockMutex(&printMutex);
				back_cmdThread(cmdThread_i);
				close(dataSock);
			}
			sleep(1);
			}
			
			else{
				if(strncmp(client_cmd,"put",3)==0)
				{
					char cmd_arg[100];
					char file[dataLen];
					int index=0,j=0;
					get_arg(client_cmd,4,cmd_arg);
					while(cmd_arg[index]!='\0'&&cmd_arg[index]!='-')
						{
							file[j]=cmd_arg[index];
							index++;	j++;
						}
					file[j]='\0';
					if(get_file(file)==0)
					{
						send_succ = send(msgSock,client_cmd,dataLen,0);
						if(send_succ == -1){
							printf("fail at send cmd to server...\n");
							break;
						}
		                               //data port for communication
						char dataPort_stream[dataLen];
						recv_succ = recv(msgSock,buf,dataLen,0);
						strcpy(dataPort_stream,buf);
						if(recv_succ == -1){
							printf("fail at recv()...\n");
							break;
						}
						int dataPort = stream_to_int(dataPort_stream);
					
						//socket param
						struct sockaddr_in tServerAddressData;
						tServerAddressData.sin_port = htons((unsigned short)dataPort);
						tServerAddressData.sin_addr.s_addr = inet_addr(serverAddress);
						tServerAddressData.sin_family = AF_INET;
						int dataSock = connectServer(tServerAddressData);
						if(dataSock == -1){
							printf("fail at connectServer() for data...\n");
							break;
						}
						//getcmd thread
						int cmdThread_i = get_cmdThread();
						if(cmdThread_i == -1){
							printf("fail at get_cmdThread()...\n");
							return -1;
						}
           					pthread_t cmdThread_server = cThread[cmdThread_i].cmd_thread;
           					//socket
           					//:dataSock,cmdThread_i;
           		
           					char dataSock_stream[20],cmdThread_i_stream[20];
           					char cmdThreadInfor[dataLen];
           		
           					int_to_stream(dataSock,dataSock_stream);
						int_to_stream(cmdThread_i,cmdThread_i_stream);
                  			makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
						exec_succ = pthread_create(&cmdThread_server,NULL,cmd_put_server,cmdThreadInfor);
						if(exec_succ != 0){
							printf("fail at pthread_create()...\n");
							return -1;
						}
					}
					else
						printf("There doesn't exist such a file in the current directory.\n");
				}
			else{
						char cmd_arg[100];
						int index=0,index_str,j=0,k=0;
						char file[dataLen];
						char type[20];
						char type_int=0;
						get_arg(client_cmd,5,cmd_arg);

						

							while(cmd_arg[k]!='\0' && cmd_arg[k]!='-')
								k++;
							if(cmd_arg[k]=='\0')
								type_int=0;
							else
							{
								while(cmd_arg[k]!='\0')
								{
									k++;
									type[j]=cmd_arg[k];
									j++;
								}
								type_int=1;
							}						
						while(cmd_arg[index]!='\0'&& cmd_arg[index]!='-')
						{		
							if(index!=0)
								index++;
							index_str=0;
					

							while(cmd_arg[index]!='&' && cmd_arg[index]!='\0' && cmd_arg[index]!='-')
							{		
								file[index_str]=cmd_arg[index];
								index_str++;
								index++;
							}
							file[index_str]='\0';	

				
							if(get_file(file)==0)
							{	
								char cmd[dataLen];
								strcpy(cmd,"put/");//printf("///////////////\n");
								strcat(cmd,file);
								if(type_int==1)
									{strcat(cmd,"-");strcat(cmd,type);}
								send_succ = send(msgSock,cmd,dataLen,0);
								if(send_succ == -1){
									printf("fail at send cmd to server...\n");
									break;
								}
						                             //create dataport to recv data
								char dataPort_stream[dataLen];
								recv_succ = recv(msgSock,buf,dataLen,0);
								strcpy(dataPort_stream,buf);
								if(recv_succ == -1){
									printf("fail at recv()...\n");
									break;
								}
								int dataPort = stream_to_int(dataPort_stream);
			
								//socket param 
								struct sockaddr_in tServerAddressData;
								tServerAddressData.sin_port = htons((unsigned short)dataPort);
								tServerAddressData.sin_addr.s_addr = inet_addr(serverAddress);
								tServerAddressData.sin_family = AF_INET;
								int dataSock = connectServer(tServerAddressData);
								if(dataSock == -1){
									printf("fail at connectServer() for data...\n");
									break;
								}
								//get cmd thread id
								int cmdThread_i = get_cmdThread();
								if(cmdThread_i == -1){
									printf("fail at get_cmdThread()...\n");
									return -1;
								}
           							pthread_t cmdThread_server = cThread[cmdThread_i].cmd_thread;
           							//socket
           							//:dataSock,cmdThread_i;
           		
           							char dataSock_stream[20],cmdThread_i_stream[20];
           							char cmdThreadInfor[dataLen];
           		
           							int_to_stream(dataSock,dataSock_stream);
								int_to_stream(cmdThread_i,cmdThread_i_stream);
									
									if(type_int==1)
									{strcat(file,"-");strcat(file,type);}
                  			makeUp_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,file,cmdThreadInfor);
						exec_succ = pthread_create(&cmdThread_server,NULL,cmd_put_server,cmdThreadInfor);
								if(exec_succ != 0){
									printf("fail at pthread_create()...\n");
									return -1;
								}
							}
							else
								printf("There doesn't exist file %s in the current directory.\n",file);
				sleep(1);

						}
						

				}//mput
			sleep(1);
			}//put mput else
			
		}//end
	}
	
	
	printf("I am out...\n");
	close(msgSock);
	return 0;
}

//pthread mutex initialization
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

//uname info
char *getusrName(char *userInfo)
{
	int i;
	char userName[50];
	for(i=0;userInfo[i]!='\0' && userInfo[i]!=':';i++)
		userName[i]=userInfo[i];
	if(userInfo[i]==':')
		userName[i]='\0';
	else
		return NULL;
	return userName;
}
//get password info for user
char *getusrPassword(char *userInfo)
{
	int i=0;
	int j;
	char userPassword[50];
	while(userInfo[i]!=':'&& userInfo[i]!='\0')
		i++;
	if(userInfo[i]=='\0')
		return NULL;
	else
		i++;
	if(userInfo[i]!='\0')
	for(j=0;userInfo[i]!='\0' && userInfo[i]!='@';i++,j++)
		userPassword[j]=userInfo[i];
	else return NULL;
	if(userInfo[i]=='@')
		userPassword[j]='\0';
	else
		return NULL;
	return userPassword;
}
//check if valid ip
int check_ip_port(struct sockaddr_in address,char *userInfo)
{	
	int i=0;
	int j;
	char serverip[50],serverport[50];
	while(userInfo[i]!='@'&& userInfo[i]!='\0')
		i++;
	if(userInfo[i]=='\0')
		return 0;
	else
		i++;
	if(userInfo[i]!='\0')
	for(j=0;userInfo[i]!='\0' && userInfo[i]!=':';i++,j++)
		serverip[j]=userInfo[i];
	else return 0;
	if(userInfo[i]==':')
		serverip[j]='\0';
	else
		return 0;
	if(strcmp(inet_ntoa(address.sin_addr),serverip)!=0)
		return 0;

	i++;
	for(j=0;userInfo[i]!='\0';i++,j++)
		serverport[j]=userInfo[i];
	serverport[j]='\0';
	if(address.sin_port!=stream_to_int(serverport))
		return 0;
	return 1;
	
}
//connect to server
int connectServer(struct sockaddr_in address){

	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock == -1){
		perror("fail at socket()...\n");
		return -1;
	}
	int connect_succ = connect(sock,(struct sockaddr*)&address,sizeof(struct sockaddr_in));
	
	if(connect_succ == -1){
		perror("fail at connect()...\n");
		return -1;
	}

       
	printf("connected to %s : %d\n",inet_ntoa(address.sin_addr),address.sin_port);

return sock;
}
//verify user
int checkUser(int sock,char* userName,char* userPassword){
	//char userName[dataLen],userPassword[dataLen];
	
	/*printf("please input the userName:");
	scanf("%s",userName);*/
	strcpy(buf,userName);
	send_succ = send(sock,buf,dataLen,0);
	if(send_succ == -1){
		printf("fail at send()...\n");
		return -1;
	}

	/*printf("please input the userPassword:");
	scanf("%s",userPassword);*/	
	strcpy(buf,userPassword);
	send_succ = send(sock,buf,dataLen,0);
	if(send_succ == -1){
		printf("fail at send()...\n");
		return -1;
	}
	
	char isUser[dataLen];
	
	recv_succ = recv(sock,buf,dataLen,0);
	strcpy(isUser,buf);
	if(recv_succ ==-1){
		printf("fail at recv()...\n");
		return -1;
	}
	if(strcmp(isUser,"-1")==0){
		//printf("an invalid user!\n");
		return -1;
	}
	else
		if(strcmp(isUser,"-2")==0)
			return -2;
	else
		return stream_to_int(isUser);
}					
//client connection exceed
int isOverFlow(sock){
	char refuse[dataLen];
	recv_succ = recv(sock,buf,dataLen,0);
	if(recv_succ ==-1){
		printf("fail at recv()...\n");
		return -1;
	}
	strcpy(refuse,buf);
	printf("%s\n",refuse);
	if(refuse[0] == 't'){
		printf("%s\n",refuse);
		return 0;
	}
	return 1;
}					

//stream to int
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

//int to steam
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

//client_cmd cmd_arg
void get_arg(char *b,int a,char *c){
	int i;
	for(i = a;b[i] != '\0';i ++)
		c[i - a] = b[i];
	c[i - a] = '\0';
}	
//verify
int get_cmdThread(){
	int i;
	for(i = 1;i < threadMAX;i ++)
		if(cThread[i].valid == 1){
			cThread[i].valid = 0;
			return i;
		}
	return -1;
}

//end cmd thr				  	
void back_cmdThread(int i){
	cThread[i].valid = 1;
}
//
void makeUp_cmdThreadInfor(char *a,char *b,char *c,char *d){
	strcpy(d,a);
	strcat(d,"/");
	strcat(d,b);
	strcat(d,"/");
	strcat(d,c);
}

//break the cmd entered into parts								
void break_cmdThreadInfor(char *a,char *b,char *c,char *d){
	int i;
	for(i = 0;d[i] != '/';i ++)
		a[i] = d[i];
	a[i] = '\0';
	
	int j;
	for(j = i + 1;d[j] != '/';j ++)
		b[j - i - 1] = d[j];
	b[j - i - 1] = '\0';
	
	int k;
	for(k = j + 1;d[k] != '\0';k ++)
		c[k - j - 1] = d[k];
	c[k - j - 1] = '\0';
}
								
				  	
//get dir path for pwd
void cmd_pwd_client(){
	if((int)currentFilePath[0] == 0)
     		getcwd(currentFilePath,sizeof(currentFilePath));
     	LockMutex(&printMutex);
     	printf("current file path in client is %s\n",currentFilePath);
     	UnlockMutex(&printMutex);
}					

//dir cmd for client
void cmd_dir_client(){
	DIR *pdir;   
     	char fileName[30];  
     	char fileInfo[50];  
     	int i, fcounts = 0, len;  
     	struct dirent *pent;  
     	int fd;  
     	struct stat fileSta;  
     	char filePath[200];  
      
      	if((int)currentFilePath[0] == 0)
      		getcwd(currentFilePath,sizeof(currentFilePath));
     	pdir = opendir(currentFilePath);  
     	pent = readdir(pdir);  
       
     	while(pent!=NULL){  
         	fcounts++;  
         	pent=readdir(pdir);  
     	}
     	closedir(pdir);  
   	printf("the number of files is = %d\n",fcounts);
     	if(fcounts <= 0)  
         	return; 
     	else{
     		if((int)currentFilePath[0] == 0)
     			getcwd(currentFilePath,sizeof(currentFilePath));
         	pdir=opendir(currentFilePath); 
         	LockMutex(&printMutex);
         	for(i = 0;i < fcounts;i++){
         		pent=readdir(pdir);
         		memset(fileName,0,30);
         		memset(fileInfo,0,sizeof(fileInfo));
         		strcpy(fileName,pent->d_name); 
               
             	//check the file is a directory or a file 
             	memset(filePath,0,sizeof(filePath));  
             	strcpy(filePath,currentFilePath);  
             	strcat(filePath,"/");  
             	strcat(filePath,fileName);  
             	fd = open(filePath,O_RDONLY, S_IREAD);  
               
             	fstat(fd,&fileSta);  
             	if(S_ISDIR(fileSta.st_mode)){  
                 		strcat(fileInfo,"dir\t");  
                 		strcat(fileInfo,fileName);  
             		}  
             	else{     
                 		strcat(fileInfo,"file\t");  
                 		strcat(fileInfo,fileName);  
             		}   
			printf("%s\n",fileInfo);
         	}  
         	UnlockMutex(&printMutex);
         	closedir(pdir);
         }
}					

//one dir up
void cmd_cdback_client(){
	
	int len;  
     	int i, record;  
   	
   	if((int)currentFilePath[0] == 0)
     		getcwd(currentFilePath,sizeof(currentFilePath));
     	
     	len = strlen(currentFilePath);  
       
     	for(i = len - 1;i >= 0;i --){
     		if(currentFilePath[i] == '/'){  
             	currentFilePath[i] = '\0';  
             	break;  
         	}  
         	currentFilePath[i] = '\0';  
     	}
     	LockMutex(&printMutex);
     	printf("current file path in client is %s\n",currentFilePath);
     	UnlockMutex(&printMutex);
}				

//cd cmd 
void cmd_cd_client(char *cmd_arg){
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0;  
     	int flag=0;
     	  
     	if((int)currentFilePath[0] == 0)
     		getcwd(currentFilePath,sizeof(currentFilePath));
     	pdir = opendir(currentFilePath); 
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
     		if((int)currentFilePath[0] == 0)
     			getcwd(currentFilePath,sizeof(currentFilePath));
     		pdir = opendir(currentFilePath);  
         	for(i = 0;i < fcounts;i ++){
         		pent = readdir(pdir);
         		if(strcmp(pent->d_name,cmd_arg) == 0){  
                 		strcat(currentFilePath,"/");  
                 		strcat(currentFilePath,cmd_arg);  
                 		flag = 1;  
                 		break;  
             		}  
     		}
     	}
     	closedir(pdir);
     	
    	if(flag == 1){
    		LockMutex(&printMutex);  
             printf("current file path in client is %s\n",currentFilePath);
             UnlockMutex(&printMutex);
     	}
     	else{
     		LockMutex(&printMutex);
     		printf("the folder you want to cd does not exist!\n");
     		UnlockMutex(&printMutex);
     	}
}				

//mkdir client cmd
void cmd_mkdir_client(char *cmd_arg){
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0;  
     	int flag = 1;
     	char mkdir_filePath[dataLen];
	
	if((int)currentFilePath[0] == 0)
     		getcwd(currentFilePath,dataLen);  //currentfilepath
     	pdir = opendir(currentFilePath); 
     	pent = readdir(pdir);  
     	while(pent != NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     	closedir(pdir);  
   
     	if(fcounts < 0){  
		printf("mkdir fails...\n");
         	return;  
     	}
     	else{  
     		if(currentFilePath[0] == 0)
     			getcwd(currentFilePath,dataLen);
     		pdir = opendir(currentFilePath);  
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
      	
      	if(flag == 1){//mkdir filepath
      		strcpy(mkdir_filePath,currentFilePath);  
            	strcat(mkdir_filePath,"/");  
            	strcat(mkdir_filePath,cmd_arg); 
           	//mkdir_filePath
      		
      		int mkdir_succ = mkdir(mkdir_filePath,0);
      		if(mkdir_succ != 0){
      			LockMutex(&printMutex);
      			printf("mkdir fails...\n");
      			UnlockMutex(&printMutex);
      		}
      		else{	
      			LockMutex(&printMutex);
      			printf("mkdir succeeds...\n");
      			UnlockMutex(&printMutex);
      		}	
      	}
      	else{//fail, already exist
      		LockMutex(&printMutex);
      		printf("There exists a dir with the same name\n");
      		UnlockMutex(&printMutex);
      	}
}

//rmdir for client
void cmd_rmdir_client(char *cmd_arg){
	
	DIR *pdir;  
     	struct dirent *pent;  
     	char filename[30];  
     	int i,fcounts = 0,fcounts_1=0;  
     	int flag = 0;
     	char rmdir_filePath[dataLen];
	
	if((int)currentFilePath[0] == 0)
     		getcwd(currentFilePath,dataLen);  //getcwd currentfilepath
     	pdir = opendir(currentFilePath); 
     	pent = readdir(pdir);  
     	while(pent != NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     	closedir(pdir);  
   
     	if(fcounts < 0){
		printf("rmdir fails...\n");  
         	return;  
     	}
     	else{  
     		if(currentFilePath[0] == 0)
     			getcwd(currentFilePath,dataLen);
     		pdir = opendir(currentFilePath);  
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
      	
      	if(flag == 1){//filepath for rmdir cmd
      		strcpy(rmdir_filePath,currentFilePath);  
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
				printf("rmdir fails...\n");  
         			return;  
     			}
			if(fcounts_1>0)
			{
				printf("This directory is not empty,you can't remove it\n");
				return;
			}
      		int rmdir_succ = rmdir(rmdir_filePath);
			if(rmdir_succ==0)
			{
				LockMutex(&printMutex);
      			printf("rmdir succeeds...\n");
      			UnlockMutex(&printMutex);return;
			}
			else
			{
				LockMutex(&printMutex);
      			printf("rmdir fails...\n");
      			UnlockMutex(&printMutex);return;
			}
		}
		else {//dir fail
      		LockMutex(&printMutex);
      		printf("There doesn't exist a dir with this name\n");
      		UnlockMutex(&printMutex);return;
      		}
      		
}

//print working directory
void *cmd_pwd_server(char *cmdThreadInfor){

	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	//pwd
	recv_succ = recv(dataSock,currentFilePath_server,dataLen,0);
	printf("recv_succ = %d\n",recv_succ);
	if(recv_succ == -1){
		printf("fail at recv() while receiving filePath from the server!\n");
		return;
	}
	LockMutex(&printMutex);
	printf("the current file path in server is %s\n",currentFilePath_server);
	UnlockMutex(&printMutex);
	
	//back cmd thread
	back_cmdThread(cmdThread_i);
	close(dataSock);
}

//server directory		
void *cmd_dir_server(char *cmdThreadInfor){
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	int fcounts = 0;
	char fcounts_stream[dataLen];
	
	recv(dataSock,buf,dataLen,0);
	if(buf[0] == '0'){
		printf("no file or dir exists!\n");
	}
	else{		
		recv_succ = recv(dataSock,fcounts_stream,dataLen,0);//recv data 
		if(recv_succ == -1){
			printf("fail at recv() while recving command dir from the sever1\n");
		}
		LockMutex(&printMutex);
		printf("the number of file is = %s \n",fcounts_stream);
		fcounts = stream_to_int(fcounts_stream);
	
		char fileInfo[dataLen];
		int i;
		for(i = 0;i < fcounts;i ++){
			recv_succ = recv(dataSock,fileInfo,dataLen,0);
			printf("%s\n",fileInfo);
		}
		UnlockMutex(&printMutex);
	}
	
	
	back_cmdThread(cmdThread_i);
	close(dataSock);
}			

//cd cmd			
void *cmd_cd_server(char *cmdThreadInfor){
	
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
// recv data from socke
	recv(dataSock,buf,dataLen,0);
	if(buf[0] == '0'){
		LockMutex(&printMutex);
		printf("no file or dir exists under %s in the server!\n",currentFilePath_server);
		UnlockMutex(&printMutex);
	}
	else{
		recv(dataSock,buf,dataLen,0);
		if(buf[0] == '0'){
			LockMutex(&printMutex);
			printf("%s does not exist under %s in the server!\n",cmd_arg,currentFilePath_server);
			UnlockMutex(&printMutex);
		}
		else{
			recv(dataSock,currentFilePath_server,dataLen,0);
			LockMutex(&printMutex);
			printf("the current file path in server is %s\n",currentFilePath_server);
			UnlockMutex(&printMutex);
		}
	}

	//back cmd
	back_cmdThread(cmdThread_i);
	close(dataSock);
}

//move on dir up
void *cmd_cdback_server(char *cmdThreadInfor){
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	recv(dataSock,currentFilePath_server,dataLen,0);
	LockMutex(&printMutex);
	printf("the current file path in server is %s\n",currentFilePath_server);
	UnlockMutex(&printMutex);
	
	//back cmd
	back_cmdThread(cmdThread_i);
	close(dataSock);
}

		

//copy /transfer from server
void *cmd_get_server(char *cmdThreadInfor){
	
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	char tran_type[20];
	int trans=0; 

	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	//receiving from server
	recv(dataSock,buf,dataLen,0);
	if(buf[0] == '0'){//failed
		LockMutex(&printMutex);
		printf("%s does not exist under %s in the server!\n",cmd_arg,currentFilePath_server);
		UnlockMutex(&printMutex);
	}
	else{
		int fd;  
     		long fileSize; 
     		char fileSize_stream[dataLen];     	
      		if((int)currentFilePath[0] == 0)
      			getcwd(currentFilePath,sizeof(currentFilePath));//getcwd current file
     		char get_filePath[200];
     		char get_fileName[30];

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

     		strcpy(get_fileName,cmd_arg);
     		strcpy(get_filePath,currentFilePath);  
     		strcat(get_filePath,"/");
     		strcat(get_filePath,get_fileName);
      
     		if(trans==0)
      			fd = open(get_filePath,O_RDWR|O_CREAT, S_IREAD|S_IWRITE);   //open file
			else 
				fd= fopen(get_filePath,"wb");
     		if(fd != -1){     
         		 
         		recv(dataSock,fileSize_stream,dataLen,0);  
  			fileSize = stream_to_int(fileSize_stream);//filesize in integer
         		memset(buf,0,dataLen); 
         		while(fileSize > dataLen){  
            			recv(dataSock,buf,dataLen,0);  
            			write(fd,buf,dataLen);  
            			fileSize = fileSize - dataLen;  
         		}  
          
         		recv(dataSock,buf,fileSize,0);  
         		write(fd,buf,fileSize);  
         		close(fd);
         		LockMutex(&printMutex);  
         		printf("get the file %s from the server!\n",get_fileName);  
         		UnlockMutex(&printMutex);
     		}  
     		else{
     			LockMutex(&printMutex);
         		printf("open or create file %s failed\n",get_filePath);
         		UnlockMutex(&printMutex);
         	}
         }
	
	//backk cmd
	back_cmdThread(cmdThread_i);
	close(dataSock);
}

//put cmd copy files to server	
void *cmd_put_server(char *cmdThreadInfor){
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	char tran_type[20];
	int trans=0; 
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	//cmd_arg[]
	recv(dataSock,buf,dataLen,0);
	if(buf[0] == '0'){//file exists...
		LockMutex(&printMutex);
		printf("%s exists under %s in the server!\n",cmd_arg,currentFilePath_server);
		UnlockMutex(&printMutex);
	}
	else{
		int fd;  
     		struct stat fileSta;  
     		long fileSize;
     		char fileSize_stream[dataLen];
     		char put_filePath[200],put_fileName[30];  
   		
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
			if(strcmp(tran_type,"binary")==0)     //for binary 1
				trans=1;

   		if((int)currentFilePath[0] == 0)
      			getcwd(currentFilePath,sizeof(currentFilePath));//get current filepath
      		strcpy(put_filePath,currentFilePath);
   		strcpy(put_fileName,cmd_arg);
   		strcat(put_filePath,"/");
   		strcat(put_filePath,cmd_arg);
   		
     			if(trans==0)
      			fd = open(put_filePath,O_RDONLY, S_IREAD);   //open file
			else 
				fd= fopen(put_filePath,"rb");
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
         		LockMutex(&printMutex);
         		printf("put the file %s to the server!\n",put_filePath);
         		UnlockMutex(&printMutex);  
     		}  
     		else{
     			LockMutex(&printMutex);
      	   		printf("open file %s failed\n",put_filePath);
      	   		UnlockMutex(&printMutex);
      	   	}
	}

	//backout cmd thread
	back_cmdThread(cmdThread_i);
	close(dataSock);
}		

//help cmd "--help"
void *cmd_help_server(char *cmdThreadInfor){
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	char help[dataLen];
	recv(dataSock,help,dataLen,0);
	
	LockMutex(&printMutex);
	printf("%s\n",help);
	UnlockMutex(&printMutex);
	
	back_cmdThread(cmdThread_i);
	close(dataSock);
}

//quit the server for each 			
void *cmd_quit_server(char *cmdThreadInfor){
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	char quit[dataLen];
	recv(dataSock,quit,dataLen,0);
	if(strcmp(quit,"quit") != 0)
		sleep(10);
	else{
		LockMutex(&printMutex);
		printf("I am leaving...\n");
		UnlockMutex(&printMutex);
	}
	//back cmd
	back_cmdThread(cmdThread_i);
	close(dataSock);
}			

//make directory command for user
void *cmd_mkdir_server(char *cmdThreadInfor){
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	recv(dataSock,buf,dataLen,0);
	if(strcmp(buf,"1") == 0){
		LockMutex(&printMutex);
		printf("%s is created in the server...\n",cmd_arg);
		UnlockMutex(&printMutex);
	}
	else if(strcmp(buf,"0") == 0){
		LockMutex(&printMutex);
		printf("Create dir %s fails...\n",cmd_arg);
		UnlockMutex(&printMutex);
	}
	else if(strcmp(buf,"2") == 0){
		LockMutex(&printMutex);
		printf("Create dir fail!There exists such a dir in server already\n",cmd_arg);
		UnlockMutex(&printMutex);	
	}
	
	//back valid cmd
	back_cmdThread(cmdThread_i);
	close(dataSock);
}	

//remove directory command 
void *cmd_rmdir_server(char *cmdThreadInfor){
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);
	
	recv(dataSock,buf,dataLen,0);
	if(strcmp(buf,"1") == 0){
		LockMutex(&printMutex);
		printf("%s has been removed in the server...\n",cmd_arg);
		UnlockMutex(&printMutex);
	}
	else if(strcmp(buf,"0") == 0){
		LockMutex(&printMutex);
		printf("Remove dir %s fails...\n",cmd_arg);
		UnlockMutex(&printMutex);
	}
	else if(strcmp(buf,"2") == 0){
		LockMutex(&printMutex);
		printf("Dir %s in server is not empty...\n",cmd_arg);
		UnlockMutex(&printMutex);
	}
	else if(strcmp(buf,"3") == 0){
		LockMutex(&printMutex);
		printf("There doesn't exist such a dir in server\n",cmd_arg);
		UnlockMutex(&printMutex);	
	}
	//back valid
	back_cmdThread(cmdThread_i);
	close(dataSock);
}			

//for copying/sending multiple files	
void *cmd_mget_server(char *cmdThreadInfor){
	
	//cmdThreadInfor,dataSock_stream,cmdThread_i_stream,cmd_arg,
	char dataSock_stream[20],cmdThread_i_stream[20],cmd_arg[100];
	int dataSock,cmdThread_i;
	char tran_type[20];
	int trans=0; 

	break_cmdThreadInfor(dataSock_stream,cmdThread_i_stream,cmd_arg,cmdThreadInfor);
	dataSock = stream_to_int(dataSock_stream);
	cmdThread_i = stream_to_int(cmdThread_i_stream);

	//filepath 
		int fd;  
     		long fileSize; 
     		char fileSize_stream[dataLen];     	
      		if((int)currentFilePath[0] == 0)
      			getcwd(currentFilePath,sizeof(currentFilePath));//get existing filepath
     		char get_filePath[200];
     		char get_fileName[30];

     		int j,k,m;
		for(j=0;cmd_arg[j]!='\0';j++)     //format into ascii
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
			if(strcmp(tran_type,"binary")==0)     //1 for binary
				trans=1;
	int index=0,index_str;
	char file[dataLen];
	while(cmd_arg[index]!='\0')				//EOF
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
	
	//recv data on socket
	recv(dataSock,buf,dataLen,0);
	if(buf[0] == '0'){//doesnt exist on server
		LockMutex(&printMutex);
		printf("%s does not exist under %s in the server!\n",file,currentFilePath_server);
		UnlockMutex(&printMutex);
	}
	else{

     		strcpy(get_fileName,file);
     		strcpy(get_filePath,currentFilePath);  
     		strcat(get_filePath,"/");
     		strcat(get_filePath,get_fileName);
      
     		if(trans==0)
      			fd = open(get_filePath,O_RDWR|O_CREAT, S_IREAD|S_IWRITE);   //open file
			else 
				fd= fopen(get_filePath,"wb");
     		if(fd != -1){     
         		 
         		recv(dataSock,fileSize_stream,dataLen,0);  
  			fileSize = stream_to_int(fileSize_stream);//convert file size
         		memset(buf,0,dataLen); 
         		while(fileSize > dataLen){  
            			recv(dataSock,buf,dataLen,0);  
            			write(fd,buf,dataLen);  
            			fileSize = fileSize - dataLen;  
         		}  
          
         		recv(dataSock,buf,fileSize,0);  
         		write(fd,buf,fileSize);  
         		close(fd);
         		LockMutex(&printMutex);  
         		printf("get the file %s from the server!\n",file);  
         		UnlockMutex(&printMutex);
     		}  
     		else{
     			LockMutex(&printMutex);
         		printf("open or create file %s failed\n",get_filePath);
         		UnlockMutex(&printMutex);
         	     }
        	 }
	}
	
	
	back_cmdThread(cmdThread_i);
	close(dataSock);
}

//get file from passed arg
int get_file(char *cmd_arg)
{//printf("%s\n",cmd_arg);
	DIR *pdir;  
     	struct dirent *pent;
	int fcounts=0; 
	char get_filePath[dataLen];

	if((int)currentFilePath[0] == 0)
     		getcwd(currentFilePath,sizeof(currentFilePath));
     	LockMutex(&printMutex);
     	printf("current file path in client is %s\n",currentFilePath);
     	UnlockMutex(&printMutex);

	pdir = opendir(currentFilePath);
     	pent=readdir(pdir);  
     	while(pent!=NULL){  
         	fcounts ++;  
         	pent = readdir(pdir);  
     	}
     
     	closedir(pdir);  
   
     	if(fcounts <= 0){  
         	return -1;  
     	}

	pdir = opendir(currentFilePath);     //Open directory cmd_arg
	int i;
	for(i = 0;i < fcounts;i ++){
			pent = readdir(pdir);
         		if(strcmp(pent->d_name,cmd_arg) == 0){printf("%d,%s\n",i,pent->d_name);
         			strcpy(get_filePath,currentFilePath);  
                 		strcat(get_filePath,"/");  
                 		strcat(get_filePath,cmd_arg); 
                 		//get_filePath
                 		return 0; 
             			} 
      			}
	return -1;
}

//identify IP addess as valid
int ip_identify(char serverAddress[30])
{
	int i,j,value;
	char value_char[20];
	for(i=0;serverAddress[i]!='\0'&&i <15;i++)
	{
		j=0;
		while(serverAddress[i]!='.'&&serverAddress[i]!='\0')
		{
			value_char[j]=serverAddress[i];
			i++;
			j++;
		}
		value_char[j]='\0';
		value=stream_to_int(value_char);
		if(value>255)
			return -1;
		if(serverAddress[i]=='\0')
			break;
	}
	if(i<15)
		return 0;
	else 
		return -1;
}
