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
#include <arpa/inet.h>
using namespace std;

typedef struct  chat
{
	int chat_type; //Ⱥ��0 ����˽��1
	//int private_id;//���˭˽�ġ�	
	int user_id;
	char user_name[30];
	char chat_msg[300];
}CHAT_PACKET;


int main(int argc, char *argv[])
{
	//==========================================socket
	int socket_fd;
	struct sockaddr_in ser_addr;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		perror("create socket err:");
		return -1;
	}
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(8888);
	ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//�ú�����ipת��Ϊ��������#include <arpa/inet.h>
	//����127.0.0.1��ʾ����
	//======================================connect
	int addr_len = sizeof(ser_addr);
	if (connect(socket_fd, (struct sockaddr*)&ser_addr, addr_len) < 0)
	{
		perror("connect err:");
		return -1;
	}
	

	int r_size = 0;
	//���ϻ�ȡ�Ӽ��̵�����
//	while (fgets(buffer, sizeof(buffer), stdin) != NULL)//��������ȴ��û�����
//	{
	
	CHAT_PACKET client = { 0, 1001, "name", "0" };
	char info[50];
	


	cout << "welcome!please input your name:" << endl;
	//	scanf(info);
	if (fgets(info, sizeof(info), stdin) != NULL)//�����Ŀͻ�����Ϣ����ֵ�û���
	{
		
	//�����Ŀ����fgets���յ��ַ���������ַ��������ַ�\0ǰ��,����и�'\n'��ô���滻Ϊ'\0'
	//ʵ�����û������жϣ��������ֻ�����з�ʹ�����ֺ���Ϣ����ͬһ�У����ÿ�~
		if (info[strlen(info) - 1] == '\n')
		{
			info[strlen(info) - 1] = 0;	
		}

		memset(client.user_name, 0, sizeof(client.user_name));
		strcpy(client.user_name, info);
		memset(info, 0, sizeof(info));
	}//û�������������
	cout << "now you can chat with your friends!" << endl;


	pid_t pid;
	while (1)
	{
		pid = fork();
		if (pid == 0)//����ӽ��̸��𲻶϶�ȡ����ʾ
		{
			while (1)
			{
				//�ͻ��˴ӷ���������,�Լ���������ҲҪ����׼������,���ӽ������治��read
				r_size = read(socket_fd, &client, sizeof(client));//��������ȴ���
				if (r_size > 0)
				{
					cout << client.user_name << " : " << client.chat_msg << endl;
				}
			}
		}
		else if (pid > 0)
		{
			//���Ͻ����û�����
			if (fgets(info, sizeof(info), stdin) != NULL)//�û�������ϢinfoȻ�����ṹ��
			{
				//�����Ŀ����fgets���յ��ַ���������ַ��������ַ�\0ǰ��,����и�'\n'��ô���滻Ϊ'\0'
				//ʵ�����û������жϣ������ַ��������з������ÿ�~
				if (info[strlen(info) - 1] == '\n')
				{
					info[strlen(info) - 1] = 0;	
				}
				memset(client.chat_msg, 0, sizeof(client.chat_msg));//����Ϣ���㣬�ٰ�����Ϣ������ṹ��
				strcpy(client.chat_msg, info);
			}
				//�ͻ��˷��͸����������ַ���strlen�ṹ��sizeof
			r_size = write(socket_fd, &client, sizeof(client));
			if (r_size < 0)
			{
				perror("send meg err");
			}
			//��Ҫ��name
			memset(client.chat_msg, 0, sizeof(client.chat_msg));//����Ϣ���㣬�ٰ�����Ϣ������ṹ��
			
		}


	}


		

	
	return 0;
}