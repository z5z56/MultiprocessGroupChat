#include <iostream>
#include<unistd.h>//unix stand lib
#include<sys/types.h>
#include<sys/fcntl.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>
#include<dirent.h>//file dir
#include <sys/wait.h>//wait func
#include <stdlib.h>//ststem
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/socket.h>//socket
#include <netinet/in.h>
#include <vector>
#include <errno.h>
using namespace std;
/*
	������ҵ��
	    �Ͽδ��룬ͬʱΪ��ҵһ
		 1.ʵ��һ��tcp��������N��tcp�ͻ��ˣ�ÿ���ͻ��˷����ַ������ݸ������������������ղ���ʾ��
		  
		  
		  �����̣���ֻ���Ⱥ�ģ�
		 2.���Ǹ����������Ρ����ʵ��Ⱥ�ģ����ʵ��˽���أ�
		   
                ע��㣺1.Ⱥ����������Ҫ�������е�accept_fd�����Դ浽������
            
			 ����Ͳ���ֱ�ӷ��ַ����ˡ�����Ҫ���ṹ��
			 
			 struct  chat
			 {
			     int chat_type; //Ⱥ�� ����˽��
				 int user_id;
				 int private_id;//���˭˽�ġ�
				 char user_name[30];
				 char chat_info[300];
			 }
			 //�ն��ַ���������

*/

//������Ϣ��,�����ӽ���д�빲���ڴ�
typedef struct  chat
{
	int chat_type; //Ⱥ��0 ����˽��1
	//int private_id;//���˭˽�ġ�	
	int user_id;
	char user_name[30];
	char chat_msg[300];
}CHAT_PACKET;
/*
typedef struct  chat
{
	int chat_type; //Ⱥ��0 ����˽��1
	int user_id;
	int private_id;//���˭˽�ġ�
	char user_name[30];
	char chat_msg[300];
}CHAT;
*/

vector<int> accfdVector;//ȫ�֣��洢�������׽���������
void sigaction_init();//�źų�ʼ����
void shm_init();//�����ڴ��ʼ��
void* shm_addr=NULL;//ȫ�֣������ڴ��ַ


void msg_handle(int num, siginfo_t *info, void *d)
{
	cout << "msg_handle" << info->si_int << endl;
	
	int r_size = 0;
	//shm_init();//���ն˹����ڴ��ʼ��,ע�⣬ֻ���ڲ�ͬ���̲���Ҫ���Σ�ͬһ�������ڳ�ʼ��һ�ξͺ�
	CHAT_PACKET recv_client;
	memcpy(&recv_client, shm_addr, sizeof(CHAT_PACKET));//�ӹ����ڴ������Ϣ
	
	cout << "shm recv: " << recv_client.chat_msg << endl;
	vector<int>::iterator it;//������
	for (it = accfdVector.begin(); it != accfdVector.end(); it++)//�������͸����пͻ���
	{
		if (*it != info->si_int)//�����Լ������Լ�������Ϣ
		{
			r_size = write(*it, &recv_client, sizeof(CHAT_PACKET));//ע�⣬accept_fd������ͻ�д
			if (r_size < 0)
			{
				cout << "server write err" << *it << endl;
			}
		}

	}
	//shmdt(shm_addr);//�ѹ�����,һֱ�ڶ������ѹ���
	
	
}


int main(int argc, char *argv[])
{
	pid_t father_pid = getpid();
	//shm_addr = (CHAT_PACKET*)malloc(5000);
	//==========================================socket
	int server_socket_fd;
	server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket_fd < 0)
	{
		perror("create socket err:");
		return -1;
	}
	
	//ʹ�ò��صȴ�TIME_WAIT״̬�Ϳ�������������
	int mw_optval = 1;
	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&mw_optval, sizeof(mw_optval));

	//==========================================bind
	//socket��socketaddr_in�Ǽ��ݵģ�socketaddr_in����Ա�Ѻã�����ͷ�ļ�#include <netinet/in.h>
	struct sockaddr_in ser_addr;

	int addr_len = sizeof(ser_addr);
	ser_addr.sin_family = AF_INET;//��ַ��Э�������ipv4Э����
	
	//ʹ���ֽ�ת������htons�����ֽ�˳��ת��Ϊ�����ֽ�˳��
	ser_addr.sin_port = htons(8888);//ע�⣬linux����ϵͳԤ��1-1024�Ķ˿ںţ��������ǲ�Ҫʹ����һ����
	ser_addr.sin_addr.s_addr =  htonl(INADDR_ANY);//INADDR_ANY�Զ���ip��ַ��192.168.xxx��
	if (bind(server_socket_fd, (struct sockaddr*)&ser_addr, addr_len) < 0)//��sockaddr_in����ת��Ϊsockaddr����
	{
		perror("bind err:");
		return -1;
	}
	//=================================listen
	if (listen(server_socket_fd, 10) < 0)//����2��ʾͬһʱ���ܷ�֧�ּ����ͻ���ͬʱ����
	{
		perror("listen err:");
		return -1;
	}
	
	
	
	//================================accept
	int accept_fd;
	
	//��װ�ź����ڷ����ź�
	sigaction_init();


	shm_init();//���Ͷ˹����ڴ��ʼ��
	
	while (1)
	{
		//����һ���ͻ������ӵ���������ʱ��accept�᷵��һ���µ��׽���������
		accept_fd = accept(server_socket_fd, NULL, NULL);
		 if (accept_fd == -1&&errno == EINTR)
		{
			continue;
		}	
		//cout << "new accept_fd is" << accept_fd<<endl;
		accfdVector.push_back(accept_fd);
	//==============================read
		int r_size = 0;
		cout << "server start to read..." << endl;
		pid_t pid;
		pid = fork();
		if (pid == 0)
		{
			while (1)
			{
				CHAT_PACKET client1;
				//�ӿͻ��˶�
				cout << "child ready to read" << endl;
				r_size = read(accept_fd, &client1, sizeof(client1));//��������ȴ��ӿͻ������յ�����,read�ַ���������\n
				if (r_size < 0)
				{
					cout << "read err" << endl;
				}
						
				//�ӽ��̰���Ϣ��д�빲���ڴ�
				memset(shm_addr, 0, sizeof(shm_addr));//�ѹ����ڴ�����
				memcpy(shm_addr, &client1, sizeof(CHAT_PACKET));//��ַ�����ݣ���С
				//Ȼ������Ϣaccept_fd֪ͨ�����̶���Ⱥ��

				union sigval value;//�½�Я������������value
				value.sival_int = accept_fd;
				cout << "send signal" << endl;
				sigqueue(father_pid, SIGUSR1, value);//Я��valueֵ�����ź�SIGUSR1�����̺�Ϊpid�Ľ���
				
				memset(&client1, 0, sizeof(client1));
				
				//exit(0);//just for test
			}
		}
		//close(accept_fd);//��رպͿͻ��˵�����

	}
	close(server_socket_fd);


	
	return 0;
}

void sigaction_init()
{
	//��װ�ź����ڷ����ź�
	struct sigaction sig_act;//�½��źŰ�װ�ṹ��
	sig_act.sa_sigaction = msg_handle;//ָ���źŹ�������
	sig_act.sa_flags = SA_SIGINFO;//�����ź���Я�����ݵ�
	//���źźͰ�װ�źŽṹ�����
	sigaction(SIGUSR1, &sig_act, NULL);//�ź�num�����źŽṹ��ָ�룬���źŽṹ��ָ��
}

void shm_init()//�����ڴ��ʼ�������������ڴ棬�ѹ����ڴ����ӵ�����,������shm_addr
{
	//���������ڴ�
	int shm_id;
	shm_id = shmget((key_t)6677, 4096, IPC_CREAT | 0666);//666�ɶ���д
	if (shm_id == -1)
	{
		perror("shm create error");
		return ;
	}

	//�ѹ����ڴ����ӵ�����
	shm_addr = shmat(shm_id, NULL, 0);
	//Ҫ���ڴ����һ��
	memset(shm_addr, 0, 4096);

}