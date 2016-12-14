#该文件告诉你怎么编译，实际情况下要视系统而定，在我的系统下我直接拷贝命令编译
rm -f IMServer
g++ -o IMServer  $(mysql_config --cflags) IMServer.cpp  $(mysql_config --libs) -lpthread
