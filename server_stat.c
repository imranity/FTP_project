#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

int count_current();
int count_all();
void list();
void kill_username(char *userName);
void quit();		
int getuserName(char *cmd,char* userName);
void quit();

#define maxUser 3

typedef struct 
{	
	char userName[50];
	char userPassword[50];
	int login;
	int msgSock;
}user;

int main()
{
	char cmd[50];
	char userName[50];
	//char *void_str=NULL;
	printf("Welcome to look up the ftp_server information\n");
	printf("Please input the command:");
	scanf("%s",cmd);
        printf("command received \n");
	while(strcmp(cmd,"stop")!=0)
	{
		if(strcmp(cmd,"count_current")==0)
{printf("SUCCESS BEFORE \n");			
printf("The amount of current users is:%d\n",count_current());
        printf("SUCCESS AFTER \n");}
		else 
            if(strcmp(cmd,"count_all")==0)
			printf("The amount of users in total is:%d\n",count_all());
		else 
		if(strcmp(cmd,"list")==0)
			{printf("showing all users \n");
list();
printf("completed\n");}		else 
		if(strncmp(cmd,"kill",4)==0)
		{	
			if(getuserName(cmd,userName)==0)
			{
				kill_username(userName);}
			else
				printf("Wrong command!\n");
		}
		else 
		if(strcmp(cmd,"quit")==0)
			quit();
		else printf("No such command!\n");
		printf("Please input the command:");
		scanf("%s",cmd);
	}
	return 0;
}

int count_current()
{
	int shm_id;
	key_t key;
	int *temp;
	char *name="/etc"; 
printf("SUCCESS name \n");			
	key=ftok(name,0);
	if(key==-1)
		perror("ftok error");
printf("SUCCESS key \n");			
	shm_id=shmget(key,1024,IPC_CREAT);
   printf("SUCCESS shmid \n");			
	if(shm_id==-1)
	{
		//perror("shmget error");
		return -1;
	}
      //printf("%d\n",shm_id);
	temp=(int *)shmat(shm_id,NULL,0);
   printf("SUCCESS temp \n");			
	return temp;
}

int count_all()
{
	int shm_id;
	key_t key;
	int *temp;
	char *name="/etc"; 
	key=ftok(name,0);
	if(key==-1)
		perror("ftok error");
	shm_id=shmget(key,1024,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return -1;
	}
      //printf("%d\n",shm_id);
	temp=(int *)shmat(shm_id,NULL,0);
	return temp;
}

void list()
{
	int i;
	int shm_id;
	key_t key;
	user *temp;
	char *name="/etc"; 
	key=ftok(name,1);
	if(key==-1)
		perror("ftok error");
	shm_id=shmget(key,4096,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	temp=(user *)shmat(shm_id,NULL,0);
	//printf("%d\n",shm_id);printf("%d\n",sizeof(temp));
	printf("The active users are \n");
	for(i=0;i<maxUser;i++)
		{if(temp[i].login==1)
			printf("%s\n",(*(temp+i)).userName);}
	return;
}

void kill_username(char *userName)
{
	int i;
	int shm_id;
	key_t key;
	char *temp;
	char *name="/etc"; 
	key=ftok(name,2);
	if(key==-1)
		perror("ftok error");
	shm_id=shmget(key,4096,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	temp=(char *)shmat(shm_id,NULL,0);
	
	
	int shm_id_1;
	key_t key_1;
	user *temp_1;
	//char *name="/root"; 
	key_1=ftok(name,1);
	//if(key_1==-1)
	//	perror("ftok error");
	shm_id_1=shmget(key_1,4096,IPC_CREAT);
	if(shm_id_1==-1)
	{
		//perror("shmget error");
		return;
	}
	temp_1=(user *)shmat(shm_id_1,NULL,0);
	//printf("%d\n",shm_id_1);
	//printf("%s\n",temp_1);
	for(i=0;i<maxUser;i++)
		if(strcmp(userName,(*(temp_1+i)).userName)==0)
			break;
	if(i==maxUser)
	{
		printf("There is no such user\n");
		return; 
	}
	if((*(temp_1+i)).login==0)
	{	
		printf("The user hasn't logged in\n");
		return;	
	}
	memcpy(temp,userName,strlen(userName)+1);

	return;
}

int getuserName(char *cmd,char* userName)
{
	int i=0;
	//char *userName;
	while(cmd[i]!='_' && cmd[i]!='\0')
		i++;
	if(cmd[i]=='\0')
	{
		//printf("Wrong command\n");
		return -1;
	}
	i++;
	int j=0;
	while(cmd[i]!='\0')
	{	
		userName[j]=cmd[i];
		i++;
		j++;
	}
	userName[j]='\0';	
	return 0;
}

void quit()
{
	int i;
	int shm_id;
	key_t key;
	user *temp;
	char *name="/etc"; 
	key=ftok(name,4);
	//if(key==-1)
	//	perror("ftok error");
	shm_id=shmget(key,4096,IPC_CREAT);
	if(shm_id==-1)
	{
		//perror("shmget error");
		return;
	}
	temp=(user *)shmat(shm_id,NULL,0);
	//printf("%d\n",shm_id);
	memcpy(temp,"quit",sizeof("quit"));
	//printf("%s\n",temp);
	return;
}
