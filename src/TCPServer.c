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
#include <sys/mman.h>
#include "chat_group.h"
#include "reallocator.h"

#define MAX_CLIENT 100
#define MAX_GROUP 20



int  num_client=0;
int  num_group=0;
char *shm_addr[MAX_GROUP];

pthread_mutex_t* ready;
pthread_mutex_t  lock;


gruppo *buffer_group[MAX_GROUP]={NULL};

int client[MAX_CLIENT]={0};
int group_client_presence[MAX_CLIENT]={21};

void *send_mex(void *i){
   
   
   char buffer[4096]; 
   int ret;
   unsigned long index=(unsigned long)i;
   int n_send,j;
   
   printf("sono il thread speditore %d-gruppo,index=%d\n",buffer_group[index]->group_id,index);
   fflush(stdout);
   printf("nome_gruppo=%s\n",buffer_group[index]->group_name);
   
   memset(buffer,0,4096);
again:   
   
   if(shm_addr[index]==NULL){
      
      syscall(60,0);
   }
   
   if(pthread_mutex_lock(ready+index)){
       printf("error mutex %d\n",index);
       exit(EXIT_FAILURE);
   }
   memset(buffer,0,strlen(buffer));
   //printf("index=%d\n",index);
   
   memcpy(buffer,shm_addr[index],4096);
   if(strcmp(buffer," ")==0 || strcmp(buffer,"\n")==0 || strlen(buffer)==0) goto again;
   
   j=0;
   if(buffer_group[index]==NULL) syscall(60,0);
   n_send=buffer_group[index]->num_client_connect;
   
   for(j=0;j<n_send;j++){
     ret=send(buffer_group[index]->num_client_group[j],buffer,strlen(buffer),0);
   }
   if(ret==-1){
     printf("invio non eseguito\n");
     syscall(60,0);
   }
   memset(buffer,0,20);
   
   goto again;
}




    


void *recive_mex(void* i){
    
  
   char buffer[4096]; 
   int j=0;
   unsigned long index=(unsigned long)i;
   int presence_group;
   int n_bytes,ret;
   pthread_t tid;
   int r;
   
   printf("sono il thread ricevitore del client %d\n",index);
   fflush(stdout);
again:   
    
    
    
    memset(buffer,4096,0);
    
    if(n_bytes=recv(client[index],buffer,4096,0)==0){
      printf("collegamento interroto con l'%d-esimo client\n",index);
      fflush(stdout);
      buffer_group[group_client_presence[index]]->num_client_connect--;
      
      if(buffer_group[group_client_presence[index]]->num_client_connect==0){
        free(buffer_group[group_client_presence[index]]);                      //deallocazione della struct gruppo iesimo
        num_group--;
        buffer_group[group_client_presence[index]]=NULL;                       //eliminazione della struct del gruppo nell'array 
        munmap(shm_addr[group_client_presence[index]],4096);                   //unmap delle pagine riservate al gruppo essendo stato rimosso
        shm_addr[group_client_presence[index]]=NULL;
      }
      client[index]=0;
      num_client--;
      syscall(60,0);
   }
   
   if(r=strcmp(buffer,"elimina gruppo\n")==0){
      if((buffer_group[group_client_presence[index]]->group_admin)!=(client[index])){
         send(client[index],"non sei l'amministratore del gruppo,eliminazione negata",strlen("non sei l'amministratore del gruppo,eliminazione negata"),0);
          goto again;
         }
       if((buffer_group[group_client_presence[index]]->group_admin==client[index]) && (buffer_group[group_client_presence[index]]->num_client_connect>1)) 
{
        send(client[index],"impossibile eliminare il gruppo ci sono 2 o piu client ancora connessi",strlen("impossibile eliminare il gruppo ci sono 2 o piu client ancora connessi"),0);
        goto again;
       } 
       
       
   } 

   //se l'amministratore e' l'unico appartenente al gruppo ed invia "abbandona gruppo" esso viene eliminato
   
   if((r=strcmp(buffer,"abbandona gruppo\n"))==0){                 

     if(group_client_presence[index]==21){
       send(client[index],"non appartieni a nessun gruppo",strlen("non appartieni a nessun gruppo"),0);
       goto again;
     }
     printf("abbandonato\n");
     
     send(client[index],"realloc",strlen("realloc"),0);
     sleep(1);
     if(buffer_group[group_client_presence[index]]->num_client_connect==1){
        printf("elimino gruppo\n");
        free(buffer_group[group_client_presence[index]]);                      //deallocazione della struct gruppo iesimo
        num_group--;
        buffer_group[group_client_presence[index]]=NULL;                       //eliminazione della struct del gruppo nell'array 
        munmap(shm_addr[group_client_presence[index]],4096);                   //unmap delle pagine riservate al gruppo essendo stato rimosso
        shm_addr[group_client_presence[index]]=NULL;                         
        group_client_presence[index]=21;                                       //21 indica la non presenza del client in nessun gruppo
        
     }
     else{
         presence_group=group_client_presence[index];
         ret=remove_client(client[index],buffer_group[group_client_presence[index]]);
         group_client_presence[index]=21;
     }
     
     
     
     
     ret=reallocated_client(index,buffer_group,client,group_client_presence,&num_group,&num_client,shm_addr);     //funzione che ricolloca il client
     if(ret==-1){
        printf("collegamento interrotto realloc fallita\n");
        fflush(stdout);     
        num_client--;                                                                    //aggiorno dati in funzione della terminazione del collegamento
        client[index]=0;
        group_client_presence[index]=21;
     }
     if(ret==-1) syscall(60,0);
     if(ret==21){
       pthread_create(&tid,NULL,send_mex,(void*)index);                                                       //creazione thread per il nuovo gruppo
       num_group++;  
       goto again;
     }
     if(ret!=21){
       
       group_client_presence[index]=ret;
       
     }
     pthread_create(&tid,NULL,recive_mex,(void*)index);                                          //creazioe del thread per ricevere mex dal client
     
     
     syscall(60,0);
     
   }
   
   
   presence_group=group_client_presence[index];
   
   /*
   if(shm_addr[presence_group]==NULL){ 
      shm_addr[presence_group]=(char*)mmap(NULL,4096,PROT_READ|PROT_WRITE|0660,MAP_SHARED|MAP_ANONYMOUS,0,0);
      printf("mappata %d\n",presence_group);
      if(shm_addr==(void*)-1){
	printf("error page mapping\n");
	exit(-1);
      }
   }
   */
   memset(shm_addr[presence_group],0,strlen(shm_addr[presence_group]));
   
   memcpy(shm_addr[presence_group],buffer,strlen(buffer));                        //lettura messaggio inviato dal client nel gruppo di appartenenza
   
   
   pthread_mutex_unlock(ready+presence_group);                                   //sblocco per leggere il messaggio condiviso nel gruppo
   goto again;
}




void *manage_client(void* z){
             
             unsigned long t=(unsigned long)z;
             unsigned long k;
             long int connect_sock=t;
             char buffer[20];
             int n_bytes;
             int size;
     	     unsigned long index;
             char c[3];
             int i;
             char name_group[20];
             int volonta;
             int esito;
             int group_index;
             int len,ret;
             char p[40];
             
             pthread_t tid;
             gruppo *new_group;
             
             
           
           
try_again:           
           
           send(connect_sock,"operazioni disponibili:\n1)crea nuovo gruppo chat\n2)joinare ad un gruppo esistente\n3)panoramica gruppi esistenti",strlen("operazioni disponibili:\n1)crea nuovo gruppo chat\n2)joinare ad un gruppo esistente\n3)panoramica gruppi esistenti"),0);
           ret=recv(connect_sock,c,3,0);
           
           if(ret==-1 || ret==0){ 
             printf("client disconnesso\n");
             syscall(60,0);
           }
           volonta=atoi(c);
           
            
           if(volonta==1){
                
                
try_again_1:                
                memset(buffer,0,20);
                
                //printf("eccolo\n");
                n_bytes=recv(connect_sock,buffer,20,0);
                buffer[n_bytes]='\0';
                for(i=0;i<num_group;i++){
                  if(strcmp(buffer,buffer_group[i]->group_name)==0){
                    printf("a=%ld\n",send(connect_sock,"gruppo esistente",strlen("gruppo esistente"),0));
                    sleep(1);
                    goto try_again_1;
                  }
                }
                new_group=(gruppo*)malloc(sizeof(gruppo));
                if(new_group==NULL){
                  printf("spazio esaurito\n");
                  fflush(stdout); 
                  send(connect_sock,"spazio server esaurito per allocare nuovo gruppo",strlen("spazio server esaurito per allocare nuovo gruppo"),0);
                  close(connect_sock);
                  syscall(60,0);
                }
                
                
                new_group->num_client_connect=0;
                
                printf("num=%d\n",num_group);
                fflush(stdout);
                create_new_group(connect_sock,buffer,new_group,num_group);
                pthread_mutex_lock(&lock);
                for(i=0;i<MAX_GROUP;i++){
                   if(buffer_group[i]==NULL){
                     buffer_group[i]=new_group;
                     
                     shm_addr[i]=(char*)mmap(NULL,4096,PROT_READ|PROT_WRITE|0660,MAP_SHARED|MAP_ANONYMOUS,0,0);
                     if(shm_addr[num_group]==(void*)-1){
                       printf("error shm\n");
                       exit(EXIT_FAILURE);
                     }
                     //printf("%ld\n",send(connect_sock,"gruppo creato con successo",strlen("gruppo creato con successo"),0));
                     break;
                   }
                }
                pthread_mutex_unlock(&lock);
                if(i==MAX_GROUP){
                  send(connect_sock,"numero massimo gruppi raggiunti",strlen("numero massimo gruppi raggiunti"),0);
                  goto try_again_1;
                }
                group_index=i;
                k=(unsigned long)group_index;
                ret=pthread_create(&tid,NULL,send_mex,(void*)k);
                if(ret==-1){
                  printf("impossibile generare nuovo thread\n");
                  exit(EXIT_FAILURE);
                }
                num_group++;
                
            }
            if(volonta==2){
                
                memset(buffer,20,0);
                
                
try_again_2:
                if(num_group==0){
                  n_bytes=send(connect_sock,"EMPTY",strlen("EMPTY"),0);
                  goto try_again;
                }
                send(connect_sock,"OK",strlen("OK"),0);
                n_bytes=recv(connect_sock,buffer,20,0);
                buffer[n_bytes]='\0';
               
                
                for(i=0;i<num_group;i++){
                  printf("gruppo-%d-esimo->%s\n",i,buffer_group[i]->group_name);
                  if(buffer_group[i]==NULL) continue;
                  if(strcmp(buffer,buffer_group[i]->group_name)==0){
                     esito=1;
                     pthread_mutex_lock(&lock);
                     add_new_client(connect_sock,buffer_group[i]);
                     n_bytes=send(connect_sock,"sei stato aggiunto al gruppo",strlen("sei stato aggiunto al gruppo"),0);
                     pthread_mutex_unlock(&lock);
                     esito=1;
                     break;
                  }   
                }          
                  if(esito!=1){
                   
                    send(connect_sock,"non esiste un gruppo con questo nome",strlen("non esiste un gruppo con questo nome"),0); 
                    memset(buffer,20,0);
                    goto try_again_2;
                  }
                 
             }
             
             if(volonta==3){
               
               memset(p,0,40);
               
               if(num_group==0){
                 send(connect_sock,"zero",strlen("zero"),0);
                 
                 sleep(1);
                 goto try_again;
               }
               for(i=0;i<num_group;i++){
                  
                  len=strlen(buffer_group[i]->group_name);
                  long int byte;
                  memcpy(p,buffer_group[i]->group_name,strlen(buffer_group[i]->group_name));
                  p[len]='\0';
                  
                  sleep(1);
                  byte=send(connect_sock,p,strlen(p),0);
                  
                  if(i+1<num_group && (strlen(buffer_group[i]->group_name)<len)){
                     memset(p,0,40);
                  }
                  //printf("stringa inviata %s b=%ld\n",p,byte);
               }
               sleep(1);
               send(connect_sock,"end",strlen("end"),0); 
               sleep(1);
               goto try_again;
              }
              
           
           pthread_mutex_lock(&lock);
           index=(unsigned long)(num_client);
           client[num_client]=connect_sock;
           
           if(volonta==2)group_client_presence[num_client]=i;
           if(volonta==1)group_client_presence[num_client]=num_group-1;
           num_client++;
           ret=pthread_create(&tid,NULL,recive_mex,(void*)index);
           if(ret==-1){
              printf("impossibile generare nuovo thread\n");
              pthread_mutex_unlock(&lock);
              exit(EXIT_FAILURE);
           }
           pthread_mutex_unlock(&lock);
           
           
      }
    



int main(int argc,char **argv){
   
      
      int listen_sock,ret,connect_sock;
      unsigned long k;
      struct sockaddr_in servADDR,cliADDR;
      int z;
      
      pthread_mutex_init(&lock,NULL);
      
      listen_sock=socket(AF_INET,SOCK_STREAM,0);
      if(listen_sock==-1){
	printf("creation error server\n");
	exit(-1);
      }
      
      servADDR.sin_family=AF_INET;
      servADDR.sin_port=htons(7000);
      servADDR.sin_addr.s_addr=htonl(INADDR_ANY);
      
      ret=bind(listen_sock,(struct sockaddr*)&servADDR,sizeof(servADDR));
      
      if(ret==-1){
	 printf("error bind server\n");
	 exit(-1);
      }
      ret=listen(listen_sock,3);
      
       if(ret==-1){
	 printf("error listen server\n");
	 exit(-1);
      }
     
      
      if(shm_addr==(void*)-1){
	printf("error page mapping\n");
	exit(-1);
      }
      
      ready=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*MAX_GROUP);
      if(ready==(void*)-1){
         printf("error malloc mutex\n");
         exit(-1);
      }
      
      int f=0;
      for(f=0;f<MAX_GROUP;f++){
        if(pthread_mutex_init(ready+f,NULL)||pthread_mutex_lock(ready+f)){
          printf("error mutex\n");
         }
      }
    
      pthread_t tid;
      int size;
      unsigned long index;
      char c[3];
      int i;
      char buffer[20];
      char name_group[20];
      int volonta;
      int n_bytes;
      int esito;
      int group_index;
      int len;
      char p[40];
      
      gruppo *new_group;
      size=sizeof(struct sockaddr_in); 
      
      while(1){
        
         
        
           
           printf("in attesa di collegamento\n");
           connect_sock=accept(listen_sock,(struct sockaddr*)&cliADDR,&size);   
           
           if(connect_sock==-1){
              printf("connessione non stabilita\n");
              exit(EXIT_FAILURE);
           }
           printf("connessione stabilita ip->%s\n",inet_ntoa(cliADDR.sin_addr));
           fflush(stdout);
           pthread_t tid;
           unsigned long id=connect_sock;
           
           pthread_create(&tid,NULL,manage_client,(void*)id);
     }
           
      pthread_join(tid,NULL);
    
      printf("fine della conversazione\n");
      fflush(stdout);
      exit(0);
}
      
             
      
