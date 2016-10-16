#ifndef MAINDLG_H
#define MAINDLG_H

#include <QtGui/QDialog>
#include <QtNetwork>
#include <map>
using namespace std;
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
namespace Ui {
    class MainDlg;
}

class MainDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(MainDlg)
public:
    explicit MainDlg(QWidget *parent = 0);
    virtual ~MainDlg();
    void SendInfor(char *strinfor,int count);// 用于发送数据包
    void InitUDPSocket();


protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::MainDlg *m_ui;
    QUdpSocket *udpSocket;
    map<int,UserNode> m_mapuserlist;
    QString strName;
    QString strPassword;
    QString strIP;
    int nID;
    int m_nCurrentID;
private slots:
    void on_m_UserList_currentRowChanged(int currentRow);
    void on_m_ButtonExit_clicked();
    void on_Button_Exit_clicked();
    void on_Button_Reg_clicked();
    void on_Button_Login_clicked();
    void on_m_ButtonSend_clicked();
    void receiveMessage();
};

#endif // MAINDLG_H
