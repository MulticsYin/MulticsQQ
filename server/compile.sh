rm -f output
g++ -o output  $(mysql_config --cflags) IMServer.cpp  $(mysql_config --libs) -lpthread