rm -f IMServer
g++ -o IMServer  $(mysql_config --cflags) IMServer.cpp  $(mysql_config --libs) -lpthread

