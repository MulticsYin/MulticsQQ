#include "maindlg.h"
#include "ui_maindlg.h"
#include <QtGui>

MainDlg::MainDlg(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::MainDlg)
{
    m_ui->setupUi(this);
    udpSocket = NULL;
    m_nCurrentID = -1; // ��ʾȫ������Ϣ
    m_ui->m_UserList->insertItem(0,"Ⱥ��");
    m_ui->Text_Pass->setEchoMode(QLineEdit::Password);
}

MainDlg::~MainDlg()
{
    delete m_ui;
}

void MainDlg::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainDlg::on_m_ButtonSend_clicked()
{
    if(NULL == udpSocket)
        InitUDPSocket();

    // �йط��͵��¼�����
    QString str = m_ui->m_EditInput->toPlainText();
    if(str.length() <= 0 || str.length()> 200)
    {
        QMessageBox::information(this, "С��ʾ",tr("������ϢΪ�ջ��ߴ���200"));
        return ;
    }
    QString strTip = tr("[��]��[");
    //m_ui->m_UserList->currentIndex()
    ConnProto conn;
    ChatContent node;
    sprintf(node.strContent,"%s",str.toAscii().data());
    if(m_nCurrentID != -1)
    {
       // ˽��
       conn.type=2;
       conn.srcuserid = nID;
       // ���ҶԷ���ID�ţ�������Ҳ�������ֱ�ӷ���
       char strname[30];
       sprintf(strname,"%s",m_ui->m_UserList->item(m_nCurrentID)->text().toAscii().data());
       int destid = -1;
       map<int,UserNode>::iterator it;
       for(it=m_mapuserlist.begin(); it != m_mapuserlist.end();it++)
       {
           if(strcmp(it->second.strName,strname) == 0)
           {
               destid = it->second.id;
               strTip+= tr(it->second.strName)+"] ˵:";
           }
       }
       if(destid == -1)
       {
           m_ui->m_EditRecord->append("Error: ���Ҳ���ָ�����û�");
           return ;
       }
       node.destuserid = destid;
    }
    else
    {
        // Ⱥ��
       conn.type=3;
       conn.srcuserid = nID;
       strTip+= "���] ˵:";
    }
    strTip+= str+"\n";
    m_ui->m_EditRecord->append(strTip);
    char buf[1024];
    char *pData = buf;
    ConnProto *pconn = (ConnProto *)pData;
    *pconn = conn;
    pData+=sizeof(ConnProto);
    ChatContent *pnode = (ChatContent *)pData;
    *pnode = node;
    SendInfor(buf,sizeof(ConnProto)+sizeof(ChatContent));
    QMessageBox::information(this, "1",tr("200"));
}
void MainDlg::SendInfor(char *strinfor,int count)
{
    // ���ڷ������ݰ�
    QByteArray datagram;
    for(int i=0; i <count; i++)
        datagram.insert(i,strinfor[i]);
    QHostAddress address;
    address.setAddress(m_ui->Text_IP->text());
    udpSocket->writeDatagram(datagram.data(), qint64(datagram.size()),
                                 address, 9000);
}
void MainDlg::receiveMessage()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());
        char *pData = datagram.data();
        ConnProto *pConn = (ConnProto *)pData;
        pData += sizeof(ConnProto);
        QString strDate = QDateTime::currentDateTime().toString(tr("yyyy-MM-dd  hh:mm:ss"));
        // ��������Բ�ͬ���͵���Ӧ��Ĵ���
        if(pConn->type == 0)
        {
            // ��ǰ��¼�ɹ��������û��б�
            QString str = "��ǰ�û��ѵ�¼�����������û��б�";
            int count = sizeof(ConnProto);
            for(int i = 0; i < pConn->srcuserid; i++)
            {
                if(count >= datagram.size())
                    break;

                UserNode *pNode = (UserNode *)pData;
                this->m_mapuserlist[pNode->id] = *pNode;
                this->m_ui->m_UserList->addItem(new QListWidgetItem(QString(pNode->strName)));
                pData += sizeof(UserNode);
                count += sizeof(UserNode);
            }

        }
        else if(pConn->type == -1)
        {
            // ��ǰ�û��ѵ�¼��������
            QString str = "ϵͳ��ʾ��[��ǰ�û��ѵ�¼���������߱��û�] ["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == -2)
        {
            // ��ǰ�û������ڣ���ע��
            QString str = "ϵͳ��ʾ��[��ǰ�û������ڣ�����ע�᱾�û�]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);

        }
        else if(pConn->type == -3)
        {
             // ��ǰ�û��������
            QString str = "ϵͳ��ʾ��[�û������������������������ٵ�¼]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == 10)
        {
            // �����û�����
            UserContent *pUser = (UserContent *)pData;
            UserNode node ;
            node.id = pConn->srcuserid;
            strcpy(node.strName,pUser->strName);
            QString str = "ϵͳ��ʾ:��ӭ���û�["+tr(node.strName)+"]����["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
            // ���뵽��ǰ�б���
            this->m_mapuserlist[node.id] = node;
            this->m_ui->m_UserList->addItem(new QListWidgetItem(QString(node.strName)));
        }
        else if(pConn->type == 11)
        {
            // ע��ɹ�
            QString str = "ϵͳ��ʾ��[�û���ע��ɹ������¼]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == -11)
        {
             // ��ǰ�û��Ѵ���
            QString str = "ϵͳ��ʾ��[��ǰҪע����û��Ѵ��ڣ�����������Ϣ]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == 2)
        {
            // ˽�Ĵ���            
            ChatContent *pNode = (ChatContent*)pData;
            int id = pNode->destuserid;
            map<int,UserNode>::iterator it = this->m_mapuserlist.find(id);
            if(it != m_mapuserlist.end())
            {

                QString str = "ϵͳ��ʾ:�û�["+tr(it->second.strName)+"]�� [��] ˵ ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
            else
            {
                QString str = "ϵͳ��ʾ:[һ������ʶ���˽����Ϣ] ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
        }
        else if(pConn->type == 3)
        {
            // Ⱥ�Ĵ���
            ChatContent *pNode = (ChatContent*)pData;
            int id = pNode->destuserid;
            map<int,UserNode>::iterator it = this->m_mapuserlist.find(id);
            if(it != m_mapuserlist.end())
            {

                QString str = "ϵͳ��ʾ:�û�["+tr(it->second.strName)+"]�� [���] ˵ ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
            else
            {
                QString str = "ϵͳ��ʾ:[һ������ʶ���Ⱥ����Ϣ] ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
        }
        else if(pConn->type == 4)
        {
            // �˳�����
            int id = pConn->srcuserid;
            map<int,UserNode>::iterator it = this->m_mapuserlist.find(id);
            if(it != m_mapuserlist.end())
            {
                QString str = "ϵͳ��ʾ:�û�["+tr(it->second.strName)+"] �˳� ["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
                // ���뵽��ǰ�б���
                this->m_mapuserlist.erase(id);                
               for(int i =0; i < m_ui->m_UserList->count(); i++)
               {
                    QListWidgetItem *item = this->m_ui->m_UserList->item(i);
                    if(item->text().compare(it->second.strName) == 0)
                    {
                        m_ui->m_UserList ->removeItemWidget( item );
                        break;
                    }
                }
            }
            else
            {
                 QString str = "ϵͳ��ʾ:[һ������ʶ����û��˳�����]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
        }
        else
        {
            QString str = "ϵͳ��ʾ��[��ǰ�в�ʶ�������]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
    }
}
void MainDlg::InitUDPSocket()
{
    QString str = m_ui->Text_Port->text();
    if(str.length() <= 0)
    {
       QMessageBox::information(this, "����","������˿ں�");
       return ;
    }

    char port[30];
    sprintf(port,"%d",str.toInt());
    QString strTip;
    strTip = tr(port);
    this->m_ui->m_EditRecord->append(str+strTip);
    //����UDP���ݰ�����������ݰ��ִ����receiveessag�۽��д���
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(str.toInt());
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(receiveMessage()));
}
void MainDlg::on_Button_Login_clicked()
{
    // �йص�¼���¼�����
    strName = m_ui->Text_Name->text();
    strPassword = m_ui->Text_Pass->text();
    strIP = m_ui->Text_IP->text();
    nID = m_ui->Text_ID->text().toInt();
    if(strName.length()<= 0
       || strPassword.length()<=0
       || strIP.length() <= 0)
    {
        QMessageBox::information(this, "����","��������ȷ�Ĳ�����Ϣ");
        return ;
    }
    if(NULL == udpSocket)
        InitUDPSocket();

    ConnProto conn;
    conn.type=0;
    conn.srcuserid = nID;
    UserContent node;
    sprintf(node.strName,"%s",strName.toAscii().data());
    sprintf(node.password,"%s",strPassword.toAscii().data());
    char buf[1024];
    char *pData = buf;
    ConnProto *pconn = (ConnProto *)pData;
    *pconn = conn;
    pData+=sizeof(ConnProto);
    UserContent *pnode = (UserContent *)pData;
    *pnode = node;
    SendInfor(buf,sizeof(ConnProto)+sizeof(UserContent));
    QString strTip;
    strTip=tr("��Ϣ��ʾ:[��¼] �û���:[")+tr(node.strName)+tr("]����:[")+tr(node.password)+tr("]");
    this->m_ui->m_EditRecord->append(strTip);
}

void MainDlg::on_Button_Reg_clicked()
{
    // �й�ע����¼�����
    QString strName = m_ui->Text_Name->text();
    QString strPassword = m_ui->Text_Pass->text();
    QString strIP = m_ui->Text_IP->text();
    QString strID = m_ui->Text_ID->text();
    if(strName.length()<= 0
       || strPassword.length()<=0
       || strIP.length() <= 0)
    {
        QMessageBox::information(this, "����","��������ȷ�Ĳ�����Ϣ");
        return ;
    }
    if(NULL == udpSocket)
        InitUDPSocket();

    ConnProto conn;
    conn.type=1;
    conn.srcuserid = strID.toInt();
    UserContent node;
    sprintf(node.strName,"%s",strName.toAscii().data());
    sprintf(node.password,"%s",strPassword.toAscii().data());
    char buf[1024];
    char *pData = buf;
    ConnProto *pconn = (ConnProto *)pData;
    *pconn = conn;
    pData+=sizeof(ConnProto);
    UserContent *pnode = (UserContent *)pData;
    *pnode = node;
    QString strTip;
    strTip=tr("��Ϣ��ʾ:[ע��] �û���:[")+tr(node.strName)+tr("]����:[")+tr(node.password)+tr("]");
    this->m_ui->m_EditRecord->append(strTip);
    SendInfor(buf,sizeof(ConnProto)+sizeof(UserContent));
}

void MainDlg::on_Button_Exit_clicked()
{
    // �й��˳�
    on_m_ButtonExit_clicked();
    exit(0);
}

void MainDlg::on_m_ButtonExit_clicked()
{
    // �й����ߵ��¼�����
    if(strName.length()<= 0
       || strPassword.length()<=0
       || strIP.length() <= 0)
    {
        QMessageBox::information(this, "����","��������ȷ�Ĳ�����Ϣ");
        return ;
    }
    if(NULL == udpSocket)
        InitUDPSocket();

    ConnProto conn;
    conn.type=4;
    conn.srcuserid = nID;
    char buf[1024];
    char *pData = buf;
    ConnProto *pconn = (ConnProto *)pData;
    *pconn = conn;
    SendInfor(buf,sizeof(ConnProto));
    QString strTip;
    strTip=tr("��Ϣ��ʾ:��ǰ�û��������� ");
    this->m_ui->m_EditRecord->append(strTip);
}

void MainDlg::on_m_UserList_currentRowChanged(int currentRow)
{
    // ��ѡ��ʱ����ǰѡ�е��б�
    if(currentRow == 0)
        m_nCurrentID = -1;
    else
    {
        m_nCurrentID = currentRow;
    }
}
