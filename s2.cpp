#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/time.h>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <thread>

using namespace std;

#define MAXFD 10	//Size of fds array


/////////// global variables ///////////////
pthread_t tid;
int server_flag = 1;
int proxy_fd;
string routingTable;
string proxy_recv_msg;
string client_port;
string route_ip;
bool client_recv_status = false;

///////////////////////////////////////////

string charPtrToString(char* toConvert) 
{
	string str;
	for (int i = 0; toConvert[i]; i++)
		str += toConvert[i];

	return str;
}

void* proxy_thread(void* arg) {
	char buff1[1000] = {};

	while (1) {
		recv(proxy_fd, buff1, sizeof(buff1), 0);
		char  rt[200];
		strcpy(rt, buff1);
		//cout << "Message recieved in thread = " <<endl<< buff1 << endl;
		
		char flag = buff1[0];

		/// message about any client connection
		if (flag == '0') {
			string s = charPtrToString(buff1);
			memset(buff1, 0, sizeof(buff1));

			s.erase(0, 1);
			cout << s << endl;
		}

		/// UPDATED - displays connection info + routing table
		else if (flag == '1') {
			string s = charPtrToString(buff1);
			memset(buff1, 0, sizeof(buff1));

			s.erase(0, 1);

			int pos = s.find("+");
			string connection = s.substr(0, pos);
			cout << connection << endl;

			s.erase(0, pos + 1);
			cout<<"=================================================="<<endl;
			cout << s << endl;
			cout<<"=================================================="<<endl;
			routingTable = s;
		}

		// proxy is sending ip/route info
		else if (flag == '3') {
			route_ip = charPtrToString(buff1);
			memset(buff1, 0, sizeof(buff1));
			route_ip.erase(0, 2); // erasing flag
		}

		// proxy is sending routing table
		else if (flag == '9') {
			string s = charPtrToString(buff1);
			memset(buff1,0,sizeof(buff1));

			s.erase(0, 1);
			routingTable = s;

			cout << routingTable << endl;
			s.clear();
		}

		else if (flag == '7') {
			string msg = charPtrToString(buff1);
			memset(buff1,0,sizeof(buff1));

			// removing flag info at start of buff/string
			msg.erase(0, 2); 

			// extracting port of recieving client
			int pos = msg.find('+');
			client_port = msg.substr(0, pos);

			// removing port from msg string
			msg.erase(0, pos); 

			// removing leftover '+' at start
			msg.erase(0, 1); 
			proxy_recv_msg = msg;

			cout << "Port to send to: " << client_port << " and msg = " << proxy_recv_msg << endl;
			//client_recv_status = true;
		}

		else if (flag == '8') {
			string msg = charPtrToString(buff1);
			memset(buff1,0,sizeof(buff1));

			msg.erase(0, 1);

			proxy_recv_msg = msg;
			client_recv_status = true;

			cout << "proxy msg = " << proxy_recv_msg << endl;
		}

		else {
			memset(buff1,0,sizeof(buff1));
			cout << rt << endl;
		}
	}

	return NULL;
}

void sendMsgToServer(int port, string flag, string msg) {
	int sockfd = socket(AF_INET,SOCK_STREAM,0);	
	assert(sockfd != -1 );

	//Set Address Information
	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//Link to server
	int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res != -1);

	send(sockfd, flag.c_str(), flag.size(), 0);
	send(sockfd, msg.c_str(), msg.size(), 0);
	close(sockfd);
}

void fds_add(int fds[],int fd)	//Add a file descriptor to the fds array
{
	int i=0;
	for(;i<MAXFD;++i)
	{
		if(fds[i]==-1)
		{
	      fds[i]=fd;
		  break;
		}
	}
}

int main()
{
	/////////////////////////// Connection to proxy ////////////////////
	
	proxy_fd = socket(AF_INET,SOCK_STREAM,0);	
	assert(proxy_fd != -1 );

	//Set Address Information
	struct sockaddr_in p_saddr;
	memset(&p_saddr,0,sizeof(p_saddr));
	p_saddr.sin_family = AF_INET;
	p_saddr.sin_port = htons(20003);
	p_saddr.sin_addr.s_addr = inet_addr("192.168.100.21");

	//Link to server
	int res1 = connect(proxy_fd,(struct sockaddr*)&p_saddr,sizeof(p_saddr));
	assert(res1 != -1);

	///////////////////////// recv thread ////////////////////////////////

	pthread_create(&tid, NULL, proxy_thread, NULL);

	//////////////////////// local variables //////////////////////////////

	string s_name = "S2";
	int s_port = 20002;

	int c3_port = 0, c4_port = 0;
	int c3_fd = 0, c4_fd = 0;
	string c3_str, c4_str;

	string clientports = "\nServer\tClient\tPort\n";

	////////////// creating its own server socket ////////////////////////

	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd!=-1);
    
	struct sockaddr_in saddr,caddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(s_port);
	saddr.sin_addr.s_addr=inet_addr("192.168.100.21");

	int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res!=-1);

	////////////////////////////////////////////////////////////////////
	
	//Create listening queue
	listen(sockfd,5);
	
   //Define fdset collection
    fd_set fdset;
	
	//Define fds array
    int fds[MAXFD];
    int i=0;
    for(;i<MAXFD;++i)
	  	fds[i]=-1;
	
    fds_add(fds,sockfd);

	while(1)
    {
		FD_ZERO(&fdset);

		int maxfd=-1;

		int i=0;
		for(;i<MAXFD;i++)
		{
			if(fds[i]==-1)
				continue;

			FD_SET(fds[i],&fdset);

			if(fds[i]>maxfd)
				maxfd=fds[i];
		}

		struct timeval tv={2,0};

		int n=select(maxfd+1,&fdset,NULL,NULL,&tv);
		if(n==-1)	//fail
			perror("select error");
			
		else if(n==0){}
			//printf("time out\n");

		else
		{
			for(i=0;i<MAXFD;++i)
			{
				if(fds[i]==-1)
					continue;
				
				if(FD_ISSET(fds[i],&fdset))
				{
					if(fds[i]==sockfd)
					{
						struct sockaddr_in caddr;
						int len=sizeof(caddr);

						int c=accept(sockfd,(struct sockaddr *)&caddr,(socklen_t*)&len);	//Accept new client connections
						if(c<0)
						{
							continue;
						}
					
						fds_add(fds,c);//Add the connection socket to the array where the file descriptor is stored

						// creating c3 data and sending to proxy
						if (c3_port == 0) 
						{
							c3_port = (int)ntohs(caddr.sin_port);
							c3_str = "S2\tC3\t" + to_string(c3_port);
							c3_str += "\n";
							c3_fd = c;
							clientports.append(c3_str);

							string temp = "02C3 connected to S2";
							temp = temp + "+" + c3_str;
							send(proxy_fd, temp.c_str(), temp.size(), 0);
						}

						// creating c4 data and sending to proxy
						else if (c4_port == 0) 
						{
							c4_port = (int)ntohs(caddr.sin_port);
							c4_str = "S2\tC4\t" + to_string(c4_port);
							c4_str += "\n";
							c4_fd = c;
							clientports.append(c4_str);

							string temp = "02C4 connected to S2";
							temp = temp + "+" + c4_str;
							send(proxy_fd, temp.c_str(), temp.size(), 0);
						}

						// send ports to proxy when two clients connected to server
						if (c3_port and c4_port) {

							// 1 = "two clients connected" and 5 = "send routing table"
							string temp = "25";
							temp += c3_str + c4_str;

						//	send(proxy_fd, temp.c_str(), temp.size(), 0);
						}
					}
					else   //Receive data recv when an existing client sends data
					{
						char buff[128]={0};
						int res = recv(fds[i],buff,127,0);

						if(res<=0)
						{
							if (fds[i] == c3_fd) { // client 1 has exited
								clientports.clear();
								c3_str.clear();
								clientports = "Server\tClient\tPort\n";
								clientports.append("S2\tC4\t" + to_string(c4_port));
								clientports += "\n";
								c3_port = 0;
							}

							else if (fds[i] == c4_fd) { // client 2 has exited
								clientports.clear();
								c4_str.clear();
								clientports = "Server\tClient\tPort\n";
								clientports.append("S2\tC3\t" + to_string(c3_port));
								clientports += "\n";
								c4_port = 0;
							}

							close(fds[i]);
							fds[i]=-1;
						}

						else
						{
							char choice = buff[0];

							cout << "buff = " << buff << endl;

							// sending client
							if (choice == '1') 
							{
								server_flag = 1;

								cout<<"Client selected option "<<buff<<endl;
								memset(buff,0,sizeof(buff));

							//	send(proxy_fd, "5", 1, 0); 										// asking proxy for updated routing table
								send(fds[i], routingTable.c_str(), routingTable.size(), 0);		//Sending ports of clients connected to all servers

								recv(fds[i],buff,127,0);										//Server will recieve the port number of client it wants to communicate with
								int p = atoi(buff);
								memset(buff,0,sizeof(buff));

								cout<<"Client to comm with = "<<p<<endl;

								//recv(fds[i],buff,127,0); // Message to forward to client
								//send(fds[i], "Message sent", 12, 0);

								int recv_fd;

								// communication between server's own clients
								if (p == c3_port or p == c4_port) {

									if (p == c3_port)
										recv_fd = c3_fd;

									else if (p == c4_port)
										recv_fd = c4_fd;

									while (1) {

										memset(buff, 0, 127);
										recv(fds[i],buff,127,0);

										string msg = "Message: ";
										msg += charPtrToString(buff);
										memset(buff,0,sizeof(buff));

										send(recv_fd, msg.c_str(), msg.size(), 0);

										msg.clear();
										recv(recv_fd,buff,127,0);
										msg = "Message: ";
										msg += charPtrToString(buff);

										send(fds[i], msg.c_str(), msg.size(), 0);
										msg.clear();
									}
								}

								// communication with another server's clients
								else {
									while (1) {
										char buff1[500];
										memset(buff1, 0, sizeof(buff1));
										recv(fds[i],buff1, sizeof(buff1),0);
										string flag;

										if (strncmp(buff1, "close", 5) == 0)
											flag = "7"; // this will indicate to proxy that connection is to be closed
										else
											flag = "6"; // this will indicate to proxy that a message is coming to be forwarded
										

										string msg;
										msg += charPtrToString(buff1); // message the client wants to send
										memset(buff1,0,sizeof(buff1));
										string port_str = to_string(p); // port string
										//string flag = "6"; // this will indicate to proxy that a message is coming to be forwarded

										string toSend = flag + "+" + port_str + "+" + msg;
										
										cout << "String sent to proxy: " << toSend << endl;
										send(proxy_fd, toSend.c_str(), toSend.size(), 0);

										if (flag == "7")
											break;

										while (proxy_recv_msg.empty()) {
												int s = 0;
										}

										// sending return message to client
										cout<<"FDS Of RECIEVER IS = "<<fds[i]<<endl;
										
										send(fds[i], proxy_recv_msg.c_str(), sizeof(proxy_recv_msg), 0);
										cout<<"Message delivered to client is = "<<proxy_recv_msg<<endl;
										proxy_recv_msg.clear();
									}
								}
							}

							// recieving client
							else if (choice == '2') {
								cout<<"Client selected option "<<buff<<endl;

								cout << "Client recv port = " << client_port << endl;

								int recv_fd;
								char buff2[200];

								if (client_port == to_string(c3_port))
									recv_fd = c3_fd;

								else if (client_port == to_string(c4_port))
									recv_fd = c4_fd;

								while (1) {
									// server sending msg to receiver client

									while (proxy_recv_msg.empty()) {
										int s = 0;
									}

									send(recv_fd, proxy_recv_msg.c_str(), proxy_recv_msg.size(), 0);

									if (proxy_recv_msg == "close")
										break;

									// reciever client sending message back
									recv(fds[i], buff2, sizeof(buff2), 0); 

									cout << "Forwarding (buff2): " << buff2 << endl;

									// server forwarding this message to proxy server
									send(proxy_fd, buff2, sizeof(buff2), 0);
									memset(buff2,0,sizeof(buff2));

									proxy_recv_msg.clear();
								}
							}

							else if (choice == '3') {
								cout<<"Client selected option "<<buff<<endl;
								memset(buff,0,sizeof(buff));

								string client_name;

								if (fds[i] == c3_fd)
									client_name = "C3";

								else if (fds[i] == c4_fd)
									client_name = "C4";

								cout << "Client name = " << client_name << endl;

								char domain[20];

								recv(fds[i], domain, sizeof(domain), 0);		//Recieving the domain name from its client

								cout << "Domain recieved = " << domain << endl;
								string dom = "9+" + s_name + "+";				// flag + server name 
								dom += charPtrToString(domain);					// flag + server name + domain
								
								memset(domain,0,sizeof(domain));

								send(proxy_fd, dom.c_str(), dom.size(), 0);		//Forwarding the domain name to proxy server

								while (route_ip.empty()) {
									int i = 0;
								}

								string all_details = client_name + "+" + route_ip;
								send(fds[i], all_details.c_str(), all_details.size(), 0);

								all_details.clear();
								memset(buff,0,sizeof(buff));
								route_ip.clear();
							}
						}
					}
				}
			}
		}
	}
}