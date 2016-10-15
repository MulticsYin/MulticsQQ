create database chat;
use chat;
create table user_infor( 
    id       int primary key,
    name     varchar(20) not null,
    password varchar(10) not null,
    regtime  datetime not null
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
