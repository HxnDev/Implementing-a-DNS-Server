#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <iostream>
using namespace std;

char displayMenu()
{
	char option;
	cout<<"\nYou have the following options: "<<endl;
	cout<<"1: Connect with other clients"<<endl;
	cout<<"2: Recieve Message"<<endl;
	cout<<"3: Find a website using DNS"<<endl;
	cout<<"4: Exit" << endl;
	cout<< "Enter your desired choice = ";
	cin>> option;
	return option;
}

string charPtrToString(char* toConvert) 
{
	string str;
	for (int i = 0; toConvert[i]; i++)
		str += toConvert[i];

	return str;
}

int selectServerPort() {
	int option;
	cout << "Server 1: 20001" << endl;
	cout << "Server 2: 20002" << endl;
	cout << "Server 4: 20004" << endl;
	cout << "\nEnter number of selected server: ";
	cin >> option;

	return option;
}

int main()
{
	char choice;
	int sockfd = socket(AF_INET,SOCK_STREAM,0);	
	assert(sockfd != -1 );

	int port = selectServerPort();

	//Set Address Information
	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr("192.168.100.21");

	//Link to server
	int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res != -1);
	
	string clientports = "Client\tPort\n";
	char buff[500] = {0};

	choice = displayMenu();
	buff[0] = choice;

	send(sockfd,buff,strlen(buff),0);		//Sending client's choice to server

	if (choice == '1')			//Client wants to communicate with other clients
	{
		//______________________________________________________________________________
		cout<<"Choice 1 selected"<<endl;
		recv(sockfd, buff, sizeof(buff), 0);			//Recieving list of clients and ports
		string temp = charPtrToString(buff);			
		clientports.append(temp);						//Storing list in string
		cout << temp << endl;
		//______________________________________________________________________________
		cout<<"Enter the client(port number) you want to communicate with = ";
		cin.ignore();
		fgets(buff,128,stdin);

		cout<<"Client to comm with = "<<buff<<endl;

		send(sockfd,buff,strlen(buff),0);				//Sends client's port no.
		while (1)
		{
			memset(buff, 0, sizeof(buff));
			cout << "Enter message: ";
			fgets(buff,sizeof(buff),stdin);
			send(sockfd,buff,strlen(buff),0);
			if (strncmp(buff, "close", 5) == 0)
			{
				break;
			}
			cout<<"Message sent"<<endl;
			memset(buff, 0, sizeof(buff));
			recv(sockfd, buff, sizeof(buff), 0);			//Recieve the message
			cout << "Message recieved = " <<buff<< endl;
			
			if (strncmp(buff, "close", 5) == 0)
			{
				break;
			}
		}
		
	}

	else if (choice == '2')		//Clients want to recieve a message
	{	
		while(1)
		{
			char buff2[200];
			memset(buff2, 0, sizeof(buff2));
			recv(sockfd, buff2, sizeof(buff2), 0);			//Recieve the message

			cout << "Message recieved = " << buff2 << endl;

			if (strncmp(buff2, "close", 5) == 0)
			{
					break;
			}

			memset(buff2, 0, sizeof(buff2));

			cin.ignore();

			cout << "buff2 = " <<  buff2 << endl;

			cout << "Enter message: ";
			fgets(buff2, sizeof(buff2), stdin);
			send(sockfd,buff2,strlen(buff2),0);

			cout << "buff2 = " << buff2 << endl;

			cout << "Message sent" << endl;
			
			if (strncmp(buff2, "close", 5) == 0)
				{
					break;
				}
			}
	}

	else if (choice == '3') 	//Clients wants to search a website using DNS
	{
		cout<<"Choice 3 selected"<<endl;

		string site;
		cout << "Enter the website you are looking for (Donot enter 'wwww.' or '.com') = ";
		cin >> site;

		send(sockfd, site.c_str(), site.size(), 0);			//Sends the domain name to the server

		char info[100];
		recv(sockfd, info, sizeof(info), 0);					//Recieves the ip and route across the domain name.

		string details = charPtrToString(info);
		string c_name, route, ip;

		int pos = details.find('+');
		c_name = details.substr(0, pos);

		details.erase(0, pos + 1);

		pos = details.find('+');
		route = details.substr(0, pos);

		details.erase(0, pos + 1);

		ip = details;
		route = c_name + " -> " + route + " -> " + c_name;

		cout << "\nIP of the website "<< site<< " = "<<ip<<endl<<endl;
		cout<<"\nThe route followed is all follows : "<<endl<<route<<endl<<endl;
	}

	else if (choice == '4') 	//Exit
	{
		cout<<"Choice 4 selected"<<endl;
		cout << "Exiting program";
	}
	
	else
	{
		cout<<"Invalid option selected"<<endl;
		//continue;
	}
	
	close(sockfd);
}
