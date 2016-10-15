#ifndef MAINDLG_H
#define MAINDLG_H

#include <QtGui/QDialog>
#include <QtNetwork>
#include <map>
using namespace std;
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
namespace Ui {
    class MainDlg;
}

class MainDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(MainDlg)
public:
    explicit MainDlg(QWidget *parent = 0);
    virtual ~MainDlg();
    void SendInfor(char *strinfor,int count);// ���ڷ������ݰ�
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
