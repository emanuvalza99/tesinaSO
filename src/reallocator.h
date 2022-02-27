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


#define MAX_CLIENT 100
#define MAX_GROUP 20






int reallocated_client(unsigned long index_client,gruppo **group,int *array_client,int *presence_client,int *n_group,int *n_client,char **shm_array){
   
    
    char c[3];
    int volonta;
    char buffer[4096];
    gruppo *new_group;
    int n_bytes,ret,i,esito;
    unsigned long k;
    pthread_t tid; 


again:    
    send(array_client[index_client],"operazioni disponibili:\n1)crea nuovo gruppo chat\n2)joinare ad un gruppo esistente\n3)panoramica gruppi esistenti",strlen("operazioni disponibili:\n1)crea nuovo gruppo chat\n2)joinare ad un gruppo esistente\n3)panoramica gruppi esistenti"),0);
           if(recv(array_client[index_client],c,3,0)==0){
             printf("realloc failed\n");  
             return -1;
           }
           volonta=atoi(c);
           if(volonta==0) goto again;
          
           if(volonta==1){
name_exist:     
                
                memset(buffer,20,0);
                n_bytes=recv(array_client[index_client],buffer,20,0);
                buffer[n_bytes]='\0';
                int exist=0;
                for(i=0;i<(*n_group);i++){
                  if(group[i]==NULL) continue;
                  if(strcmp(buffer,group[i]->group_name)==0){
                    send(array_client[index_client],"gruppo esistente",strlen("gruppo esistente"),0);
                    printf("exist\n");
                    exist=1;
                    break;
                  }
                }
                if(exist==1) goto name_exist;
                new_group=(gruppo*)malloc(sizeof(gruppo));
                new_group->num_client_connect=0;
                printf("creo %s\n",buffer);
                create_new_group(array_client[index_client],buffer,new_group,*n_group);
                if(*n_group==0){
                    group[0]=new_group;
                    shm_array[0]=(char*)mmap(NULL,4096,PROT_READ|PROT_WRITE|0660,MAP_SHARED|MAP_ANONYMOUS,0,0);
                    if(shm_array[i]==(void*)-1){
                        printf("error shm\n");
                        exit(EXIT_FAILURE);
                    }
                    presence_client[index_client]=0;
                    return 21;
                }
                
                for(i=0;i<MAX_GROUP;i++){
                  
                  if(group[i]==NULL){
                      group[i]=new_group;
                      
                      
                      shm_array[i]=(char*)mmap(NULL,4096,PROT_READ|PROT_WRITE|0660,MAP_SHARED|MAP_ANONYMOUS,0,0);
                      if(shm_array[i]==(void*)-1){
                        printf("error shm\n");
                        exit(EXIT_FAILURE);
                      }
                      send(array_client[index_client],"gruppo creato con successo",strlen("gruppo creato con successo"),0);
                      presence_client[index_client]=i;
                      return 21;
                    }
                 
                }
                if(i==MAX_GROUP){
                  printf("max_group\n");
                }
                                
            }
            if(volonta==2){
                
                
                
                
try_again:
                if(n_group==0){
                  send(array_client[index_client],"EMPTY",strlen("EMPTY"),0);
                  goto again;
                }
                send(array_client[index_client],"OK",strlen("OK"),0);
                memset(buffer,20,0);
                n_bytes=recv(array_client[index_client],buffer,20,0);
                buffer[n_bytes]='\0';
                printf("%s\n",buffer);
                for(i=0;i<MAX_GROUP;i++){
                  if(group[i]==NULL){
                     
                     continue;
                  }
                  
                  
                  if(esito=strcmp(buffer,group[i]->group_name)==0){
                     
                     printf("esito=%d\n",esito);
                     
                     add_new_client(array_client[index_client],group[i]);
                     //n_bytes=send(array_client[index_client],"sei stato aggiunto al gruppo",strlen("sei stato aggiunto al gruppo"),0);
                     
                     esito=1;
                     sleep(1);
                     send(array_client[index_client],"sei stato aggiunto al gruppo",strlen("sei stato aggiunto al gruppo"),0);
                     break;
                  }   
                }          
                  if(esito!=1){
                   
                    send(array_client[index_client],"non esiste un gruppo con questo nome",strlen("non esiste un gruppo con questo nome"),0); 
                    memset(buffer,20,0);
                    goto try_again;
                  }
             }
             
           
             if(volonta==3){
               
               
               int len;
               long int byte;
               char p[40];
               
               
               memset(p,0,40);
               
               if(*n_group==0){
                  
                  send(array_client[index_client],"nessun gruppo esistente",strlen("nessun gruppo esistente"),0);
                  sleep(1);
                  send(array_client[index_client],"end",strlen("end"),0); 
                  sleep(1);
                  goto again;
               }
               else{
                 
                 for(i=0;i<MAX_GROUP;i++){
                   if(group[i]!=NULL){
                        
                   len=strlen(group[i]->group_name);
                  
                   memcpy(p,group[i]->group_name,strlen(group[i]->group_name));
                   p[len]='\0';
                  
                   sleep(1);
                   byte=send(array_client[index_client],p,strlen(p),0);
                  
                   if(i+1<*n_group && (strlen(group[i]->group_name)<len)){
                     memset(p,0,40);
                   }
                   
                  }
                 }
               }
               sleep(1);
               send(array_client[index_client],"end",strlen("end"),0); 
               sleep(1);
               goto again;
           }
              
           return i;
}
 
