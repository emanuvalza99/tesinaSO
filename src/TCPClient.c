#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_CLIENT 100

void volonta();

pthread_mutex_t done;


int listen_sock;

void *recive_mex(void* i){
   
  
   
   char buffer[4096];
   int n_bytes;
   while(1){
      memset(buffer,0,4096);
      sleep(1);
      if((n_bytes=recv(listen_sock,buffer,4096,0))==0){
        printf("collegamento server interrotto\n");
        exit(EXIT_FAILURE);
      }
      buffer[n_bytes]=='\0';
      
      if(strcmp(buffer,"realloc")==0){
         volonta();
         pthread_mutex_unlock(&done);
         continue;
      }
      printf("stringa ricevuta->%s\n",buffer);
      fflush(stdout);
      memset(buffer,0,4096);
   }
}



void *send_mex(void *i){
   
   
 
   char buffer[4096];
   
   while(1){      
      fgets(buffer,4096,stdin);
      if(strcmp(buffer," ")==0 || strcmp(buffer,"\n")==0 || strcmp(buffer,"\0")==0) continue;
      if(strcmp(buffer,"abbandona gruppo\n")==0){
        send(listen_sock,buffer,4096,0);
        pthread_mutex_lock(&done);
        memset(buffer,0,4096);
        continue;
      }
      send(listen_sock,buffer,4096,0);
      memset(buffer,0,strlen(buffer));
   }
}



void volonta(){
   
    char buffer[140];
    char risposta_server[50];
    char *c;
    int v;
    int n_bytes;
    
    int ret;
    char b[40];
    int counter=0;
    
listen:    
       
    memset(buffer,0,140);
    n_bytes=recv(listen_sock,buffer,140,0);
    buffer[n_bytes]='\0';
    printf("%s\n",buffer);
    fflush(stdout);
    
    ret=strcmp(buffer,"operazioni disponibili:\n1)crea nuovo gruppo chat\n2)joinare ad un gruppo esistente\n3)panoramica gruppi esistenti");
    
    if(ret==0){
retry:
      scanf("%ms", &c);
      if(memcmp("1", c, strlen(c)) == 0 || memcmp("2", c, strlen(c)) == 0 || memcmp("3", c, strlen(c)) == 0){
         v = atoi(c);
      }
      else{
         if(memcmp("exit", c, strlen(c)) == 0){
            printf("Arrivederci...\n");
            exit(0);
         }
         printf("Input non valido, inserire 1, 2 o 3\n");
         free(c);
         goto retry;
      }
      
      
      
      
      
      
      send(listen_sock,c,strlen(c),0);
      if(v==1){

name_exist:       
       
       memset(buffer,0,110);
       sleep(1);
       printf("inserire nome del nuovo gruppo\n");
       fflush(stdout);
       scanf("%s",buffer);
       send(listen_sock,buffer,strlen(buffer),0);
       
       n_bytes=recv(listen_sock,risposta_server,110,0);
       risposta_server[n_bytes]='\0';
       printf("%s\n",risposta_server);
       if(strcmp(risposta_server,"gruppo esistente")==0){
         //printf("gruppo già esistente,riprova\n");
         fflush(stdout);
         goto name_exist;
       }
       
       memset(risposta_server,50,0);
       return;
      }
      if(v==2){
        
try_again:         
         
         
         n_bytes=recv(listen_sock,risposta_server,5,0);
         risposta_server[n_bytes]='\0';
         printf("%s\n",risposta_server);
         if(strcmp(risposta_server,"EMPTY")==0){
           printf("non c'è alcun gruppo esistente\n");
           goto listen;
         }
         
         memset(buffer,110,0);
         printf("inserire nome del gruppo a cui unirsi\n");
         scanf("%20s",buffer);
         send(listen_sock,buffer,strlen(buffer),0);
         
         n_bytes=recv(listen_sock,risposta_server,50,0);
         risposta_server[n_bytes]='\0';
         
         int p;
         if(strcmp(risposta_server,"sei stato aggiunto al gruppo")==0){
           printf("%s\n",risposta_server); 
           return;
         } 
       
         if(strcmp(risposta_server,"non esiste un gruppo con questo nome")==0){
            printf("%s\n",risposta_server);
            goto try_again;
         }
      }  
      
      if(v==3){ 
        counter=0;
        while(1){
           
           n_bytes=recv(listen_sock,b,50,0);
           /*if(strcmp(b,"\0")==0){
             continue;
           }*/ 
           b[n_bytes]='\0';
           if(strcmp(b,"zero")==0){
             printf("non ci sono gruppi esitenti\n");
             break;
           }
           if(strcmp(b,"end")==0) break;
           
           printf("gruppo %d-esimo->%s\n",counter,b);
           fflush(stdout);
           counter++;
           
           memset(b,strlen(b),0);
         }
         memset(b,strlen(b),0);
         
         goto listen;
    
      }
     
   }
   else{
       memset(buffer,100,0);
       printf("inserire nome nuovo gruppo\n");
       scanf("%s",buffer);
       send(listen_sock,buffer,strlen(buffer),0);
       return;
    }
}
     
    




int main(int argc,char **argv){
   
    int connect_sock,ret;
    struct sockaddr_in cliADDR,servADDR;
    struct hostent *h;
    
    if(argc<2){
       printf("inserire indirizzo ip del server\n");
       exit(0);  
    }
   
    h=gethostbyname(argv[1]);
    if(h==(void*)-1){
      printf("error ip server\n");
      exit(0);
    }
    
    servADDR.sin_family=h->h_addrtype;
    servADDR.sin_port=htons(7000);
    servADDR.sin_addr = *((struct in_addr *)h->h_addr_list[0]);
    
    listen_sock=socket(AF_INET,SOCK_STREAM,0);
    if(listen==(void*)-1){
       printf("impossibile allocare socket client\n");
	exit(-1);
    }
    
    cliADDR.sin_family=AF_INET;
    cliADDR.sin_port=htons(0);
    cliADDR.sin_addr.s_addr=INADDR_ANY; 
   
    ret=bind(listen_sock,(struct sockaddr*)&cliADDR,sizeof(cliADDR));
    if(ret==-1){
      printf("error bind socket\n");
      exit(-1);
    }
    int size=sizeof(struct sockaddr_in);
    ret=connect(listen_sock,(struct sockaddr*)&servADDR,size);
    if(ret==-1){
       printf("error connection\n");
       exit(-1);
    }
    printf("connessione stabilita %s\n",inet_ntoa(servADDR.sin_addr));
    pthread_t tid;
    
    pthread_mutex_init(&done,NULL);
    pthread_mutex_lock(&done);
    
    volonta();
    
    pthread_create(&tid,NULL,recive_mex, NULL);
    pthread_create(&tid,NULL,send_mex, NULL);
    
    syscall(60,0);
}

         
    
