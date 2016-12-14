#该文件告诉你怎么编译，实际情况下要视系统而定，在我的系统下我直接拷贝命令编译
rm -f MulticsQQ_Server
g++ -o MulticsQQ_Server  $(mysql_config --cflags) MulticsQQ_Server.cpp  $(mysql_config --libs) -lpthread
