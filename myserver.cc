#include <stdio.h> // basic I/O
#include <stdlib.h>
#include <sys/types.h> // standard system types
#include <netinet/in.h> // Internet address structures
#include <sys/socket.h> // socket API
#include <arpa/inet.h>
#include <netdb.h> // host to IP resolution
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <iostream>
#include <cstring>
#include <fstream>

using namespace std;

#define BUFFER_SIZE 256

// prototype
void parametercheck(int argc, char *argv[]);
int openSocket(void);
void socketBind(int socket_id, struct sockaddr_in srv_ad);
int acceptConnection(int s_socket_id, struct sockaddr_in cli_ad);
void rcvMsg(int socket_id, char buff[], int buff_size);
void sendMsg(int socket_id, char buff[], int buff_size);
void clearBuff(char buff[]);
void sendMsgHello(int socket_id);
void welcomeMsg(int port);
void acceptMsg(struct sockaddr_in cli_ad);
void clientDisconnectMsg(void);
string getCurrentDir(void);
bool isExists(string f_name);
bool createFile(string f_name);
void sndFile(int c_socket_fd, char buff[], string f_name);
void rcvFile(int c_socket_fd, char buff[], string f_name);

int main(int argc, char *argv[])
{
	// variable definitions
	int s_sock_fd, c_socket_fd, port;
	socklen_t clilen;
	struct sockaddr_in srv_ad, cli_ad;
	char buff[BUFFER_SIZE];

	parametercheck(argc, argv);	// check parameters

	port = atoi(argv[1]);	// getting port no.

	s_sock_fd = openSocket();	// open socket

	// server details
	srv_ad.sin_family = AF_INET;
	srv_ad.sin_addr.s_addr = INADDR_ANY;
	srv_ad.sin_port = htons(port);

	socketBind(s_sock_fd, srv_ad);	// binding socket

	welcomeMsg(port);

	listen(s_sock_fd, 5);	// starts listening for clients

	while (true)
	{
		c_socket_fd = acceptConnection(s_sock_fd, cli_ad);	// accepting connection from client

		acceptMsg(cli_ad);
		
		sendMsgHello(c_socket_fd);
		
		bool flag = true;
		
		do
		{
			clearBuff(buff);
			rcvMsg(c_socket_fd, buff, 4);
			
			if (strncmp(buff, "list", 4) == 0)
			{
				string file_list = getCurrentDir();
				
				clearBuff(buff);
				strcpy(buff, file_list.c_str());

				sendMsg(c_socket_fd, buff, strlen(buff));

				cout << "\nSending file list ....\n\n";
			}
			else if (strncmp(buff, "_noA", 4) == 0)
			{
				// do nothing
			}
			else if (strncmp(buff, "_del", 4) == 0)
			{
				clearBuff(buff);
				rcvMsg(c_socket_fd, buff, BUFFER_SIZE);

				string f_name = buff;

				if (!isExists(f_name))
				{
					clearBuff(buff);
					string ack = "Server: '" + f_name + "' does not exist.";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));
				}
				else if (!remove(f_name.c_str()))
				{

					clearBuff(buff);
					string ack = "Server: '" + f_name + "' has been deleted.";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));
				}
				else
				{

					clearBuff(buff);
					string ack = "Server: File could not be deleted.";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));	
				}
			}	
			else if (strncmp(buff, "_cre", 4) == 0)
			{
				clearBuff(buff);
				rcvMsg(c_socket_fd, buff, BUFFER_SIZE);

				string f_name = buff;
				
				if (isExists(f_name))
				{
					clearBuff(buff);
					string ack = "Server: '" + f_name + "' already exists.";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));
				}
				else
				{
					if (createFile(f_name))
					{
						clearBuff(buff);
						string ack = "Server: '" + f_name + "' has been created.";
						strcpy(buff, ack.c_str());
						sendMsg(c_socket_fd, buff, strlen(buff));	
					}
					else
					{
						clearBuff(buff);
						string ack = "Server: File could not be created\n";
						strcpy(buff, ack.c_str());
						sendMsg(c_socket_fd, buff, strlen(buff));		
					}
					
				}
			}
			else if (strncmp(buff, "recv", 4) == 0)
			{
				clearBuff(buff);
				rcvMsg(c_socket_fd, buff, BUFFER_SIZE);

				string f_name = buff;

				if (!isExists(f_name))
				{
					clearBuff(buff);
					string ack = "n_ex";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));
				}
				else
				{
					clearBuff(buff);
					string ack = "_exi";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));	
				
					sndFile(c_socket_fd, buff, f_name);
				}
			}
			else if (strncmp(buff, "send", 4) == 0)
			{
				clearBuff(buff);
				rcvMsg(c_socket_fd, buff, BUFFER_SIZE);

				string f_name = buff;

				if (isExists(f_name))
				{
					clearBuff(buff);
					string ack = "_ex";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));
				}
				else
				{
					clearBuff(buff);
					string ack = "_ok";
					strcpy(buff, ack.c_str());
					sendMsg(c_socket_fd, buff, strlen(buff));	

					// recv file
					rcvFile(c_socket_fd, buff, f_name);
				}
			}
			else if (strncmp(buff, "exit", 4) == 0)
			{
				flag = false;
			}
			else
			{
				clearBuff(buff);
				string helpMsg = "\n> Invalid Command\n> HELP : \n\n> LIST server\n> LIST client\n> Create client <filename>\n> Create server <filename>\n> SEND <filename>\n> RECIEVE <filename>\n> exit\n\n";

				strcpy(buff, helpMsg.c_str());
				
				sendMsg(c_socket_fd, buff, strlen(buff));

				cout << "c: invalid command\n"
					<< "Sending help...\n\n";
			}

		} while (flag);

		clientDisconnectMsg();

		close(c_socket_fd);	
	}
	
	close(s_sock_fd);
	return 0;

}

void parametercheck(int argc, char* argv[])
{
	if (argc != 2)
    {
      fprintf(stderr, "Usage: %s <port>\n", argv[0]);
      exit(-1);
    }
}

int openSocket(void)
{
	int socket_id = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_id < 0)
	{
		cout << "Socket Opening Failed\n";
		exit(-1);
	}
	return socket_id;
}

void socketBind(int socket_id, struct sockaddr_in srv_ad)
{
	int srv_size = sizeof(srv_ad);
	if (bind(socket_id, (struct sockaddr *) &srv_ad, srv_size) < 0)
	{
		cout << "Socket Binding Failed\n";
		exit(-1);
	}
}

int acceptConnection(int s_socket_id, struct sockaddr_in cli_ad)
{
	socklen_t clilen;
	clilen = sizeof(cli_ad);

	int c_socket_id = accept(s_socket_id, (struct sockaddr *) &cli_ad, &clilen);
	if (c_socket_id < 0)
	{
		cout << "Connection acceptance failed\n";
		exit(-1);
	}	
	return c_socket_id;
}

void rcvMsg(int socket_id, char buff[], int buff_size)
{
	int x = read(socket_id, buff, buff_size);
	if (x < 0)
	{
		cout << "Failure of reading from socket\n";
		exit(-1);
	}
}

void sendMsg(int socket_id, char buff[], int buff_size)
{
	int x = write(socket_id, buff, buff_size);
	if (x < 0)
	{
		cout << "Failure of writing to socket\n";
		exit(-1);
	}
}

void clearBuff(char buff[])
{
	bzero(buff, BUFFER_SIZE);
}

void sendMsgHello(int socket_id)
{
	int x = write(socket_id, "Hello from Asgard", 33);
	if (x < 0)
	{
		cout << "Failure of writing to socket\n";
		exit(-1);
	}	

}

void welcomeMsg(int port)
{
	cout << "\nStarting to run server at port " << port
		<< "\n.. creating local listener socket\n"
		<< ".. binding socket to port:"<< port
		<< "\n.. starting to listen at the port\n"
		<< ".. waiting for connection \n\n";
}

void acceptMsg(struct sockaddr_in cli_ad)
{
	cout << "\nA new client connected to server...\n\n";
}

void clientDisconnectMsg(void)
{
	cout << "\nClient Disconnected...\n\n"
		<< "Listening to clients again... \n\n";
}

string getCurrentDir(void)
{
	string file_list = "\n> Server : File List\n";
	DIR *_dir;
	struct dirent *_files;

	_dir = opendir("./");
	if (_dir != NULL)
	{
		while (_files = readdir(_dir))
		{
			file_list += "> ";
			file_list += _files->d_name;
			file_list += "\n";
		}
	}

	return file_list;
}

bool isExists(string f_name)
{

	ifstream in_file;
	in_file.open(f_name.c_str());
	if (in_file.fail())
	{
		return false;
	}
	
	in_file.close();
	return true;
}

bool createFile(string f_name)
{
	ofstream of_file;
	of_file.open(f_name.c_str());
	if (of_file.fail())
	{
		return false;
	}
	
	of_file.close();
	return true;
}

void rcvFile(int c_socket_fd, char buff[], string f_name)
{
	ofstream out_file;
	out_file.open(f_name.c_str());
	
	clearBuff(buff);
	rcvMsg(c_socket_fd, buff, 4);
	cout << buff << endl;	//t
	while (strncmp(buff, "_con", 4) == 0)
	{
		clearBuff(buff);
		rcvMsg(c_socket_fd, buff, BUFFER_SIZE);

		string temp = buff;
		out_file << temp;
		cout << temp << endl;	//

		clearBuff(buff);
		string ack = "_ok";
		strcpy(buff, ack.c_str());
		sendMsg(c_socket_fd, buff, strlen(buff));
		cout << buff << endl;	//t

		clearBuff(buff);
		rcvMsg(c_socket_fd, buff, 4);
		cout << buff << endl;	// t
	}

	out_file.close();
}

void sndFile(int c_socket_fd, char buff[], string f_name)
{
	ifstream in_file;
	in_file.open(f_name.c_str());
	string cmd;
	while (!in_file.eof())
	{	
		clearBuff(buff);
		cmd = "_con";
		strcpy(buff, cmd.c_str());
		
		sendMsg(c_socket_fd, buff, strlen(buff));	

		string temp;
		getline(in_file, temp);
		temp += "\n";

		clearBuff(buff);
		strcpy(buff, temp.c_str());

		sendMsg(c_socket_fd, buff, strlen(buff));

		clearBuff(buff);
		rcvMsg(c_socket_fd, buff, 3);
	}

	clearBuff(buff);
	cmd = "_fin";
	strcpy(buff, cmd.c_str());

	sendMsg(c_socket_fd, buff, strlen(buff));	

	in_file.close();
}