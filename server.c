/*
	Link de YT:https://www.youtube.com/watch?v=fNerEo6Lstw
*/
// Librerias
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <errno.h>
	#include <string.h>
	#include <pthread.h>
	#include <sys/types.h>
	#include <signal.h>
	#include <stdbool.h>
	
	//Definimos MAXIMA CANTIDAD de clientes y BUFFER
	
	#define MAX_CLIENTS 100
	#define BUFFER_SZ 2048
	
	
	static _Atomic unsigned int client_count = 0;
	static int client_id = 10;
	
	/* Client structure , esto es como que el objeto cliente sus propiedades.*/
	
	typedef struct {
	 struct sockaddr_in address;
	 int sockfd;
	 int uid;
	 char name[32];
	} client_t;
	
	client_t *clients[MAX_CLIENTS];
	
	pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; 
	
	void str_overwrite_stdout() {  
	 printf("\r%s", "> ");
	 fflush(stdout);
	}
	
	void str_trim_lf (char* arr, int length) {
	 int i;
	 
	 for (i = 0; i < length; i++) 
	 {
	 
	 if (arr[i] == '\n') {
	 arr[i] = '\0';
	 break;
	 }
	 }
	}
	
	void print_client_addr(struct sockaddr_in addr){
	 printf("%d.%d.%d.%d",
	 addr.sin_addr.s_addr & 0xff,
	 (addr.sin_addr.s_addr & 0xff00) >> 8,
	 (addr.sin_addr.s_addr & 0xff0000) >> 16,
	 (addr.sin_addr.s_addr & 0xff000000) >> 24);
	}
	
	/* Usuarios disponibles */  // realizamos una funcion para verificar que usuarios se encuentran disponibles en el momento.
	
	void conectados(int sockfd, int uid){   //Solicitamos el socket y el ID del cliente
	
	 pthread_mutex_lock(&clients_mutex);
	
	 char* title = "\nUsuarios disponibles : ";   //Titulo
	 if(write(sockfd, title, strlen(title)) > 0){  
	 
		for(int i=0; i<MAX_CLIENTS; i++){  //Ciclo para recorrer todos los clientes 
		
		if(clients[i] && (clients[i]->uid != uid)){ 
		char name[32];
		strcpy(name, clients[i]->name);
		strcat(name, "\n");
		
		if(write(sockfd, name, strlen(name)) < 0){
		break;
		}
		}
		}
	 }
	 pthread_mutex_unlock(&clients_mutex);
	}
	

		/* ELiminar un usuario repetido */ // Dependiendo lo que devuelva nuestras funciones verificadoras esta funcion entra en juego a remover el usuario repetido de ser el
	void remover(char *s, int uid){
	 pthread_mutex_lock(&clients_mutex);
	
	 for(int i=0; i<MAX_CLIENTS; ++i){
	 if(clients[i]){
	 if(clients[i]->uid == uid){
	 if(write(clients[i]-> sockfd, s, strlen(s)) < 0){
	 break;
	 }
	 }
	 }
	 }
	 pthread_mutex_unlock(&clients_mutex);
	}


	/* Verificar si esta el usuario */ // Verificador para ver si el cliente ya se encuentra , tomamos de parametro el nombre.
	bool verificar(char *name){
	 for(int i=0; i<MAX_CLIENTS; ++i){
	 if(clients[i]){
	 if(strcmp(clients[i]->name, name) == 0){
	 return true;
	 }
	 }
	 }
		return false;
	}
	
	/* Usuarios repetido */  // Luego realizamos otra validacion para ver si el cliente en si ya se encuentra repetido , no solamente el nombre. 
	bool repetido(client_t *cl){
	 for(int i=0; i<MAX_CLIENTS; i++){
	 if(clients[i])
		{
	 if(strcmp(clients[i]->name, cl->name) == 0)
		{
	 if(clients[i]->uid < cl->uid)
		{ 
	 printf("El usuario (%s) ya existe.\n", cl->name);
	 return true;
	 }
	 }
	 }
	 }
		return false;
	}
	
	/* Add clients to queue */
	void queue_add(client_t *cl){
	 pthread_mutex_lock(&clients_mutex);
	
	 for(int i=0; i<MAX_CLIENTS; ++i){
	 if(!clients[i]){
	 clients[i] = cl;
	 break;
	 }
	 }
	 pthread_mutex_unlock(&clients_mutex);
	}
	
	/* Remove clients to queue */
	void queue_remove(int uid){
	 pthread_mutex_lock(&clients_mutex);
	
	 for(int i=0; i<MAX_CLIENTS; ++i){
	 if(clients[i]){
	 if(clients[i]->uid == uid){
	 clients[i] = NULL;
	 break;
	 }
	 }
	 }
	 pthread_mutex_unlock(&clients_mutex);
	}
	
	/* Send message to all clients except sender" */
	void send_message(char *s, int uid){
	 pthread_mutex_lock(&clients_mutex);
	
	 for(int i=0; i<MAX_CLIENTS; ++i){
	 if(clients[i]){
	 if(clients[i]->uid != uid){
	 if(write(clients[i]-> sockfd, s, strlen(s)) < 0){
	 perror("ERROR: write to descriptor failed");
	 break;
	 }
	 }
	 }
	 }
	 pthread_mutex_unlock(&clients_mutex);
	}
	
	/* Informacion de los usuarios*/
	void info_user(char *s, char *name){
	 pthread_mutex_lock(&clients_mutex);
	
	 for(int i=0; i<MAX_CLIENTS; ++i){ //Recorremos todos los clientes
	 
	 if(clients[i]){
	 
	 if(strcmp(clients[i]->name, name) == 0){
	 
	 if(write(clients[i]-> sockfd, s, strlen(s)) < 0){
	 break;
	 }
	 }
	 }
	 }
	 pthread_mutex_unlock(&clients_mutex);
	}

	
	/* Handle all communication with the client */
	void *handle_client(void *arg){
	 char buffer_out[BUFFER_SZ];
	 char buffer_out_copy[BUFFER_SZ];
	 char name[32];
	 char *Ayuda = {
	 
	 /* Menu */
	 "\n\nElige una opcion\n\n<Mensaje> -> Mandar mensajes globales\n\n<Nombre_del_usuario> <Mensaje> ->Mandar mensaje directo a un usuario especifico\n\n<Mostrar-Usuario> -> Imprime usuarios conectados\n\n<Info_user> <Nombre> Informacion del usuario\n\n<Ayuda>\n\n<Salir> Sesion Cerrada\n\n"
	 };
	 int leave_flag = 0;
	 client_count++;
	 client_t *cli = (client_t*)arg;
	
	 /* Colocar usuario */
	 
	 if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1){
	 printf("ERROR: Coloque un usuario\n");
	 leave_flag = 1;
	 } 
		else 
		{
		
	 /*Bienvenido*/
	 
	 strcpy(cli->name, name);
	 
	 /* Verificar si el usuario se encuentra igual a otro usuario */
	 
	 bool verificar = repetido(cli);
	 if(!verificar)
		{
		
		/* Se imprime el ingreso global*/
		
	 sprintf(buffer_out, "%s Se encuentra en el chat\n", cli->name);
	 printf("%s", buffer_out);
	 send_message(buffer_out, cli->uid);
	 } 
		else 
		{
	 sprintf(buffer_out, "El usuario (%s) ya existe.\n", cli->name);
	 remover(buffer_out, cli->uid);
	 leave_flag = 1;
	 }
	 }
	
	 bzero(buffer_out, BUFFER_SZ);
	
		/* Ayuda */
		
	 info_user(Ayuda, cli->name);
	
	 while (1)
	 {
	 if(leave_flag){
	 break;
	 }
	
	 int receive = recv(cli->sockfd, buffer_out, BUFFER_SZ, 0);
	
	 if (receive > 0)
	 {
	 strcpy(buffer_out_copy, buffer_out);
									// CREACION COMANDOS FUNCIONALIDAD CHAT
									
	 char* value_token = strtok(buffer_out_copy, " ");
	 char* show_users_list = "Mostrar-Usuario";   				
	 char* Info_user = "Info_user";
	 value_token = strtok(NULL, " "); 
	
	 str_trim_lf(value_token, strlen(value_token));
	
	 if (strlen(buffer_out) > 0)
	 {
	 
	 if(strcmp(value_token, show_users_list) == 0)
		{
		/* Usuarios disponibles*/
		
	 conectados(cli->sockfd, cli->uid);
	 } 
		
		else if(strcmp(value_token, "") == 0)
		{
	 /* Informacion del usuario */
	 
	 value_token = strtok(NULL, " "); 
	 str_trim_lf(value_token, strlen(value_token));

	 } 
		
		else if(strcmp(value_token, "Salir") == 0)
		{
	 /*Desconectar usuario*/
	 
	 sprintf(buffer_out, "%s ha salido\n", cli->name);
	 printf("%s\n", buffer_out);
	 send_message(buffer_out, cli->uid);
	 leave_flag = 1;
	 } 
		
		else if(strcmp(value_token, "Ayuda") == 0)
		{
	 info_user(Ayuda, cli->name);
	 } 
		
		else 
		{
		
	 /* Confirmar destinatario del mensaje */ 
	 
	 if(verificar(value_token))
		{
	 info_user(buffer_out, value_token);
	 } 
		
		else
		{
	 send_message(buffer_out, cli->uid);
	 str_trim_lf(buffer_out, strlen(buffer_out));
	 printf("%s -> %s\n", buffer_out, cli->name);
	 }
	 }
	 }
	 } 
		
		else if (receive == 0)
		{
	 sprintf(buffer_out, "%s  SALIO \n", cli->name);
	 printf("%s\n", buffer_out);
	 send_message(buffer_out, cli->uid);
	 leave_flag = 1;
	 } 
		
		else 
		{
	 printf("ERROR: -1\n");
	 leave_flag = 1;
	 }
	
	 bzero(buffer_out, BUFFER_SZ);
	
	 }
	
	 close(cli->sockfd);
	 queue_remove(cli->uid);
	 free(cli);
	 client_count--;
	 pthread_detach(pthread_self());
		return NULL;
	
	}
	
	int main(int argc, char **argv){
	 
	 if(argc != 2){
	 printf("Usage: %s <port>\n", argv[0]);
	 return EXIT_FAILURE;
	 }
	
	 char *ip = "18.221.139.84";
	 int port = atoi(argv[1]);
	
	 int option = 1;
	
		int listenfd = 0, connfd = 0;
	 struct sockaddr_in serv_addr;
	 struct sockaddr_in cli_addr;
	
	 pthread_t tid;
	
	 /* Socket settings */
	 listenfd = socket(AF_INET, SOCK_STREAM, 0);
	 serv_addr.sin_family = AF_INET;
	 serv_addr.sin_addr.s_addr = inet_addr(ip);
	 serv_addr.sin_port = htons(port);
	
	 /* Ignore pipe signals */
	 signal(SIGPIPE, SIG_IGN);
	 if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
		{
	 printf("ERROR: setsockopt failed");
	 return EXIT_FAILURE;
	 }
	
	 /* Bind */
	 if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 ){
	 printf("ERROR: bind\n");
	 return EXIT_FAILURE;
	 }
	
	 /* Listen */
	 if (listen(listenfd, 10) < 0) {
	 printf("ERROR: listen\n");
	 return EXIT_FAILURE;
	 }
	
	 printf("Bienvenido\n");
	
	 while(1){
	 socklen_t clilen = sizeof(cli_addr);
	 connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
	
	 /* Check if max clients is reached */
	 if((client_count + 1) == MAX_CLIENTS){  //Verificamos actual cantidad de clientes vs maxima cantidad
	 printf("Hay demasiados usuarios en este momento\n");
	 print_client_addr(cli_addr);
	 close(connfd);
	 continue;
	 }
	
	 /* Client settings */
	 client_t *cli = (client_t *)malloc(sizeof(client_t));
	 cli->address = cli_addr;
	 cli->sockfd = connfd;
	 cli->uid = tid++;
	

	
	 /* Add client to the queue and fork thread */
	 queue_add(cli);
	 pthread_create(&tid, NULL, &handle_client, (void*)cli);
	
	 /* Reduce CPU usage */
	 sleep(1);
	
	 }
	
	 return EXIT_SUCCESS;
	
	}
