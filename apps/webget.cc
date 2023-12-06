#include "socket.hh"
#include <vector>
#include <cstdlib>
#include <iostream>
#include <random>
#include <span>
#include <string>

using namespace std;
vector<string>MoreRequest;
void get_URL( const string& host, const string& path )
{   
      Address addr(host,"http");//连接的口子（插头）
      TCPSocket tcp_socket;//建立套接字，用来连接（有了插坐）
 	try{
        	tcp_socket.connect(addr);
    	}
    	catch ( const exception& e ) {
    	cerr << "Unable to connect:"<<e.what() << "\n";
    	return ;
  	}
    string DefaultRequest="GET";
    string ConnectionHead = "Close";
    string CacheControlHead="no-cache";
        if(!MoreRequest.empty()){
        for(string &str:MoreRequest){
        CacheControlHead+=(","+str);
        }
    }
    string RequestHead(
    DefaultRequest+" "+path+ " HTTP/1.1\r\n" );
    RequestHead+="Host: " + host+ "\r\n";
    RequestHead+="Cache-Control: " + CacheControlHead + "\r\n";
    RequestHead+="Connection: " + ConnectionHead + "\r\n\r\n";
    tcp_socket.write(RequestHead);
    string outinfo;
  tcp_socket.read( outinfo );
  while ( !outinfo.empty() ) {
    cout << outinfo;
    tcp_socket.read( outinfo );
  }
    tcp_socket.close();
}


int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
