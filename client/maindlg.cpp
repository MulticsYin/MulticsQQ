#include "maindlg.h"
#include "ui_maindlg.h"
#include <QtGui>

MainDlg::MainDlg(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::MainDlg)
{
    m_ui->setupUi(this);
    udpSocket = NULL;
    m_nCurrentID = -1; // 表示全部的信息
    m_ui->m_UserList->insertItem(0,"群聊");
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

    // 有关发送的事件处理
    QString str = m_ui->m_EditInput->toPlainText();
    if(str.length() <= 0 || str.length()> 200)
    {
        QMessageBox::information(this, "小提示",tr("发送信息为空或者大于200"));
        return ;
    }
    QString strTip = tr("[我]对[");
    //m_ui->m_UserList->currentIndex()
    ConnProto conn;
    ChatContent node;
    sprintf(node.strContent,"%s",str.toAscii().data());
    if(m_nCurrentID != -1)
    {
       // 私聊
       conn.type=2;
       conn.srcuserid = nID;
       // 查找对方的ID号，如果查找不到，则直接返回
       char strname[30];
       sprintf(strname,"%s",m_ui->m_UserList->item(m_nCurrentID)->text().toAscii().data());
       int destid = -1;
       map<int,UserNode>::iterator it;
       for(it=m_mapuserlist.begin(); it != m_mapuserlist.end();it++)
       {
           if(strcmp(it->second.strName,strname) == 0)
           {
               destid = it->second.id;
               strTip+= tr(it->second.strName)+"] 说:";
           }
       }
       if(destid == -1)
       {
           m_ui->m_EditRecord->append("Error: 查找不到指定的用户");
           return ;
       }
       node.destuserid = destid;
    }
    else
    {
        // 群聊
       conn.type=3;
       conn.srcuserid = nID;
       strTip+= "大家] 说:";
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
    // 用于发送数据包
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
        // 以下是针对不同类型的响应码的处理
        if(pConn->type == 0)
        {
            // 当前登录成功，下载用户列表
            QString str = "当前用户已登录，正在下载用户列表";
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
            // 当前用户已登录，先离线
            QString str = "系统提示：[当前用户已登录，请先离线本用户] ["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == -2)
        {
            // 当前用户不存在，先注册
            QString str = "系统提示：[当前用户不存在，请先注册本用户]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);

        }
        else if(pConn->type == -3)
        {
             // 当前用户密码错误
            QString str = "系统提示：[用户名或密码错误，请重新输入后，再登录]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == 10)
        {
            // 有新用户到来
            UserContent *pUser = (UserContent *)pData;
            UserNode node ;
            node.id = pConn->srcuserid;
            strcpy(node.strName,pUser->strName);
            QString str = "系统提示:欢迎新用户["+tr(node.strName)+"]到来["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
            // 加入到当前列表中
            this->m_mapuserlist[node.id] = node;
            this->m_ui->m_UserList->addItem(new QListWidgetItem(QString(node.strName)));
        }
        else if(pConn->type == 11)
        {
            // 注册成功
            QString str = "系统提示：[用户已注册成功，请登录]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == -11)
        {
             // 当前用户已存在
            QString str = "系统提示：[当前要注册的用户已存在，请更换相关信息]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
        else if(pConn->type == 2)
        {
            // 私聊处理            
            ChatContent *pNode = (ChatContent*)pData;
            int id = pNode->destuserid;
            map<int,UserNode>::iterator it = this->m_mapuserlist.find(id);
            if(it != m_mapuserlist.end())
            {

                QString str = "系统提示:用户["+tr(it->second.strName)+"]对 [我] 说 ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
            else
            {
                QString str = "系统提示:[一条不能识别的私聊信息] ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
        }
        else if(pConn->type == 3)
        {
            // 群聊处理
            ChatContent *pNode = (ChatContent*)pData;
            int id = pNode->destuserid;
            map<int,UserNode>::iterator it = this->m_mapuserlist.find(id);
            if(it != m_mapuserlist.end())
            {

                QString str = "系统提示:用户["+tr(it->second.strName)+"]对 [大家] 说 ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
            else
            {
                QString str = "系统提示:[一条不能识别的群聊信息] ["+tr(pNode->strContent)+"]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
        }
        else if(pConn->type == 4)
        {
            // 退出处理
            int id = pConn->srcuserid;
            map<int,UserNode>::iterator it = this->m_mapuserlist.find(id);
            if(it != m_mapuserlist.end())
            {
                QString str = "系统提示:用户["+tr(it->second.strName)+"] 退出 ["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
                // 加入到当前列表中
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
                 QString str = "系统提示:[一条不能识别的用户退出命令]["+strDate+"]\n";
                this->m_ui->m_EditRecord->append(str);
            }
        }
        else
        {
            QString str = "系统提示：[当前有不识别的命令]["+strDate+"]\n";
            this->m_ui->m_EditRecord->append(str);
        }
    }
}
void MainDlg::InitUDPSocket()
{
    QString str = m_ui->Text_Port->text();
    if(str.length() <= 0)
    {
       QMessageBox::information(this, "错误","请输入端口号");
       return ;
    }

    char port[30];
    sprintf(port,"%d",str.toInt());
    QString strTip;
    strTip = tr(port);
    this->m_ui->m_EditRecord->append(str+strTip);
    //监听UDP数据包，如果有数据包抵达调用receiveessag槽进行处理
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(str.toInt());
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(receiveMessage()));
}
void MainDlg::on_Button_Login_clicked()
{
    // 有关登录的事件处理
    strName = m_ui->Text_Name->text();
    strPassword = m_ui->Text_Pass->text();
    strIP = m_ui->Text_IP->text();
    nID = m_ui->Text_ID->text().toInt();
    if(strName.length()<= 0
       || strPassword.length()<=0
       || strIP.length() <= 0)
    {
        QMessageBox::information(this, "错误","请输入正确的参数信息");
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
    strTip=tr("信息提示:[登录] 用户名:[")+tr(node.strName)+tr("]密码:[")+tr(node.password)+tr("]");
    this->m_ui->m_EditRecord->append(strTip);
}

void MainDlg::on_Button_Reg_clicked()
{
    // 有关注册的事件处理
    QString strName = m_ui->Text_Name->text();
    QString strPassword = m_ui->Text_Pass->text();
    QString strIP = m_ui->Text_IP->text();
    QString strID = m_ui->Text_ID->text();
    if(strName.length()<= 0
       || strPassword.length()<=0
       || strIP.length() <= 0)
    {
        QMessageBox::information(this, "错误","请输入正确的参数信息");
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
    strTip=tr("信息提示:[注册] 用户名:[")+tr(node.strName)+tr("]密码:[")+tr(node.password)+tr("]");
    this->m_ui->m_EditRecord->append(strTip);
    SendInfor(buf,sizeof(ConnProto)+sizeof(UserContent));
}

void MainDlg::on_Button_Exit_clicked()
{
    // 有关退出
    on_m_ButtonExit_clicked();
    exit(0);
}

void MainDlg::on_m_ButtonExit_clicked()
{
    // 有关离线的事件处理
    if(strName.length()<= 0
       || strPassword.length()<=0
       || strIP.length() <= 0)
    {
        QMessageBox::information(this, "错误","请输入正确的参数信息");
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
    strTip=tr("信息提示:当前用户正在离线 ");
    this->m_ui->m_EditRecord->append(strTip);
}

void MainDlg::on_m_UserList_currentRowChanged(int currentRow)
{
    // 当选择时，当前选中的列表
    if(currentRow == 0)
        m_nCurrentID = -1;
    else
    {
        m_nCurrentID = currentRow;
    }
}
