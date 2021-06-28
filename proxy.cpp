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
#include <vector>
#include <sstream>
#include <fstream>
using namespace std;

#define MAXFD 10	//Size of fds array

// used to store actual data of routing table
vector<string> servers;
vector<string> clients;
vector<string> ports;

// proxy cache
vector<string> sites;
vector<string> ips;

// used in cross-communication
int to_send = 0;

// status for circuit switch
bool busy = false;

// find if website is cached in proxy
string getIpFromCache(string site) {
	for (int i = 0; i < sites.size(); i++) {
		if (sites[i] == site)
			return ips[i];
	}

	return "";
}

string getServerFromClientPort(string port) {

	for (int i = 0; i < ports.size(); i++) {
		if (ports[i] == port)
			return servers[i];
	}

	return "";
}

void displayVectors() {
	for (int i = 0; i < sites.size(); i++) {
		cout << sites[i] << " = " << ips[i] << endl;
	}
}

string ip(string domain)
{
    string word;
    string found;       //If the website is found in the DNS Server
    
    ifstream obj("cache.txt", ios::in);
    
    string temp = "www." + domain;          //Adding "www." at start of the domain name
    temp += ".com";                         //Adding ".com" at end of the domain name

    while (!obj.eof())
    {
        obj >> word;                        //Reading word by word

        if (word == temp)
        {
            obj >> found;
            //cout << "IP address of '"<<temp<<"' = " << found << endl;
            return found;
        }
    }
    obj.close();
    return "";
}

void updateCache(string domain, string ip) {
	ofstream out("cache.txt", ios::app);

	if (out) {
		out << "www." << domain << ".com " << ip << endl;
		out.close();
	}
}

void parseRoutingTable(string rtable) {
    servers.clear();
    clients.clear();
    ports.clear();

	// removing header lines
	rtable.erase(0, 34); 

	// finding number of lines in table
    int count = 0;
    for (int i = 0; i < rtable.size(); i++) {
        if (rtable[i] == 10)
            count++;
    }

	// extracting info in each line, storing in vectors
    istringstream ss(rtable);
	string c, p, s;
    for (int i = 0; i < count; i++) {
        ss >> s;
        ss >> c;
        ss >> p;

        servers.push_back(s);
        clients.push_back(c);
        ports.push_back(p);
    }
}

// sends one message to a server/port then closes connection
void sendMsgToServer(int port, string msg) {
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

string charPtrToString(char* toConvert) 
{
	string str;
	for (int i = 0; toConvert[i]; i++)
		str += toConvert[i];

	return str;
}

// returns routing table as a string
string getRoutingTable(string s1, string s2, string s4) {
    string s = "ROUTING TABLE:\n";
    s += "Server\tClient\tPort\n";
    s += s1 + s2 + s4 + "\n";

	//cout << s;
	return s;
}

int main()
{
	string s_name = "S3 (proxy dns)";
	int s_port = 20003;

    string s1_str, s2_str, s4_str;

    int s_fd[3] = {0};
    int s_ports[3] = {20001, 20002, 20004};

	int s_count = 0;

	bool status = false;
	int count = 0;


	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd!=-1);
	
  	//printf("sockfd=%d\n",sockfd);
    
	struct sockaddr_in saddr,caddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(s_port);
	saddr.sin_addr.s_addr=inet_addr("192.168.100.21");

	int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res!=-1);
	
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

	string client;

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

		struct timeval tv={5,0};

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
					// new connections
					if(fds[i]==sockfd)
					{
						struct sockaddr_in caddr;
						int len=sizeof(caddr);

						int c=accept(sockfd,(struct sockaddr *)&caddr,(socklen_t*)&len);
						if(c<0)
						{
							continue;
						}
					
						fds_add(fds,c);
						s_fd[count] = c;
						count++;

					}

					//Receive data recv when an existing client sends data
					else 
					{
						char buff[500]={0};
						int res = recv(fds[i],buff,sizeof(buff),0);

						if(res<=0)
						{
							close(fds[i]);
							fds[i]=-1;
						}

						else
						{
							char flag1 = buff[0];
							char flag2 = buff[1];


							 // a new client has connected
							if (flag1 == '0')
							{
								string temp = charPtrToString(buff);
								string temp2 = temp;
								memset(buff,0,sizeof(buff));

								temp.erase(0, 2);
								int pos = temp.find('+');
								string client_conn = temp.substr(0, pos);

								cout << client_conn << endl; // display a client has connected

								temp.erase(0, pos + 1);

								if (flag2 == '1') { // server 1
									s1_str += temp;
								}

								else if (flag2 == '2') { // server 2
									s2_str += temp;
								}

								else if (flag2 == '4') { // server 4
									s4_str += temp;
								}

								string s = getRoutingTable(s1_str, s2_str, s4_str);
								cout<<"=================================================="<<endl;
								cout << s << endl;
								cout<<"=================================================="<<endl;

								string all_info = "1";
								all_info += client_conn + "+" + s;

								//Send (flagged) message to all servers about client connected to any server + routing table
								send(4, all_info.c_str(), all_info.size(), 0);	// to s1
								send(5, all_info.c_str(), all_info.size(), 0);	// to s2
								send(6, all_info.c_str(), all_info.size(), 0);	// to s4
							}

							 // recieving list of clients of s1
					/*		else if (flag1 == '1') 
							{
                                string str = charPtrToString(buff);
								memset(buff,0,sizeof(buff));
								str.erase(0, 2);
								s1_str = str;

                               cout << getRoutingTable(s1_str, s2_str, s4_str);
                            }

							// recieving list of clients of s2
                            else if (flag1 == '2') 
							{
								cout << "into 2" << endl;
                                //recv(fds[i],buff,sizeof(buff),0);
                                string str = charPtrToString(buff);
								memset(buff,0,sizeof(buff));
								str.erase(0, 2);
								s2_str = str;

                               	cout << getRoutingTable(s1_str, s2_str, s4_str);
                            }

							// recieving list of clients of s4
                            else if (flag1 == '4') 
							{
                                //recv(fds[i],buff,sizeof(buff),0); 
                                string str = charPtrToString(buff);
								memset(buff,0,sizeof(buff));
								str.erase(0, 2);
								s4_str = str;

                               cout << getRoutingTable(s1_str, s2_str, s4_str);
                            } */

							// server requesting routing table (for client)
							else if (flag1 == 5) {
								string rtable = getRoutingTable(s1_str, s2_str, s4_str);
								string rt = "9";
								rt.append(rtable);
								send(fds[i], rt.c_str(), rt.size(), 0);
							}

							// server forwarding client message
							else if (flag1 == '6') {
								busy = true;
								bool status = false;

								while (1) {

									if (status == true) {
										memset(buff ,0,sizeof(buff));
										recv(fds[i], buff, sizeof(buff), 0);
									}

									bool breakLoop = false;

									if (buff[0] == '7')
										breakLoop = true;

									status = true;

									cout << "Server has sent: " << buff << endl;

									string msg = charPtrToString(buff);
									memset(buff,0,sizeof(buff));

									// removing flag info at start of buff/string
									msg.erase(0, 2); 

									// extracting port of recieving client
									int pos = msg.find('+');
									string port = msg.substr(0, pos);

									// removing port from msg string
									msg.erase(0, pos); 

									// removing leftover '+' at start
									msg.erase(0, 1); 
									string data = msg;

									cout << "Data = " << data << endl;

									// extracting server/clients/ports from routing table
									string rtable = getRoutingTable(s1_str, s2_str, s4_str);
									parseRoutingTable(rtable);

									string server = getServerFromClientPort(port);
									cout << "Server = " << server << endl;

									string t = "7";
									data = t + "+" + port + "+" + data;

									if (server == "S1")
										to_send = 4;

									else if (server == "S2")
										to_send = 5;

									else if (server == "S4")
										to_send = 6;

									else
										cout << "Server not found for that client port" << endl;
									
									// sending to relevant server
									send(to_send, data.c_str(), data.size(), 0);

									char buff2[200];

									// recieving return message from relevant server
									recv(to_send, buff2, sizeof(buff2), 0);

									cout << "Message recieved back: " << buff2 << endl;

									//send msg back to original server
									msg = "8";
									msg = msg + charPtrToString(buff2);
									memset(buff2,0,sizeof(buff2));
									cout << fds[i] << endl;
									send(fds[i], msg.c_str(), sizeof(msg), 0);
									msg.clear();

									if (breakLoop == true)
										break;
								}

								busy = false; // communication over, link is open
							}

							// client -> server (requesting IP of site)
							else if (flag1 == '9') {
								string route;

								string msg = charPtrToString(buff);
								msg.erase(0, 2); // removing flag

								int pos = msg.find('+');
								string requesting_server = msg.substr(0, pos);
								msg.erase(0, pos+1);

								string domain = msg;

								cout << "Requesting server = " << requesting_server << endl;
								cout << "Domain requested = " << domain << endl;

								string cache_IP = ip(domain);
								cout << cache_IP << endl;

							//	displayVectors();

								if (cache_IP.empty()) {
									route += requesting_server + " -> S3 (proxy) -> S4 -> ||DNS Server|| -> S4 -> S3 (proxy) -> " + requesting_server;

									domain = "2" + domain;
									send(6, domain.c_str(), domain.size(), 0);		// sending site to s4

									char ip[30];
									recv(6, ip, sizeof(ip), 0);						// recieving ip from s4
									string ip_str = charPtrToString(ip);

									cout << "IP = " << ip_str << endl;

									string ip_to_send = "3+" + route + "+" + ip_str;

									send(fds[i], ip_to_send.c_str(), ip_to_send.size(), 0);		// sending route + ip back to requesting server

									domain.erase(0, 1);
									//sites.push_back(domain);
									//ips.push_back(ip_str);

									updateCache(domain, ip);
								}

								else {
									route = requesting_server + " -> S3 (proxy) -> " + requesting_server + " -> ";
									string ip_to_send = "3+" + route + "+" + cache_IP;
									send(fds[i], ip_to_send.c_str(), ip_to_send.size(), 0);
								}
							}

							// server requesting routing table
							if (flag2 == '5')
							{
								string rtable = getRoutingTable(s1_str, s2_str, s4_str);
								string rt = "0";
								rt.append(rtable);
								send(fds[i], rt.c_str(), rt.size(), 0);

							}
						}
					}
				}
			}
		}
	}
}
