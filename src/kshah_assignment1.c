/**
 * @pranavmo_assignment1
 * @author  Pranav Moreshwar Sorte <pranavmo@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
// #include <arpa/inet.h>

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 256
#define UDP_PORT 53;
#define MSG_SIZE 256

#include "../include/global.h"
#include "../include/logger.h"

char *ub_it_name = "pranavmo";
char str_ip[INET_ADDRSTRLEN];
int login_status_client = 0;
typedef struct list_clients
{
	int id;
	char hostname[BUFFER_SIZE];
	char IPaddress[CMD_SIZE];
	int port_num;
	int log_status;
	int num_sent;
	int num_recv;
	int is_block;
	char blocked_by[100];
	struct list_clients *next;
} list_clients;

typedef struct list_peers
{
	char hostname[BUFFER_SIZE];
	char IPaddress[CMD_SIZE];
	int port;
	int is_block;
	struct list_peers *next;
	
} list_peers;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */


int startServer(int port_no);
int startClient(int port_no);

void printStatement();
void displayIP();
void listClients(list_clients** client_list);
int validate_ip(char *ip);
int valid_portnum(char* port);
void add_client_list(char buffer[256], list_peers** head);
void add_server_list(char hosts[200], int client_listen_port, int s, list_clients** head);
int connect_to_host(char* ip_server, int client_port, int server_port);
int validate_number(char *str);
int connect_to_host(char *ip_server, int client_port, int server_port);
void send_server_list(int socket, list_clients** list);
void free_client_list(list_peers** head);
void send_msg_to_client(list_clients** list, char ipaddr[256], char message_to_client[256]);
int check_client_list(char* IPaddr, list_peers** head);
int is_blocked(char *ipaddr, list_peers** head);
void change_block_status(int stat, char* ipaddr, list_peers** head);
int check_server_list(char* ipaddr, list_clients** head);
void list_blocked_clients(char IPaddr[100], list_clients** head);

// void sendToIP(char ipaddress[256], list_clients** list, char messageToSend[256]);

int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	if(argc != 3){
		printf("Only 2 arguments accepted.\n");
		exit(-1);
	}

	if(strcmp(argv[1], "s")==0){
		int port_no = atoi(argv[2]);
		// printf("Hi");
		//Server function
		startServer(port_no);

	}
	else{
		int port_no = atoi(argv[2]);
		// printf("Client Hi");
		//Client function
		startClient(port_no);
	}

	return 0;
}

int startServer(int port_no){
	// printf("Hello From Server\n");
	int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
	struct sockaddr_in client_addr, server_addr;
	
	fd_set master_list, watch_list;
	list_clients *server_list = NULL;	
	
	/* Socket */
	server_socket = socket(AF_INET,SOCK_STREAM,0);
	if(server_socket < 0)
		perror("Cannot create socket");
	
	
	/*Filling up the sockaddr_in*/
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port_no);

	/* Bind */
	if(bind(server_socket, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0 )
		perror("Bind failed");

	
	
	/* Listen */
	if(listen(server_socket, BACKLOG) < 0)
		perror("Unable to listen on port");
	
	/* ---------------------------------------------------------------------------- */
	
	/* Zero select FD sets */
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);
	
	/* Register the listening socket */
	FD_SET(server_socket, &master_list);
	/* Register STDIN */
	FD_SET(STDIN, &master_list);
	
	head_socket = server_socket;
	
	while(TRUE){
		memcpy(&watch_list, &master_list, sizeof(master_list));
		
		//printf("\n[PA1-Server@CSE489/589]$ ");
		//fflush(stdout);
		
		/* select() system call. This will BLOCK */
		selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
		if(selret < 0)
			perror("select failed.");
		
		/* Check if we have sockets/STDIN to process */
		if(selret > 0){
			/* Loop through socket descriptors to check which ones are ready */
			for(sock_index=0; sock_index<=head_socket; sock_index+=1){
				
				if(FD_ISSET(sock_index, &watch_list)){
					
					/* Check if new command on STDIN */
					if (sock_index == STDIN){
						char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);
						
						memset(cmd, '\0', CMD_SIZE);
						if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);
						
						int size_of_cmd = strlen(cmd);
						if(cmd[size_of_cmd - 1] == '\n'){
							cmd[size_of_cmd - 1] = '\0';
						}
						
						// printf("HElllllo----> This is cmd passed-----%s",cmd);
						// printf("This isss----> %d",sizeof(cmd));
						//https://www.educative.io/edpresso/splitting-a-string-using-strtok-in-c

						int i =0;
						char *token = strtok(cmd, " ");
						char *argvs[3];


						while(token){
							argvs[i] = malloc(strlen(token)+1);
							strcpy(argvs[i],token);
							i += 1;
							token = strtok(NULL," ");
						}

						// printf("First arguement %d",*argvs[0]);
						// printf("Argv %d",sizeof(argvs[0]));
						// printf("Length of LIST %d", strlen(cmd));
						
						//Process PA1 commands here ...
						if(strcmp(argvs[0],"AUTHOR")==0){
							cse4589_print_and_log("[%s:SUCCESS]\n","AUTHOR");
							cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ub_it_name);
							cse4589_print_and_log("[%s:END]\n","AUTHOR");
						}

						else if(strcmp(argvs[0],"PORT")==0) {
							cse4589_print_and_log("[%s:SUCCESS]\n","PORT");
							cse4589_print_and_log("PORT:%d\n",port_no);
							cse4589_print_and_log("[%s:END]\n","PORT");
						}
						else if(strcmp(argvs[0],"IP")==0){
							
							
							// cse4589_print_and_log("PORT:%d\n",port_no);
							displayIP();
							
						}
						else if(strcmp(argvs[0], "LIST") == 0){
							cse4589_print_and_log("[%s:SUCCESS]\n","LIST");
							listClients(&server_list);
							cse4589_print_and_log("[%s:END]\n","LIST");
						}

						else if(strcmp(argvs[0], "BLOCKED")==0) {
							char *ip = (char*) malloc(sizeof(char)*strlen(argvs[1]));
							strncpy(ip, argvs[1], strlen(argvs[1]));

							if(validate_ip(ip) && check_server_list(argvs[1], &server_list)){
								cse4589_print_and_log("[%s:SUCCESS]\n", "BLOCKED");
								// displayBlockedList(argv[1], &myServerList);
								list_blocked_clients(argvs[1],&server_list);
								cse4589_print_and_log("[%s:END]\n", "BLOCKED");
							}
							else {
								cse4589_print_and_log("[%s:ERROR]\n", "BLOCKED");
								cse4589_print_and_log("[%s:END]\n", "BLOCKED");
							}
						}
						

						
						free(cmd);
					}
					/* Check if new client is requesting connection */
					else if(sock_index == server_socket){
						caddr_len = sizeof(client_addr);
						fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
						// printf("FD Accept--->%d",fdaccept);
						if(fdaccept < 0)
							perror("Accept failed.");
						
						printf("\nRemote Host connected!\n");      

						char hosts[1024];
						char serv[20];

						getnameinfo(&client_addr, sizeof client_addr, hosts, sizeof hosts, serv, sizeof serv, 0);
						char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);

						if(recv(fdaccept,buffer,BUFFER_SIZE,0)>=0){
							// printf("Client on listen port %s \n", buffer);

						}   

						int client_listening_port = atoi(buffer);
						add_server_list(hosts, client_listening_port, fdaccept, &server_list);      
						send_server_list(fdaccept, &server_list);         
						
						/* Add to watched socket list */
						FD_SET(fdaccept, &master_list);
						if(fdaccept > head_socket) head_socket = fdaccept;
						
					}
					/* Read from existing clients */
					else{
						/* Initialize buffer to receieve response */
						char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);
						
						if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
							close(sock_index);
							printf("Remote Host terminated connection!\n");
							
							/* Remove from watched list */
							FD_CLR(sock_index, &master_list);
						}
						else {
							//Process incoming data from existing clients here ...
							
							// printf("\nClient sent me: %s\n", buffer);
							

							int is_client_blocked = 0;
                        	char ip_addr_client[50];
                        	memset(ip_addr_client, 0, sizeof ip_addr_client);
                        	list_clients *temp = server_list;

							while(temp!=NULL) {
								if(temp->id == sock_index){
									strncpy(ip_addr_client, temp->IPaddress,sizeof(temp->IPaddress));
									is_client_blocked = temp->is_block;

								}
								temp=temp->next;
							}
							free(temp);

							char* msg_to_send = (char*) calloc(strlen(buffer) + 1, sizeof(char));;
							strncpy(msg_to_send,buffer,strlen(buffer));
							int i =0;
							char *token = strtok(msg_to_send, " ");
							char *argv_client[500];


							while(token){
								argv_client[i] = malloc(strlen(token)+1);
								strcpy(argv_client[i],token);
								i += 1;
								token = strtok(NULL," ");
							}

							if(strcmp(argv_client[0],"REFRESH")==0){
								// printf("Heelo from refresh in server:\n");
								list_clients *temp = server_list;
								while(temp != NULL){
									// printf("Hello inside while loop\n");
									if(temp->id == sock_index){
										send_server_list(sock_index, &server_list);
										break;
									}
									temp = temp->next;
								}

							}

							else if(strcmp(argv_client[0],"SEND")==0){
								char message_from_client[256];
								memset(message_from_client, 0, sizeof message_from_client);

								if(is_client_blocked == 0)
								{
									
									printf("Message coming from client: %s\n", message_from_client);
									char *first_token, *second_token, *remaining_part, *context;
									char delimiter[] = " ";
									// int inputLength = strlen(input);
									// char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
									// strncpy(inputCopy, input, inputLength);

									first_token = strtok_r (buffer, delimiter, &context);
									second_token = strtok_r (NULL, delimiter, &context);
									remaining_part = context;
									strcat(message_from_client, "RCV ");
									strcat(message_from_client, ip_addr_client);
									strcat(message_from_client, " ");
									strcat(message_from_client, remaining_part);
									cse4589_print_and_log("[RELAYED:SUCCESS]\n");
									cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", ip_addr_client, second_token, remaining_part);
									cse4589_print_and_log("[RELAYED:END]\n");
									// sendToIP(second_token, &server_list, message_from_client);
									send_msg_to_client(&server_list,second_token,message_from_client);
								}


							}

							else if(strcmp(argv_client[0],"BROADCAST")==0){
								char message_from_client[256];
								memset(message_from_client, 0, sizeof message_from_client);

								char *first_token, *remaining_part, *context;
								char delimiter[] = " ";
								// int inputLength = strlen(input);
								// char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
								// strncpy(inputCopy, input, inputLength);

								first_token = strtok_r (buffer, delimiter, &context);
								// second_token = strtok_r (NULL, delimiter, &context);
								remaining_part = context;
								strcat(message_from_client, "RCV ");
								strcat(message_from_client, ip_addr_client);
								strcat(message_from_client, " ");
								strcat(message_from_client, remaining_part);

								for(int i = 0;i<=head_socket;i++){
									if(FD_ISSET(i, &master_list)){
										if(i!=0 && i!=server_socket && i!=sock_index){
											printf("%d",i);
											if(send(i,message_from_client,strlen(message_from_client),0) == strlen(message_from_client)){
												printf("Done!\n");
												cse4589_print_and_log("[RELAYED:SUCCESS]\n");
												cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", ip_addr_client, "255.255.255.255", remaining_part);
												cse4589_print_and_log("[RELAYED:END]\n");
											}
											fflush(stdout);
										}
									}
								}
							}

							else if (strcmp(argv_client[0],"BLOCK")==0) {
								char *first_token, *remaining_part, *context;
								char delimiter[] = " ";
								first_token = strtok_r (buffer, delimiter, &context);
								remaining_part = context;
								list_clients *temp1 = server_list;
								while(temp1 != NULL){
									if(strcmp(temp1->IPaddress,remaining_part)==0){
										strcpy(temp1->blocked_by, ip_addr_client);
										temp1->is_block = 1;
									}
									temp1 = temp1->next;
								}
							}

							else if (strcmp(argv_client[0],"UNBLOCK")==0) {
								char *first_token, *remaining_part, *context;
								char delimiter[] = " ";
								first_token = strtok_r (buffer, delimiter, &context);
								remaining_part = context;
								list_clients *temp1 = server_list;
								while(temp1 != NULL){
									if(strcmp(temp1->IPaddress,remaining_part)==0){
										// strcpy(temp1->blocked_by, ip_addr_client);
										temp1->is_block = 0;
									}
									temp1 = temp1->next;
								}
							}





						}
						
						free(buffer);
					}
				}
			}
		}
	}
}

int startClient(int port_no){
	// printf("Hello From client\n");
	int server_flag;
	list_peers *mylist_clients = NULL;
	int server;

	int head_socket, selret, sock_index, fdaccept=0, caddr_len;
	fd_set master_list, watch_list;

	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);
	FD_SET(STDIN, &master_list);
	
	head_socket = STDIN;

	while(TRUE){
		memcpy(&watch_list, &master_list, sizeof(master_list));
		
		//printf("\n[PA1-Server@CSE489/589]$ ");
		//fflush(stdout);
		
		/* select() system call. This will BLOCK */
		selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
		if(selret < 0)
			perror("select failed.");

		
		/* Check if we have sockets/STDIN to process */
		if(selret > 0){
			/* Loop through socket descriptors to check which ones are ready */
			for(sock_index=0; sock_index<=head_socket; sock_index+=1){
				
				if(FD_ISSET(sock_index, &watch_list)){
					
					/* Check if new command on STDIN */
					if (sock_index == STDIN){
						char *cmd = (char*) malloc(sizeof(char)*MSG_SIZE);
						
						memset(cmd, '\0', MSG_SIZE);
						if(fgets(cmd, MSG_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);
						
						int size_of_cmd = strlen(cmd);
						if(cmd[size_of_cmd - 1] == '\n'){
							cmd[size_of_cmd - 1] = '\0';
						}
						printf("I got: %s (size:%d chars) \n", cmd, strlen(cmd));
						char *message_to_client = (char*) malloc(sizeof(char)*MSG_SIZE);
						memset(message_to_client, '\0', MSG_SIZE);
						strncpy(message_to_client, cmd, strlen(cmd));
						// printf("message at client side: %s \n", message_to_client);
						// printf("HElllllo----> This is cmd passed-----%s",cmd);
						if(strlen(message_to_client)==0){
							continue;
						}
						//https://www.educative.io/edpresso/splitting-a-string-using-strtok-in-c
						int i =0;
						char *token = strtok(cmd, " ");
						char *argvs[1000];
						memset(argvs,0,sizeof(argvs));


						while(token){
							argvs[i] = malloc(strlen(token)+1);
							strcpy(argvs[i],token);
							i += 1;
							token = strtok(NULL," ");
						}
						// int i =0;
						// char *token = strtok(cmd, " ");
						// char *argvs[1000];

						// while(token!=NULL){
						// 	argvs[i++] = token;
						// 	token = strtok(NULL, " ");
						// }

						// printf("First arguement %d",*argvs[0]);

						
						//Process PA1 commands here ...
						if(strcmp(argvs[0],"AUTHOR")==0){
							cse4589_print_and_log("[%s:SUCCESS]\n","AUTHOR");
							cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ub_it_name);
							cse4589_print_and_log("[%s:END]\n","AUTHOR");
						}

						else if(strcmp(argvs[0],"PORT")==0) {
							cse4589_print_and_log("[%s:SUCCESS]\n","PORT");
							cse4589_print_and_log("PORT:%d\n",port_no);
							cse4589_print_and_log("[%s:END]\n","PORT");
						}
						else if(strcmp(argvs[0],"IP")==0){
							
							
							// cse4589_print_and_log("PORT:%d\n",port_no);
							displayIP();
							
						}
						else if(strcmp(argvs[0], "LIST") == 0){
							cse4589_print_and_log("[%s:SUCCESS]\n","LIST");
							list_peers *temp = mylist_clients;
							int client_id = 1;
							while(temp != NULL){
								cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", client_id, temp->hostname, temp->IPaddress, temp->port);
								temp = temp->next;
								client_id = client_id + 1;
			
							}
							cse4589_print_and_log("[%s:END]\n","LIST");
						}
						else if(strcmp(argvs[0], "LOGIN") == 0){
							cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
							cse4589_print_and_log("[%s:END]\n", "LOGIN");
							char *ip = (char*) malloc(sizeof(char)*strlen(argvs[1]));
							strncpy(ip, argvs[1], strlen(argvs[1]));
							int isValid = valid_portnum(argvs[2]);
							printf("Validity of port--->%d",isValid);
							// printf("%s",argvs[2]);
							//Validate IP address --> https://www.tutorialspoint.com/c-program-to-validate-an-ip-address
							if(validate_ip(ip) && isValid == 1){
								
								// printf("Helllo inside LOGIN ------\n");
								server = connect_to_host(argvs[1], port_no, atoi(argvs[2]));
								char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
								memset(buffer, '\0', BUFFER_SIZE);
								mylist_clients == NULL;
								// int tp = recv(server,buffer,BUFFER_SIZE,0);
								// printf("TP---->\n",tp);
								if(recv(server,buffer,BUFFER_SIZE,0) >= 0){
									// printf("Helllo from here-----%s",buffer);
									// printf("Server responded:\n %s", buffer);
									login_status_client = 1;
									add_client_list(buffer, &mylist_clients);
									fflush(stdout);
								}
								FD_SET(server, &master_list);
								if(server > head_socket){
									head_socket = server;
								}

							}
							else {
								cse4589_print_and_log("[%s:ERROR]\n","LOGIN");
								cse4589_print_and_log("[%s:ERROR]\n","LOGIN");
							}
						}
						else if(strcmp(argvs[0], "REFRESH") == 0){
							

							if(send(server, message_to_client,strlen(message_to_client),0)==strlen(message_to_client)){
								// printf("Done -- at client\n");
							}
							char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
							memset(buffer, '\0', BUFFER_SIZE);

							if(recv(server, buffer, BUFFER_SIZE,0)>=0){
								// printf("Response from server---->%s",buffer);
								free_client_list(&mylist_clients);
								if(mylist_clients != NULL){
									// printf("The list contains clients\n");
					
								}
								add_client_list(buffer, &mylist_clients);
								fflush(stdout);

							}

						}

						else if(strcmp(argvs[0], "SEND")== 0){
							char *ip = (char*) malloc(sizeof(char)*strlen(argvs[1]));
							strncpy(ip, argvs[1], strlen(argvs[1]));

							if(validate_ip(ip) && check_client_list(argvs[1], &mylist_clients)){
								if(send(server, message_to_client, strlen(message_to_client), 0) == strlen(message_to_client)){
									// printf("Done in send\n");
								}
								cse4589_print_and_log("[%s:SUCCESS]\n", "SEND");
								cse4589_print_and_log("[%s:END]\n", "SEND");
							}
							else {
								cse4589_print_and_log("[%s:ERROR]\n", "SEND");
								cse4589_print_and_log("[%s:END]\n", "SEND");
							}
							fflush(stdout);
						}

						else if(strcmp(argvs[0],"BROADCAST")==0){
							if(send(server,message_to_client,strlen(message_to_client),0) == strlen(message_to_client)){

							}

							fflush(stdout);
						}

						else if(strcmp(argvs[0],"BLOCK")==0){
							char *ip = (char*) malloc(sizeof(char)*strlen(argvs[1]));
							strncpy(ip, argvs[1], strlen(argvs[1]));

							if(validate_ip(ip) && check_client_list(argvs[1], &mylist_clients) && is_blocked(argvs[1], &mylist_clients)==0){
								if(send(server, message_to_client, strlen(message_to_client), 0) == strlen(message_to_client)){
												
								}

								change_block_status(1, argvs[1], &mylist_clients);
								cse4589_print_and_log("[%s:SUCCESS]\n", "BLOCK");
								cse4589_print_and_log("[%s:END]\n", "BLOCK");
							}
							else
							{
								cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
								cse4589_print_and_log("[%s:END]\n", "BLOCK");
						    }
						}

						else if(strcmp(argvs[0],"UNBLOCK")==0){
							char *ip = (char*) malloc(sizeof(char)*strlen(argvs[1]));
							strncpy(ip, argvs[1], strlen(argvs[1]));

							if(validate_ip(ip) && check_client_list(argvs[1], &mylist_clients) && is_blocked(argvs[1], &mylist_clients)==1){
								if(send(server, message_to_client, strlen(message_to_client), 0) == strlen(message_to_client)){
												
								}

								change_block_status(0, argvs[1], &mylist_clients);
								cse4589_print_and_log("[%s:SUCCESS]\n", "UNBLOCK");
								cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
							}
							else
							{
								cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
								cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
						    }
						}

						else if(strcmp(argvs[0],"LOGOUT")==0){
							cse4589_print_and_log("[%s:SUCCESS]\n","LOGOUT");
                            server = close(server);
                            cse4589_print_and_log("[%s:END]\n","LOGOUT");
                            return 0;
						}

						else if(strcmp(argvs[0],"EXIT")==0){
							cse4589_print_and_log("[%s:SUCCESS]\n","EXIT");
                            close(server);
                            cse4589_print_and_log("[%s:END]\n","EXIT");
                            return 0;
						}
						

						
						
					}
					/* Check if new client is requesting connection */
					/* Read from existing clients */
					else{
						/* Initialize buffer to receieve response */
						char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);
						
						if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
							close(sock_index);
							printf("Remote Host terminated connection!\n");
							
							/* Remove from watched list */
							FD_CLR(sock_index, &master_list);
						}
						else {
							//Process incoming data from existing clients here ...
							// printf("\nServer sent me: %s\n", buffer);

							//DIvided sentence into first two words --https://stackoverflow.com/questions/1556616/how-to-extract-words-from-a-sentence-efficiently-in-c

							char *first_token, *second_token, *remaining_part, *context;
							char delimiter[] = " ";
							// int inputLength = strlen(input);
							// char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
							// strncpy(inputCopy, input, inputLength);

							first_token = strtok_r (buffer, delimiter, &context);
							second_token = strtok_r (NULL, delimiter, &context);
							remaining_part = context;

							// printf("%s\n", first_token);
							// printf("%s\n", second_token);
							// printf("%s\n", remaining_part);

							cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
							cse4589_print_and_log("msg from:%s\n[msg]:%s\n", second_token, remaining_part);
							cse4589_print_and_log("[RECEIVED:END]\n");

							
						}
						
						free(buffer);
					}
				}
			}
		}
	}




}


void printStatement(){
	cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ub_it_name);
}


void displayIP(){

	//https://ubmnc.wordpress.com/2010/09/22/on-getting-the-ip-name-of-a-machine-for-chatty/
	
	struct sockaddr_in temp_udp;
	int len = sizeof(temp_udp);
	int error_flag = 0;
	int socketfd;
	char ip_str[INET_ADDRSTRLEN];
	socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// printf("Hello I am above!\n");
	if(socketfd < 0){
		printf("Error while creating socket\n");
		exit(-1);
	}

	memset((char*) &temp_udp,0,sizeof(temp_udp));
	temp_udp.sin_family = AF_INET;
	temp_udp.sin_port = htons(2000);
	inet_pton(AF_INET, "8.8.8.8", &temp_udp.sin_addr);
	// temp_udp.sin_addr.s_addr = "8.8.8.8";

	int flag = connect(socketfd,(struct sockaddr*)&temp_udp,sizeof(temp_udp));
	// printf("%d\n",flag);

	int flag2 = getsockname(socketfd, (struct sockaddr *) &temp_udp, (unsigned int*) &len);
	inet_ntop(AF_INET,&(temp_udp.sin_addr),ip_str,sizeof(temp_udp));
	if(flag2 < 0){
		error_flag = 1;
	}

	if(error_flag == 0){
		cse4589_print_and_log("[%s:SUCCESS]\n", "IP");
		cse4589_print_and_log("IP:%s\n",ip_str);
		cse4589_print_and_log("[%s:END]\n", "IP");
	}
	else {
		cse4589_print_and_log("[%s:ERROR]\n","IP");
		cse4589_print_and_log("[%s:END]\n","IP");
	}

	// char host[256];
	// char *IP;
	// struct hostent *host_entry;
	// int hostname;

	// hostname = gethostname(host,sizeof(host));
	// IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
	// printf("Host IP: %s", IP);

}



void listClients(list_clients** client_list){
	list_clients* temp = *client_list;
	int id = 1;
	while(temp != NULL){
		// cse4589_print_and_log("Hi");
		cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", id, temp->hostname, temp->IPaddress, temp->port_num);
		id = id+1;
		temp = temp->next;
	}
}

int validate_ip(char *ip) { //check whether the IP is valid or not
   int i, num, dots = 0;
   char *ptr;
   if (ip == NULL)
      return 0;
      ptr = strtok(ip, "."); //cut the string using dor delimiter
      if (ptr == NULL)
         return 0;
   while (ptr) {
      if (!validate_number(ptr)) //check whether the sub string is
         //holding only number or not
         return 0;
         num = atoi(ptr); //convert substring to number
         if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, "."); //cut the next part of the string
            if (ptr != NULL)
               dots++; //increase the dot count
         } else
            return 0;
    }
    if (dots != 3) //if the number of dots are not 3, return false
       return 0;
      return 1;
}

int validate_number(char *str) {
   while (*str) {
      if(!isdigit(*str)){ //if the character is not a number, return
         //false
         return 0;
      }
      str++; //point to next character
   }
   return 1;
}

int valid_portnum(char* port){
	if(strlen(port) < 4){
		return 0;
	}
	while (*port) {
        if (isdigit(*port++) == 0) return 0;
    }
    return 1;
}


int connect_to_host(char *ip_server, int client_port, int server_port)
{
    int fdsocket, len;
    struct sockaddr_in remote_server_addr;

    fdsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(fdsocket < 0)
       perror("Failed to create socket");

    bzero(&remote_server_addr, sizeof(remote_server_addr));
    remote_server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_server, &remote_server_addr.sin_addr);
    remote_server_addr.sin_port = htons(server_port);

    if(connect(fdsocket, (struct sockaddr*)&remote_server_addr, sizeof(remote_server_addr)) < 0)
        perror("Connect failed");
    char portNo[100];
    memset(portNo, 0, sizeof(portNo));
    sprintf(portNo, "%d", client_port);
    if(send(fdsocket, portNo, strlen(portNo), 0) == strlen(portNo))
			printf("Done!\n");
	fflush(stdout);
    // cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LOGIN");
    // cse4589_print_and_log((char *)"[%s:END]\n", "LOGIN");

    return fdsocket;
}

void add_client_list(char buffer[256], list_peers** head) {
	// printf("Hello from adding client to list");
	char *line;
	char *token;
	char buf[256];
	// printf("%sBuffer in client---->\n",buffer);
	for (line = strtok (buffer, "+"); line != NULL;
		line = strtok (line + strlen (line) + 1, "+"))
	{
		strncpy (buf, line, sizeof (buf));
		int i = 1;
		list_peers *last = *head;
		list_peers *new = (list_peers*)malloc(sizeof(list_peers));
		
		for (token = strtok (buf, "-"); token != NULL;
		token = strtok (token + strlen (token) + 1, "-"))
		{
			if(i == 1)
			{
				new->port = atoi(token);
				i += 1;
			}
			else if(i == 2)
			{
				memset(new->IPaddress, 0, 100);
				strncpy(new->IPaddress, token, strlen(token));
				i += 1;
			}
			else if(i == 3)
			{
				memset(new->hostname, 0, 256);
				strncpy(new->hostname, token, strlen(token));
				i += 1;
			}
		}
		new->is_block = 0;
		// printf("\nPort is %d\n", new->port);
		// printf("\nIP address %s\n", new->IPaddress);
		// printf("\nHost Is %s\n", new->hostname);
		new->next = NULL;
		if(*head == NULL) 
		{
			*head = new;
		}
		else
		{
			printf("Second insertions\n");
			while(last->next != NULL)
			{
				last = last->next;
			}
			last->next = new;
		}

		// if(*head==NULL){
        
        // *head = new;
        // new->next = NULL;
		// printf("First insertion\n");
    	// }
		// else{
		// 	// printf("oye here!")
		// 	printf("Second insertions\n");
		// 	new->next = *head;
		// 	*head = new;
		// }	
	}	
	// printf("Client added succesfully-----\n");

}

void add_server_list(char hosts[200], int client_listen_port, int s, list_clients** head) {
	// printf("Helllo from adding to server");
	// printf("Hosts ---> %s",hosts);
	socklen_t len;
	struct sockaddr_storage sock_addr;
	char ipstr[INET6_ADDRSTRLEN];
	int port;

	len = sizeof sock_addr;
	getpeername(s, (struct sockaddr*)&sock_addr, &len);

	// deal with both IPv4 and IPv6:
	if (sock_addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&sock_addr;
		//getnameinfo(&sadd, sizeof sadd, host, sizeof host, service, sizeof service, 0);
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	}
	
	list_clients *new_node = (list_clients*)malloc(sizeof(list_clients));
	new_node->id = s;
	strncpy(new_node->IPaddress,ipstr, strlen(ipstr));
	strncpy(new_node->hostname, hosts, strlen(hosts));
	new_node->port_num = client_listen_port;
	new_node->is_block = 0;
	new_node->log_status = 1;
	new_node->next = NULL;
	list_clients *current = *head;
	
	// || (*head)->port_num > new_node->port_num
	// && current->next->port_num < new_node->port_num
	if(*head == NULL || (*head)->port_num > new_node->port_num) 
	{
		new_node->next = *head;
		*head = new_node;
	}
	else
	{
		while(current->next != NULL && current->next->port_num < new_node->port_num )
		{
			current = current->next;
		}
		new_node->next = current->next;
		current->next = new_node;
	}
	// printf("Peer IP address: %s\n", ipstr);
	// printf("Peer Listening port      : %d\n", client_listen_port);
	// printf("Host Name      : %s\n", hosts);
}

void send_server_list(int socket, list_clients** list){
	// printf("Hello I am here in send server list");
	list_clients *temp = *list;
	char buffer_to_send[100];
	char list_to_client[1000];

	memset(buffer_to_send,0,sizeof(buffer_to_send));
	memset(list_to_client,0,sizeof(list_to_client));

	while(temp!=NULL){
		// printf("The port is %d",temp->port_num);

		//https://www.geeksforgeeks.org/sprintf-in-c/
		sprintf(buffer_to_send,"%d-%s-%s+",temp->port_num,temp->IPaddress,temp->hostname);
		strcat(list_to_client,buffer_to_send);
		fflush(stdout);
		temp = temp->next;
	}

	// printf("%s---->",buffer_to_send);

	if(send(socket,list_to_client, strlen(list_to_client), 0) == strlen(list_to_client)){
		printf("Done");
	}
}

void free_client_list(list_peers** head)
{
   /* deref head_ref to get the real head */
   list_peers* current = *head;
   list_peers* next;
 
   while (current != NULL) 
   {
       next = current->next;
       free(current);
       current = next;
   }
   
   
   *head = NULL;
}

void send_msg_to_client(list_clients** list, char ipaddr[256], char message_to_client[256]){
	list_clients *temp = *list;
	while(temp != NULL)
	{
		if(strcmp(temp->IPaddress, ipaddr) == 0)
		{
			if(send(temp->id, message_to_client, strlen(message_to_client), 0) == strlen(message_to_client))
				printf("Done!\n");
			fflush(stdout);
		}
		temp = temp->next;
	}
}

int check_client_list(char* IPaddr, list_peers** head){
	list_peers* temp = *head;
	while(temp != NULL){
		if(strcmp(temp->IPaddress,IPaddr)==0){
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

// void sendToIP(char ipaddress[256], list_clients** list, char messageToSend[256])
// {
// 	list_clients *temp = *list;
// 	while(temp != NULL)
// 	{
// 		if(strcmp(temp->IPaddress, ipaddress) == 0)
// 		{
// 			if(send(temp->id, messageToSend, strlen(messageToSend), 0) == strlen(messageToSend))
// 				printf("Done!\n");
// 			fflush(stdout);
// 		}
// 		temp = temp->next;
// 	}
// }

int is_blocked(char *ipaddr, list_peers** head){
	list_peers *temp = *head;
	while(temp!=NULL){
		if(strcmp(temp->IPaddress,ipaddr)==0){
			if(temp->is_block == 1){
				return 1;
			}
		}
		temp = temp->next;
	}
	return 0;
}

void change_block_status(int stat, char* ipaddr, list_peers** head){
	list_peers* temp = *head;
	while(temp!=NULL){
		if(strcmp(temp->IPaddress, ipaddr)==0){
			temp->is_block = stat;
		}
		temp = temp->next;
	}
}

int check_server_list(char* ipaddr, list_clients** head) {
	list_clients* temp = *head;
	while(temp != NULL){
		if(strcmp(temp->IPaddress,ipaddr)==0){
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

void list_blocked_clients(char IPaddr[100], list_clients** head){
	list_clients *temp = *head;
	int id = 1;
	while(temp!=NULL){
		if(temp->is_block == 1 && strcmp(temp->blocked_by,IPaddr)==0){
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", id, temp->hostname, temp->IPaddress, temp->port_num);

			id = id+1;
		}
		temp = temp->next;
	}
}
