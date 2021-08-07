#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/time.h>
#include <fstream>
#include <iostream>

using namespace std;

#define MAXFD 10	//Size of fds array

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

string ip(string domain)
{
    string word;
    string found;       //If the website is found in the DNS Server
    string notfound = "Website not found";      //Message to be displayed if not found
    
    fstream obj("dns.txt", ios::in);
    
    string temp = "www." + domain;          //Adding "www." at start of the domain name
    temp += ".com";                         //Adding ".com" at end of the domain name

    while (!obj.eof())
    {
        obj >> word;                        //Reading word by word

        if (word == temp)
        {
            obj >> found;
            return found;
        }
    }
    obj.close();
    return notfound;
}

int main()
{
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd!=-1);

    string s_name = "DNS";
    int port = 20000;

	struct sockaddr_in saddr,caddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(port);
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
    {
	  	fds[i]=-1;
    }
	
	//Add a file descriptor to the fds array
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

		else if(n==0)
		{
			//printf("time out\n");
		}

		else
		{
			for(i=0;i<MAXFD;++i)
			{
				if(fds[i]==-1)
					continue;

				if(FD_ISSET(fds[i],&fdset))
				{
					if(fds[i]==sockfd)	// a new client requests a connection
					{
						//accept
						struct sockaddr_in caddr;
						int len=sizeof(caddr);

						int c=accept(sockfd,(struct sockaddr *)&caddr, (socklen_t*)&len);	//Accept new client connections
						if(c<0)
						{
							continue;
						}
					
						fds_add(fds,c);

                        cout << "Connected to server." << endl;
					}
					else   //Receive data recv when an existing client sends data
					{
						char buff[128]={0};
						int res=recv(fds[i],buff,127,0);
						if(res<=0)
						{
							close(fds[i]);
							fds[i]=-1;
						}
						else
						{
							cout << "Domain requested = " << buff << endl;
                            string domain = charPtrToString(buff);

                            memset(buff, 0, sizeof(buff));

                            string ip_addr = ip(domain);
                            cout << "Matching IP = " << ip_addr << endl;

                            send(fds[i], ip_addr.c_str(), ip_addr.size(), 0);           //Sending back the ip of the website
						}
					}
				}
			}
		}
	}
}
