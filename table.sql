#为聊天软件建立相应数据库，表
create database chat;
use chat;

#用户信息表
create table user_infor( 
    id       int primary key,      #用于表示用户的ID号，全局唯一;
    name     varchar(20) not null, #用于表示用户的代号;
    password varchar(10) not null, #用于存储用户的密码;
    regtime  datetime not null     #用于表示用户注册的时间;
);

#用户聊天信息表
create table user_chat( 
    recid   int auto_increment primary key, #记录ID，表内唯一，自动增长;
    srcid   int not null,                   #发起人的ID号;
    destid  int ,                           #目标的ID号;
    type    int not null,                   #聊天类型：0为群聊，1为私聊;
    content varchar(200) not null,          #聊天的内容;
    centime datetime not null,              #发起聊天的时间;
    strip   varchar(15) not null,           #聊天发起者的IP;
    port    int not null                    #聊天发起者的端口号;
);

#用户的日志表
create table user_log( 
    recid   int auto_increment primary key, #记录ID，表内唯一，自动增长;
    srcid   int not null,                   #用户ID号;
    type    int not null,                   #用户行为：0-登录，1-注册，2-私聊，3-群聊，4-退出，5-不能识别的操作;
    protime datetime not null,              #操作时间;
    proendtime datetime,                    #记录用户退出时间;
    strip   varchar(15) not null,           #用户IP地址;
    port    int not null                    #用户端口号;
);
