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




typedef struct chat_group{
    
    int group_admin;
    int group_id;
    int num_client_connect;
    char group_name[20];
    int num_client_group[100];
}gruppo;



int create_new_group(int channel,char *name,struct chat_group *c,int num){
    
    
    if(num==100){
       return 0;
    }
    int z;
    
    for(z=0;z<100;z++){
      c->num_client_group[z]=21;
    }
    
    memcpy(c->group_name,name,strlen(name));
    c->group_name[strlen(name)]='\0';
    
    c->num_client_group[c->num_client_connect]=channel;
    c->group_admin=channel;
    //printf("%d-%d\n",channel,c->group_admin);
    c->num_client_connect++;
    c->group_id=num;
    send(channel,"gruppo creato con successo",strlen("gruppo creato con successo"),0);
    
    return 1;
}


int add_new_client(int channel,struct chat_group *c){
    
    int i=0;
     
     for(i;i<100;i++){
       if(c->num_client_group[i]==21){
          c->num_client_group[i]=channel;
          c->num_client_connect++;
          return 1;
       }
     }
     return 0;
}


int remove_client(int channel,struct chat_group *c){
    
    int i=0;
    //printf("%d-%s\n",c->group_admin,c->group_name);
    for(i=0;i<100;i++){
      
      if(c->num_client_group[i]==channel){
         
         c->num_client_group[i]=21;
         
         break;
         }
    }
    //printf("%d-%d\n",channel,c->group_admin);
    if(channel==c->group_admin && c->num_client_connect!=1){
        for(i=0;i<100;i++){
           
           if((c->num_client_group[i]!=21) && (c->num_client_group[i]!=c->group_admin)){
              c->group_admin=c->num_client_group[i];
              c->num_client_connect--;
              send(c->num_client_group[i],"sei il nuovo amministratore",strlen("sei il nuovo amministratore"),0);
              printf("new_admin=%d\n",c->group_admin);
              return 2;
           }
        }
    }
    c->num_client_connect--;
    return 0;
}

