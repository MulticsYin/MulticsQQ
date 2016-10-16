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
//基本信息格式，用于存入排列缓存队列中
struct ConnProto
{
	// 进行通信协议的定义
	int type; // 客户机对服务器0 为登录；1 为注册；2 为向指定的用户聊天；3 为群聊；4 为退出
			  // 服务器对客户机0 为登录成功，此时srcuserid保存当前在线用户数目；-1为操作失败 2 为有用户信息到；3 有群用户信息到；4 有用户退出
	int srcuserid; // 源用户ID号
};

// 用于传送聊天内容
struct ChatContent
{
	int  destuserid; // 目标用户，如果选择的是私聊
	char strContent[200]; // 聊天的内容，最多为200个字符	
};

// 用于传送内容信息
struct UserContent
{
	char strName[20];
	char password[10];
};

// 用于内部的用户信息表的存储
struct UserNode
{
	// 在线用户信息结点
	int  id;
	char strName[20]; // 姓名
	char password[10]; // 所在的密码
	char strIP[16];   // 所在IP地址
	int  port;  // 端口号
	char startdate[30];
	char enddate[30];
};

// 用于内部的排队信息队列
struct PacketNode
{
	char infor[1024];
	char date[30];
	char strIP[16];
	int  port;
};

////////////////////////////////////////////////////////////////////////////
// 公共数据区　用于存储公共的信息数据
queue<PacketNode> g_qPacketNodes;  // 用于表示请求的信息队列
queue<string>	g_pWriteNodes; // 用于向MysQL写入的队列信息

map<int,UserNode> g_pOnLineUserMap; // 用MAP查找表的形式保存当前在线用户
map<int,UserNode> g_pDisLineUserMap; // 用MAP查找表的形式保存当前不在线用户

MYSQL* m_pMyData;                  //msyql 连接句柄
int g_nServerSocket;               // UDP套接字
int g_ServerPort = 9000;                // 端口
int g_bIsIMServer = 0;             // 是否开启采集
int g_lWriteNum = 0;
int g_lWriteFailNum = 0;

char g_dbName[30];                 // 数据表名
char g_tableName[30];
char g_strIP[15];
char g_strUser[50];
char g_strPass[20];
int  g_nPort = 3306;


////////////////////////////////////////////////////////////
// 公开函数头信息
void Help();

// IM Server的开启与关闭
void RunIMServer();
void StopIMServer();

// 状态查询
void ShowStatus();

void InitDB();
void DBInfor();


// 三个线程
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
	printf("即时聊天服务器\n");
	char strTime[100];
	GetCurTime(strTime,0);
	printf("%s\n",strTime);
	InitDB();
	while(1)
	 {	 
		 printf("请输入命令:>");
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
			 printf("对不起，该命令不识别!!!\n");
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

	// 进行数据库连接，如果连接失败，则选择系统退出
	m_pMyData = mysql_init(NULL);
	printf("正在进行数据库连接................");
	if(mysql_real_connect(m_pMyData,g_strIP,g_strUser,g_strPass,g_dbName,g_nPort,NULL,0))
	{
		printf("[OK]\n正在查找指定的数据库................");
		if ( mysql_select_db( m_pMyData, g_dbName) < 0 ) //选择制定的数据库失败
		{
			mysql_close( m_pMyData ) ;//初始化mysql结构失败	
			printf("[FAILED]\n数据库查找失败，应用程序退出........\n");
			DBInfor();
			exit(0);
					
		}
		else
		{
			printf("[OK]\n已连到上数据库...\n");
		}
	}
	else 
    {  
		mysql_close( m_pMyData ) ;//初始化mysql结构失败	
		printf("[FAILED]\n数据库连接失败，应用程序退出........\n");
		DBInfor();
		exit(0);
    }
	// 用于
	InitUserList();
}
void DBInfor()
{
	printf("---------数据库配置信息----------------\n");
	printf("%-12s %-12s\n","用户名",g_strUser);
	printf("%-12s %-12s\n","密码",g_strPass);
	printf("%-12s %-12s\n","IP",g_strIP);
	printf("%-12s %-12d\n","Port",g_nPort);
	printf("%-12s %-12s\n","数据库名",g_dbName);
	printf("%-12s %-12s\n","表名",g_tableName);
}
void Help()
{
	printf("==================================================================\n");
	printf("IMServer version：V1.0\n");
	printf("author：my2005lb\n\n\n");
	printf("%-12s %-12s\n","run im server","开启聊天服务");
	printf("%-12s %-12s\n","stop im server","关闭聊天服务");
	printf("%-12s %-12s\n","status","状态查询");
	printf("%-12s %-12s\n","exit","退出");
	printf("==================================================================\n");
}
// 采集的开启与关闭
void RunIMServer()
{
	if(g_bIsIMServer)
	{
		printf("当前的聊天服务已处于开启状态....\n");
	}
	else
	{
		// 启动三个线程　分别是数据接收线程、聊天服务线程、数据库写入线程　
		g_bIsIMServer = 1;
		pthread_t threadid;
		pthread_create(&threadid, NULL, IMServerThread, NULL);     
		pthread_t threadid1;
		pthread_create(&threadid1, NULL, IMServerApplicationThread, NULL);  
		pthread_t threadid2;
		pthread_create(&threadid2, NULL, WriteThread, NULL);  
		printf("聊天服务已开启....\n");
	}
}
void StopIMServer()
{
	if(g_bIsIMServer)
	{
		g_bIsIMServer = 0;		
		printf("聊天服务已关闭....\n");
	}
	else
	{
		printf("当前聊天服务已处于关闭状态....\n");
	}
}
// 状态查询
void ShowStatus()
{
	printf("==========当前的状态查询==============\n");
	printf("系统当前处于:[%s]\n",g_bIsIMServer>0?"启动":"关闭");
	printf("\n==========用户信息状态==============\n");
	printf("当前在线用户个数:[%d]\n",g_pOnLineUserMap.size());
	printf("当前离线用户个数:[%d]\n",g_pDisLineUserMap.size());
	printf("当前请求队列的个数:[%d]\n",g_qPacketNodes.size());
	printf("\n==========数据存储状态查询==============\n");
	printf("当前待写入的数据请求个数:[%d]\n",g_pWriteNodes.size());
	printf("成功写入的个数:[%d]\n",g_lWriteNum);
	printf("写入失败的个数:[%d]\n",g_lWriteFailNum);
}
void* IMServerThread(void *threapara)
{
	// 用于初始化UDP套接字
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
	// 判断是否处于服务状态，若处于服务状态，则加入服务队列
	while (g_bIsIMServer) {
		//cout << "config Server State is runing...." << endl;
		PacketNode node;
		recvlen = recvfrom(g_nServerSocket, node.infor, SOCKET_MAXBUF, 0,
				(sockaddr*) &remote_addr, &len);
		if (recvlen > 0) {
			
			strcpy(node.strIP, inet_ntoa(remote_addr.sin_addr));
			node.port = ntohs(remote_addr.sin_port);
			GetCurTime(node.date,1);

			printf("信息：接收长度:[%d] 实际长度:[%d] IP地址:[%s] 端口号:[%d] [%d] [%d]\n",recvlen,strlen(node.infor),node.strIP,node.port,htons(remote_addr.sin_port),remote_addr.sin_port);
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
			// 如果当前的队列长度为0，则睡眠
			sleep(1);
			continue;
		}
		PacketNode node = g_qPacketNodes.front();
		g_qPacketNodes.pop();

		char *pData = node.infor;
		ConnProto *pHeader = (ConnProto*)pData;
		pData += sizeof(ConnProto);
		
		// 0 为登录；1 为注册；2 为向指定的用户聊天；3 为群聊；4 为退出
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
			// 不识别的协议类型
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
			// 获得队头结点，构造SQL语句，写入数据库中	
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
		sprintf(strTime,"系统运行时间为: %d-%d-%d %d:%d:%d\n",tm->tm_year + 1900,tm->tm_mon + 1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	else if(type == 1)
		sprintf(strTime,"%d-%d-%d %d:%d:%d",tm->tm_year + 1900,tm->tm_mon + 1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	else if(type == 2)
		sprintf(strTime,"%d_%d_%d",tm->tm_year + 1900,tm->tm_mon + 1,tm->tm_mday);

    return 0;
}
// 用于初始化用户信息列表
void InitUserList()
{
	MYSQL_RES * res ;//保存查询结果
	MYSQL_FIELD * fd ;//保存字段结果
	MYSQL_ROW row ;
	int num_row = 0;//得到记录数量
	int num_col = 0;//得到字段数量
	const char *strSQL="select id,name,password from user_infor";
	if(!mysql_real_query( m_pMyData, strSQL,strlen(strSQL)))
	{
	//	printf("执行成功，影响%d行\n",mysql_affected_rows(m_pMyData));//得到受影响的行数
		res  = mysql_store_result( m_pMyData ) ;//保存查询结果
		num_row = (int) mysql_num_rows( res ) ; //得到记录数量
		num_col = mysql_num_fields( res ) ;//得到字段数量
		
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
		printf("下载用户信息失败");
	}
}
// 用于信息的发送
void SendInfor(int srcid,char *strinfor,int destid,int type,int count)
{		
	if(type == 0)
	{
		// 群发	
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
		// 私聊
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
	// 针对用户登录，如果用户登录成功则返回响应码，以及当前的在线用户列表
	ConnProto retnode;
	map<int,UserNode>::iterator p1 = g_pOnLineUserMap.find(srcid);
	map<int,UserNode>::iterator p2 = g_pDisLineUserMap.find(srcid);
	if(p1 != g_pOnLineUserMap.end()){
		// 当前存在已登录
		retnode.type = -1;
	}
	else if(p2 == g_pDisLineUserMap.end()){
		// 不存在该用户
		retnode.type = -2;
	}
	else{
		UserContent *pUser = (UserContent *)pData;
		if(strcmp(pUser->strName,p2->second.strName) == 0 
			&& strcmp(pUser->strName,p2->second.strName) == 0 )	{
			// 如果成功 将非在线用户迁移至在线用户列表中
			retnode.type = 0;
			strcpy(p2->second.strIP,node.strIP);
			p2->second.port = node.port;
			strcpy(p2->second.startdate,node.date);
			g_pOnLineUserMap[srcid] = p2->second;
			g_pDisLineUserMap.erase(srcid);		
	
	    	// 遍历获得当前的每一个节点，格式化用户在线表，发送到登录客户端
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
			// 向群众广播该用户上线通知
			char strnotice[100];
			// 用于格式化待发送的登录信息
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
			// 写入日志记录
			char log[1024];
			sprintf(log,"insert into user_log(srcid,type,strip,port,protime) values(%d,0,'%s',%d,'%s')",
				srcid,node.strIP,node.port,node.date);
			g_pWriteNodes.push(log);
		    return ;
		}	
		else
			retnode.type = -3;
	}
	// 向登录的发起者，发送响应码，来提示出错的原因 
	char error[100];
	char *perror = error;
	ConnProto *pretnode =(ConnProto *)perror;
	*pretnode = retnode;
	// 如果失败，则返回失败的结果
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
		// 存在该用户
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

		// 向非在线用户列表中写入该记录
		g_pDisLineUserMap[srcid] = pUser;

		// 格式化记录，写入用户信息表中
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
	// 格式化记录，写入用户日志
	char log[1024];
	sprintf(log,"insert into user_log(srcid,type,strip,port,protime) values(%d,%d,'%s',%d,'%s')",
			srcid,retnode.type,node.strIP,node.port,node.date);
	g_pWriteNodes.push(log);
}
void PrivateChatFunc(char *pData,PacketNode &node,int srcid)
{
	// 进行发送数据的格式化
	ChatContent *pCont = (ChatContent *)pData;
	ConnProto *pCP = (ConnProto *)node.infor;
	pCP->srcuserid = pCont->destuserid;
	pCont->destuserid = srcid;
	SendInfor(srcid,node.infor,pCP->srcuserid,1,sizeof(ConnProto)+sizeof(ChatContent));
	// 进行聊天记录的存储
	char log[2000];
	sprintf(log,"insert into user_chat(srcid,destid,type,strip,port,centime,content) values(%d,%d,%d,'%s',%d,'%s','%s')",
			srcid,pCP->srcuserid,pCP->type,node.strIP,node.port,node.date,pCont->strContent);
	g_pWriteNodes.push(log);
}
void PublicChatFunc(char *pData,PacketNode &node,int srcid)
{
	// 类似于私聊
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
	// 如果是当前在线用户
	// 用户迁移 操作日志的写入 用户群发广播
	// 如果成功 将在线用户迁移至非在线用户列表中
	g_pDisLineUserMap[srcid] = p1->second;
	g_pOnLineUserMap.erase(srcid);		
		
	// 向群众广播该用户下线通知
	char strnotice[100];
	// 用于格式化待发送的登录信息
	ConnProto notice;
	notice.srcuserid = srcid;
	notice.type = 4;
	char *pStr = strnotice;
	ConnProto *pnotice =(ConnProto *)pStr;
	*pnotice = notice;
	SendInfor(srcid,strnotice,0,0,sizeof(ConnProto));

	// 写入日志记录
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
