#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mysql.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <map>
#include <queue>
#include <string>
using namespace std;

#define SOCKET_MAXBUF 1024

////////////////////////////////////////////////////////////////////////////
//������Ϣ��ʽ�����ڴ������л��������
struct ConnProto
{
	// ����ͨ��Э��Ķ���
	int type; // �ͻ����Է�����0 Ϊ��¼��1 Ϊע�᣻2 Ϊ��ָ�����û����죻3 ΪȺ�ģ�4 Ϊ�˳�
			  // �������Կͻ���0 Ϊ��¼�ɹ�����ʱsrcuserid���浱ǰ�����û���Ŀ��-1Ϊ����ʧ�� 2 Ϊ���û���Ϣ����3 ��Ⱥ�û���Ϣ����4 ���û��˳�
	int srcuserid; // Դ�û�ID��
};

// ���ڴ�����������
struct ChatContent
{
	int  destuserid; // Ŀ���û������ѡ�����˽��
	char strContent[200]; // ��������ݣ����Ϊ200���ַ�	
};

// ���ڴ���������Ϣ
struct UserContent
{
	char strName[20];
	char password[10];
};

// �����ڲ����û���Ϣ��Ĵ洢
struct UserNode
{
	// �����û���Ϣ���
	int  id;
	char strName[20]; // ����
	char password[10]; // ���ڵ�����
	char strIP[16];   // ����IP��ַ
	int  port;  // �˿ں�
	char startdate[30];
	char enddate[30];
};

// �����ڲ����Ŷ���Ϣ����
struct PacketNode
{
	char infor[1024];
	char date[30];
	char strIP[16];
	int  port;
};

////////////////////////////////////////////////////////////////////////////
// ���������������ڴ洢��������Ϣ����
queue<PacketNode> g_qPacketNodes;  // ���ڱ�ʾ�������Ϣ����
queue<string>	g_pWriteNodes; // ������MysQLд��Ķ�����Ϣ

map<int,UserNode> g_pOnLineUserMap; // ��MAP���ұ����ʽ���浱ǰ�����û�
map<int,UserNode> g_pDisLineUserMap; // ��MAP���ұ����ʽ���浱ǰ�������û�

MYSQL* m_pMyData;                  //msyql ���Ӿ��
int g_nServerSocket;               // UDP�׽���
int g_ServerPort = 9000;                // �˿�
int g_bIsIMServer = 0;             // �Ƿ����ɼ�
int g_lWriteNum = 0;
int g_lWriteFailNum = 0;

char g_dbName[30];                 // ���ݱ���
char g_tableName[30];
char g_strIP[15];
char g_strUser[50];
char g_strPass[20];
int  g_nPort = 3306;


////////////////////////////////////////////////////////////
// ��������ͷ��Ϣ
void Help();

// IM Server�Ŀ�����ر�
void RunIMServer();
void StopIMServer();

// ״̬��ѯ
void ShowStatus();

void InitDB();
void DBInfor();


// �����߳�
void* WriteThread(void *threapara);
void* IMServerThread(void *threapara);
void* IMServerApplicationThread(void *threapara);
int GetCurTime(char *strTime,int type);

void InitUserList();
void SendInfor(int srcid,char *strinfor,int destid,int type,int count);
void LoginFunc(char *pData,PacketNode &node,int srcid);
void RegisterFunc(char *pData,PacketNode &node,int srcid);
void PrivateChatFunc(char *pData,PacketNode &node,int srcid);
void PublicChatFunc(char *pData,PacketNode &node,int srcid);
void QuitFunc(char *pData,PacketNode &node,int srcid);
void OtherFunc(char *pData,PacketNode &node,int srcid);
////////////////////////////////////////////////////////////

int main()
{
	printf("��ʱ���������\n");
	char strTime[100];
	GetCurTime(strTime,0);
	printf("%s\n",strTime);
	InitDB();
	while(1)
	 {	 
		 printf("����������:>");
		 char command[255];
	     gets(command);
	
		 if(strcmp(command,"run im server")==0)
		 {
			 RunIMServer();
		 }
		 else if(strcmp(command,"stop im server")==0)
		 {
			 StopIMServer();
		 }
		 else if(strcmp(command,"status")==0)
		 {
			 ShowStatus();
		 }
		 else if(strcmp(command,"exit")==0)
		 {
			 exit(0);
		 }
		 else if(strcmp(command,"help") == 0)
		 {
			 Help();
		 }
		 else
		 {
			 printf("�Բ��𣬸����ʶ��!!!\n");
		 }
	 }
	return 0;
}
void InitDB()
{
	strcpy(g_dbName,"chat");
	strcpy(g_strIP,"10.21.0.96");
	strcpy(g_strUser,"root");
	strcpy(g_strPass,"523667");
	strcpy(g_tableName,"");	

	// �������ݿ����ӣ��������ʧ�ܣ���ѡ��ϵͳ�˳�
	m_pMyData = mysql_init(NULL);
	printf("���ڽ������ݿ�����................");
	if(mysql_real_connect(m_pMyData,g_strIP,g_strUser,g_strPass,g_dbName,g_nPort,NULL,0))
	{
		printf("[OK]\n���ڲ���ָ�������ݿ�................");
		if ( mysql_select_db( m_pMyData, g_dbName) < 0 ) //ѡ���ƶ������ݿ�ʧ��
		{
			mysql_close( m_pMyData ) ;//��ʼ��mysql�ṹʧ��	
			printf("[FAILED]\n���ݿ����ʧ�ܣ�Ӧ�ó����˳�........\n");
			DBInfor();
			exit(0);
					
		}
		else
		{
			printf("[OK]\n�����������ݿ�...\n");
		}
	}
	else 
    {  
		mysql_close( m_pMyData ) ;//��ʼ��mysql�ṹʧ��	
		printf("[FAILED]\n���ݿ�����ʧ�ܣ�Ӧ�ó����˳�........\n");
		DBInfor();
		exit(0);
    }
	// ����
	InitUserList();
}
void DBInfor()
{
	printf("---------���ݿ�������Ϣ----------------\n");
	printf("%-12s %-12s\n","�û���",g_strUser);
	printf("%-12s %-12s\n","����",g_strPass);
	printf("%-12s %-12s\n","IP",g_strIP);
	printf("%-12s %-12d\n","Port",g_nPort);
	printf("%-12s %-12s\n","���ݿ���",g_dbName);
	printf("%-12s %-12s\n","����",g_tableName);
}
void Help()
{
	printf("==================================================================\n");
	printf("IMServer version��V1.0\n");
	printf("author��my2005lb\n\n\n");
	printf("%-12s %-12s\n","run im server","�����������");
	printf("%-12s %-12s\n","stop im server","�ر��������");
	printf("%-12s %-12s\n","status","״̬��ѯ");
	printf("%-12s %-12s\n","exit","�˳�");
	printf("==================================================================\n");
}
// �ɼ��Ŀ�����ر�
void RunIMServer()
{
	if(g_bIsIMServer)
	{
		printf("��ǰ����������Ѵ��ڿ���״̬....\n");
	}
	else
	{
		// ���������̡߳��ֱ������ݽ����̡߳���������̡߳����ݿ�д���̡߳�
		g_bIsIMServer = 1;
		pthread_t threadid;
		pthread_create(&threadid, NULL, IMServerThread, NULL);     
		pthread_t threadid1;
		pthread_create(&threadid1, NULL, IMServerApplicationThread, NULL);  
		pthread_t threadid2;
		pthread_create(&threadid2, NULL, WriteThread, NULL);  
		printf("��������ѿ���....\n");
	}
}
void StopIMServer()
{
	if(g_bIsIMServer)
	{
		g_bIsIMServer = 0;		
		printf("��������ѹر�....\n");
	}
	else
	{
		printf("��ǰ��������Ѵ��ڹر�״̬....\n");
	}
}
// ״̬��ѯ
void ShowStatus()
{
	printf("==========��ǰ��״̬��ѯ==============\n");
	printf("ϵͳ��ǰ����:[%s]\n",g_bIsIMServer>0?"����":"�ر�");
	printf("\n==========�û���Ϣ״̬==============\n");
	printf("��ǰ�����û�����:[%d]\n",g_pOnLineUserMap.size());
	printf("��ǰ�����û�����:[%d]\n",g_pDisLineUserMap.size());
	printf("��ǰ������еĸ���:[%d]\n",g_qPacketNodes.size());
	printf("\n==========���ݴ洢״̬��ѯ==============\n");
	printf("��ǰ��д��������������:[%d]\n",g_pWriteNodes.size());
	printf("�ɹ�д��ĸ���:[%d]\n",g_lWriteNum);
	printf("д��ʧ�ܵĸ���:[%d]\n",g_lWriteFailNum);
}
void* IMServerThread(void *threapara)
{
	// ���ڳ�ʼ��UDP�׽���
	int recvlen;
	struct sockaddr_in ser_addr;
	struct sockaddr_in remote_addr;
	if ((g_nServerSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Error: Create Socket Error!!!");
		return NULL;
	}
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(g_ServerPort);
	ser_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(g_nServerSocket, (struct sockaddr*) &ser_addr,
			sizeof(struct sockaddr)) == -1) {
		return NULL;
	}
	socklen_t len = sizeof(remote_addr);
	// �ж��Ƿ��ڷ���״̬�������ڷ���״̬�������������
	while (g_bIsIMServer) {
		//cout << "config Server State is runing...." << endl;
		PacketNode node;
		recvlen = recvfrom(g_nServerSocket, node.infor, SOCKET_MAXBUF, 0,
				(sockaddr*) &remote_addr, &len);
		if (recvlen > 0) {
			
			strcpy(node.strIP, inet_ntoa(remote_addr.sin_addr));
			node.port = ntohs(remote_addr.sin_port);
			GetCurTime(node.date,1);

			printf("��Ϣ�����ճ���:[%d] ʵ�ʳ���:[%d] IP��ַ:[%s] �˿ں�:[%d] [%d] [%d]\n",recvlen,strlen(node.infor),node.strIP,node.port,htons(remote_addr.sin_port),remote_addr.sin_port);
			g_qPacketNodes.push(node);
		}	
	}
	return NULL;
}
void* IMServerApplicationThread(void *threapara)
{
	while (g_bIsIMServer) {
		if(g_qPacketNodes.size() <= 0)
		{
			// �����ǰ�Ķ��г���Ϊ0����˯��
			sleep(1);
			continue;
		}
		PacketNode node = g_qPacketNodes.front();
		g_qPacketNodes.pop();

		char *pData = node.infor;
		ConnProto *pHeader = (ConnProto*)pData;
		pData += sizeof(ConnProto);
		
		// 0 Ϊ��¼��1 Ϊע�᣻2 Ϊ��ָ�����û����죻3 ΪȺ�ģ�4 Ϊ�˳�
		if(pHeader->type == 0)
		{
			LoginFunc(pData,node,pHeader->srcuserid);
		}
		else if(pHeader->type == 1)
		{
			RegisterFunc(pData,node,pHeader->srcuserid);
		}
		else if(pHeader->type == 2)
		{
			PrivateChatFunc(pData,node,pHeader->srcuserid);
		}
		else if(pHeader->type == 3)
		{
			PublicChatFunc(pData,node,pHeader->srcuserid);
		}
		else if(pHeader->type == 4)
		{
			QuitFunc(pData,node,pHeader->srcuserid);
		}
		else
		{
			// ��ʶ���Э������
			OtherFunc(pData,node,pHeader->srcuserid);
		}
	}
	return NULL;
}
void* WriteThread(void *threapara)
{		
	while(g_bIsIMServer)
	{
		if(g_pWriteNodes.size() > 0)
		{
			string strSQL = g_pWriteNodes.front();
			g_pWriteNodes.pop();			
			// ��ö�ͷ��㣬����SQL��䣬д�����ݿ���	
			if(!mysql_real_query( m_pMyData, strSQL.c_str(),strlen(strSQL.c_str())))
				g_lWriteNum++;
			else
			{
				g_lWriteFailNum++;
			}				
		}
	}
	mysql_close( m_pMyData );
	pthread_exit(NULL);
	return NULL;
}

int GetCurTime(char *strTime,int type)
{
    time_t t;
	struct tm *tm = NULL;
    t = time(NULL);
    if(t == -1)
	{
        return -1;
    }
    tm = localtime(&t);
    if(tm == NULL)
	{
        return -1;
    }
	if(type == 0)
		sprintf(strTime,"ϵͳ����ʱ��Ϊ: %d-%d-%d %d:%d:%d\n",tm->tm_year + 1900,tm->tm_mon + 1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	else if(type == 1)
		sprintf(strTime,"%d-%d-%d %d:%d:%d",tm->tm_year + 1900,tm->tm_mon + 1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	else if(type == 2)
		sprintf(strTime,"%d_%d_%d",tm->tm_year + 1900,tm->tm_mon + 1,tm->tm_mday);

    return 0;
}
// ���ڳ�ʼ���û���Ϣ�б�
void InitUserList()
{
	MYSQL_RES * res ;//�����ѯ���
	MYSQL_FIELD * fd ;//�����ֶν��
	MYSQL_ROW row ;
	int num_row = 0;//�õ���¼����
	int num_col = 0;//�õ��ֶ�����
	const char *strSQL="select id,name,password from user_infor";
	if(!mysql_real_query( m_pMyData, strSQL,strlen(strSQL)))
	{
	//	printf("ִ�гɹ���Ӱ��%d��\n",mysql_affected_rows(m_pMyData));//�õ���Ӱ�������
		res  = mysql_store_result( m_pMyData ) ;//�����ѯ���
		num_row = (int) mysql_num_rows( res ) ; //�õ���¼����
		num_col = mysql_num_fields( res ) ;//�õ��ֶ�����
		
		for (int f1 = 0; f1 < num_row; f1++) 
		{
			row = mysql_fetch_row(res); 
			UserNode node;
			node.id = atoi(row[0]);
			strcpy(node.strName,row[1]);
			strcpy(node.password,row[2]);
			g_pDisLineUserMap[node.id] = node;
		}
		mysql_free_result(res);
	}
	else      
	{
		printf("�����û���Ϣʧ��");
	}
}
// ������Ϣ�ķ���
void SendInfor(int srcid,char *strinfor,int destid,int type,int count)
{		
	if(type == 0)
	{
		// Ⱥ��	
		map<int,UserNode>::iterator it; 
        for(it=g_pOnLineUserMap.begin(); it != g_pOnLineUserMap.end();it++)   
        {   
			if(it->first != srcid)
			{
				struct sockaddr_in destAddr;
				bzero(&destAddr,sizeof(destAddr));
				destAddr.sin_family =AF_INET;
				destAddr.sin_port = htons(it->second.port);
				destAddr.sin_addr.s_addr = inet_addr(it->second.strIP);
				sendto(g_nServerSocket, strinfor, count, 0, (struct sockaddr *)&destAddr,sizeof(destAddr));
			}
        }   
	}
	else
	{
		// ˽��
		map<int,UserNode>::iterator it = g_pOnLineUserMap.find(destid);
		if(it != g_pOnLineUserMap.end())
		{
			struct sockaddr_in destAddr;
			bzero(&destAddr,sizeof(destAddr));
			destAddr.sin_family =AF_INET;
			destAddr.sin_port = htons(it->second.port);
			destAddr.sin_addr.s_addr = inet_addr(it->second.strIP);
			sendto(g_nServerSocket, strinfor, count, 0, (struct sockaddr *)&destAddr,sizeof(destAddr));
		}
	}
}

void LoginFunc(char *pData,PacketNode &node,int srcid)
{
	// ����û���¼������û���¼�ɹ��򷵻���Ӧ�룬�Լ���ǰ�������û��б�
	ConnProto retnode;
	map<int,UserNode>::iterator p1 = g_pOnLineUserMap.find(srcid);
	map<int,UserNode>::iterator p2 = g_pDisLineUserMap.find(srcid);
	if(p1 != g_pOnLineUserMap.end()){
		// ��ǰ�����ѵ�¼
		retnode.type = -1;
	}
	else if(p2 == g_pDisLineUserMap.end()){
		// �����ڸ��û�
		retnode.type = -2;
	}
	else{
		UserContent *pUser = (UserContent *)pData;
		if(strcmp(pUser->strName,p2->second.strName) == 0 
			&& strcmp(pUser->strName,p2->second.strName) == 0 )	{
			// ����ɹ� ���������û�Ǩ���������û��б���
			retnode.type = 0;
			strcpy(p2->second.strIP,node.strIP);
			p2->second.port = node.port;
			strcpy(p2->second.startdate,node.date);
			g_pOnLineUserMap[srcid] = p2->second;
			g_pDisLineUserMap.erase(srcid);		
	
	    	// ������õ�ǰ��ÿһ���ڵ㣬��ʽ���û����߱����͵���¼�ͻ���
			char strRet[2048];
			int ncount = 0;
			char *pstrRet = strRet;
			ConnProto *pretnode = (ConnProto *)pstrRet;
			retnode.srcuserid = g_pOnLineUserMap.size()-1;
			*pretnode = retnode;
			pstrRet += sizeof(ConnProto);
			ncount += sizeof(ConnProto);
			map<int,UserNode>::iterator it; 
			for(it=g_pOnLineUserMap.begin(); it != g_pOnLineUserMap.end();it++) {
				if(it->first != srcid){
					UserNode cont = it->second;
					UserNode *pcont = (UserNode *)pstrRet;
					*pcont = cont;
					pstrRet += sizeof(UserNode);
					ncount += sizeof(UserNode);
				}
			}
			SendInfor(srcid,strRet,srcid,1,ncount);
			// ��Ⱥ�ڹ㲥���û�����֪ͨ
			char strnotice[100];
			// ���ڸ�ʽ�������͵ĵ�¼��Ϣ
			ConnProto notice;
			notice.srcuserid = srcid;
			notice.type = 10;
			UserNode cont;
			strcpy(cont.strName,pUser->strName);
			cont.id = srcid;
			strcpy(cont.strIP,node.strIP);
			cont.port = node.port;
			strcpy(cont.startdate,node.date);
			char *pStr = strnotice;
			ConnProto *pnotice =(ConnProto *)pStr;
			*pnotice = notice;
			pStr+= sizeof(ConnProto);
			UserNode *pcont = (UserNode *)pStr;
			*pcont = cont;
			SendInfor(srcid,strnotice,0,0,sizeof(ConnProto)+sizeof(UserNode));
			// д����־��¼
			char log[1024];
			sprintf(log,"insert into user_log(srcid,type,strip,port,protime) values(%d,0,'%s',%d,'%s')",
				srcid,node.strIP,node.port,node.date);
			g_pWriteNodes.push(log);
		    return ;
		}	
		else
			retnode.type = -3;
	}
	// ���¼�ķ����ߣ�������Ӧ�룬����ʾ�����ԭ�� 
	char error[100];
	char *perror = error;
	ConnProto *pretnode =(ConnProto *)perror;
	*pretnode = retnode;
	// ���ʧ�ܣ��򷵻�ʧ�ܵĽ��
	SendInfor(srcid,error,srcid,1,sizeof(ConnProto));
	char log[1024];
	sprintf(log,"insert into user_log(srcid,type,strip,port,protime) values(%d,%d,'%s',%d,'%s')",
				srcid,retnode.type,node.strIP,node.port,node.date);
	g_pWriteNodes.push(log);

}
void RegisterFunc(char *pData,PacketNode &node,int srcid)
{
	ConnProto retnode;
	map<int,UserNode>::iterator p1 = g_pOnLineUserMap.find(srcid);
	map<int,UserNode>::iterator p2 = g_pDisLineUserMap.find(srcid);
	if(p1 != g_pOnLineUserMap.end() 
		|| p2 != g_pDisLineUserMap.end())
	{
		// ���ڸ��û�
		retnode.type = -11;
	}
	else
	{
		retnode.type = 11;
		UserContent *pcont = (UserContent *)pData;
		UserNode pUser;
		pUser.id = srcid;
		strcpy(pUser.password,pcont->password);
		strcpy(pUser.strName,pcont->strName);
		strcpy(pUser.strIP,node.strIP);
		pUser.port = node.port;
		strcpy(pUser.startdate,node.date);

		// ��������û��б���д��ü�¼
		g_pDisLineUserMap[srcid] = pUser;

		// ��ʽ����¼��д���û���Ϣ����
		char userinfor[1024];
		sprintf(userinfor,"insert into user_infor(id,name,password,regtime) values(%d,'%s','%s','%s')",
				srcid,pUser.strName,pUser.password,pUser.startdate);
		g_pWriteNodes.push(userinfor);	
	}
	char infor[100];
	char *pinfor = infor;
	ConnProto *pretnode =(ConnProto *)pinfor;
	*pretnode = retnode;
	SendInfor(srcid,infor,srcid,1,sizeof(ConnProto));
	// ��ʽ����¼��д���û���־
	char log[1024];
	sprintf(log,"insert into user_log(srcid,type,strip,port,protime) values(%d,%d,'%s',%d,'%s')",
			srcid,retnode.type,node.strIP,node.port,node.date);
	g_pWriteNodes.push(log);
}
void PrivateChatFunc(char *pData,PacketNode &node,int srcid)
{
	// ���з������ݵĸ�ʽ��
	ChatContent *pCont = (ChatContent *)pData;
	ConnProto *pCP = (ConnProto *)node.infor;
	pCP->srcuserid = pCont->destuserid;
	pCont->destuserid = srcid;
	SendInfor(srcid,node.infor,pCP->srcuserid,1,sizeof(ConnProto)+sizeof(ChatContent));
	// ���������¼�Ĵ洢
	char log[2000];
	sprintf(log,"insert into user_chat(srcid,destid,type,strip,port,centime,content) values(%d,%d,%d,'%s',%d,'%s','%s')",
			srcid,pCP->srcuserid,pCP->type,node.strIP,node.port,node.date,pCont->strContent);
	g_pWriteNodes.push(log);
}
void PublicChatFunc(char *pData,PacketNode &node,int srcid)
{
	// ������˽��
	ChatContent *pCont = (ChatContent *)pData;
	ConnProto *pCP = (ConnProto *)node.infor;
	pCP->srcuserid = pCont->destuserid;
	pCont->destuserid = srcid;
	SendInfor(srcid,node.infor,pCP->srcuserid,0,sizeof(ConnProto)+sizeof(ChatContent));
	char log[2000];
	sprintf(log,"insert into user_chat(srcid,destid,type,strip,port,centime,content) values(%d,%d,%d,'%s',%d,'%s','%s')",
			srcid,0,pCP->type,node.strIP,node.port,node.date,pCont->strContent);
	g_pWriteNodes.push(log);
}
void QuitFunc(char *pData,PacketNode &node,int srcid)
{	
	map<int,UserNode>::iterator p1 = g_pOnLineUserMap.find(srcid);
	if(p1 == g_pOnLineUserMap.end())
	{	
		return ;
	}
	// ����ǵ�ǰ�����û�
	// �û�Ǩ�� ������־��д�� �û�Ⱥ���㲥
	// ����ɹ� �������û�Ǩ�����������û��б���
	g_pDisLineUserMap[srcid] = p1->second;
	g_pOnLineUserMap.erase(srcid);		
		
	// ��Ⱥ�ڹ㲥���û�����֪ͨ
	char strnotice[100];
	// ���ڸ�ʽ�������͵ĵ�¼��Ϣ
	ConnProto notice;
	notice.srcuserid = srcid;
	notice.type = 4;
	char *pStr = strnotice;
	ConnProto *pnotice =(ConnProto *)pStr;
	*pnotice = notice;
	SendInfor(srcid,strnotice,0,0,sizeof(ConnProto));

	// д����־��¼
	char log[1024];
	char strdate[30];
	GetCurTime(strdate,1);
	sprintf(log,"insert into user_log(srcid,type,strip,port,protime,proendtime) values(%d,%d,'%s',%d,'%s','%s')",
		srcid,4,node.strIP,node.port,node.date,strdate);
	g_pWriteNodes.push(log);

}
void OtherFunc(char *pData,PacketNode &node,int srcid)
{
	char log[1024];
	sprintf(log,"insert into user_log(srcid,type,strip,port,protime) values(%d,%d,'%s',%d,'%s')",
			srcid,5,node.strIP,node.port,node.date);
	g_pWriteNodes.push(log);
}