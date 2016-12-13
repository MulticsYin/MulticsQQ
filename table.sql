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

create table user_chat( 
    recid   int auto_increment primary key,
    srcid   int not null,
    destid  int ,
    type    int not null,
    content varchar(200) not null,
    centime datetime not null,
    strip   varchar(15) not null,
    port    int not null
);

create table user_log( 
    recid   int auto_increment primary key,
    srcid   int not null,
    type    int not null,
    protime datetime not null,
    proendtime datetime,
    strip   varchar(15) not null,
    port    int not null
);
