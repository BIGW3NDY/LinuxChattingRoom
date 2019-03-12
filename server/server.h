#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <signal.h>
#define BACKLOG 5
#define MSGKEY 1024
#define LOGIN 1
#define MESSAGE 2
#define LOGOUT 3

const char* ip = "127.0.0.1";

struct msgbuf{
	long mtype;//�ӽ���pid
	char mtext[1024];//��Ϣ����
	int mclass; //��½����Ϣ���ǳ�
	char username[100];	
	int user_pid;
	//struct USER user; //�û���Ϣ
};


void getAddr(char *port,struct sockaddr_in* my_addr){
    int p = atoi(port);
    my_addr->sin_family = AF_INET;
    my_addr->sin_port = htons(p);
    my_addr->sin_addr.s_addr = inet_addr(ip);
}

void write_queue(int msgqid,struct msgbuf* msg,int flags){
    msgsnd(msgqid,msg,sizeof(struct msgbuf),flags);
    //if(msgsnd>0)
	//		printf("to %ld send:%s  %d\n",msg->mtype,msg->mtext,msg->mclass);
}

void read_queue(int msgqid,struct msgbuf* msg,int type,int flags){
	int t = msgrcv(msgqid,msg,sizeof(struct msgbuf),type,flags);
	//printf("from %d msgrcv:%d  %s\n",type,t,msg->mtext);	
	if(t<0)
		perror("msgrcv");
}

int create_IPCqueue(char* path, char id){
    key_t key;
	if((key=ftok(".",'a'))==-1){
		perror("ftok");
		exit(1);
	}

	int msgqid;
	msgctl(msgqid,IPC_RMID,0);
	msgqid = msgget(key,IPC_CREAT|0666);
	if(msgqid<0){
		printf("��Ϣ���д���ʧ��");
		exit(1);
	}
	return msgqid;
}

//�ӿͻ��˽�����Ϣ����д����Ϣ���й������̶�ȡ
int receive_from_client(int msgqid,int accept_fd,char* recvstr,int datasize,struct msgbuf* msg){
	int recvbytes = recv(accept_fd,recvstr,datasize,0);
	if(recvbytes == -1){
		perror("receiveʧ��");
		//continue
	}
	recvstr[recvbytes] = '\0';
	if(recvstr[0]=='*'){
		//printf("11\n");
		//���������û�����		
		//user[online_cnt++] = getppid();
		//printf("online_cnt:%d\n",online_cnt);
		//����Ϣ�ṹ����д��user��Ϣ		
		strcpy(msg->username,recvstr);
		msg->user_pid = getpid();//uer_pid Ϊ send_msg ���̺�
		//printf("add:%s %d\n",msg->username,msg->user_pid);		
		msg->mtype = 1;	
		//����Ϣ�ṹ�����Ϣ�ַ�����д��������Ϣ
		strcpy(msg->mtext," login");
		msg->mclass = LOGIN;
		//����Ϣд����Ϣ����		
		write_queue(msgqid,msg,IPC_NOWAIT);
	}

	else if(strcmp(recvstr,"QUIT")==0){
		//����Ϣ�ṹ�����Ϣ�ַ�����д��������Ϣ    	
		strcpy(msg->mtext," logout");
    	msg->mclass= LOGOUT;
		//����Ϣд����Ϣ����		
		write_queue(msgqid,msg,IPC_NOWAIT);
		//�˳����̣��������ܿͻ�����Ϣ
		exit(1);
	}
	
	else{
		//����Ϣ�ṹ����Ϣ�ַ�����д�ϴӿͻ��˽��ܵ�����Ϣ
		strcpy(msg->mtext,recvstr);
		msg->mclass = MESSAGE;
		//printf("write_queue:%s %d\n",msg->mtext,msg->mclass);
		//����Ϣд����Ϣ����
		write_queue(msgqid,msg,IPC_NOWAIT);
	}

	return recvbytes;
}


//����Ϣ�����а�mtype��ȡ�������Կͻ��˵���Ϣ�����ҷ���
int send_to_client(int msgqid,int accept_fd,struct msgbuf *msg_out){
	//����Ϣ�����ж�ȡ��Ϣ	
	int type = getppid();	
	read_queue(msgqid,msg_out,type,0);						
	int s;
	char msg[1024];	
	//��ͻ��˷�����Ϣ
	if(msg_out->mclass == LOGOUT){	
		if(msg_out->user_pid == type){
			send(accept_fd,"QUIT",strlen(msg),0);
			exit(1);		
		}
		strcpy(msg,msg_out->username);
		strcat(msg,msg_out->mtext);		
		s = send(accept_fd,msg,strlen(msg),0);		
	}	
	
	else if(msg_out->mclass == LOGIN){
		//printf("user come in\n");
		strcpy(msg,msg_out->username);
		strcat(msg,msg_out->mtext);
		//printf("%s\n",msg);	
		s = send(accept_fd,msg,strlen(msg),0);
	}
	
	else{
		strcpy(msg,msg_out->username);
		strcat(msg,": ");
		strcat(msg,msg_out->mtext);
		s = send(accept_fd,msg,strlen(msg),0);	
	}
	return s;
}
