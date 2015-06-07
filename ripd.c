 /*-------------------------------
 - Author:Tornike Abuladze       -
 -                               -
 - Lector:George Kobiashvili     -
 -                               -
 - Midterm:17/11/14              -
 --------------------------------*/


 #include <netdb.h>
 #include <ifaddrs.h>
 #include <sys/types.h>   /* for type definitions */
 #include <sys/socket.h>  /* for socket API function calls */
 #include <netinet/in.h>  /* for address structs */
 #include <arpa/inet.h>   /* for sockaddr_in */
 #include <stdio.h>       /* for printf() */
 #include <stdlib.h>      /* for atoi() */
 #include <string.h>      /* for strlen() */
 #include <unistd.h>      /* for close() */
 #include <inttypes.h>
 #include "uthash.h"
 #include <pthread.h> 
 #include <semaphore.h>

 #define MAX_LEN  2048   /* maximum receive string size */
 sem_t map_semaphore;

 struct my_struct {
    uint32_t id; /* we'll use this field as the key */
    uint32_t subnet_mask;
    uint32_t metric;
    uint32_t received_from;          
    UT_hash_handle hh; /* makes this structure hashable */
 };
 struct my_struct *table = NULL;
 //konkretuli interfeisis struqtura
 struct concrete_interfeice{
    uint32_t ip;
    uint32_t network_addr;
    uint32_t mask;
 };
 //yvela interfeisis shenaxvistvis
 struct all_interfeices{
    struct concrete_interfeice *array;
    int size;
 };
 
 //add to hashtable
 void add_entry(struct my_struct *s) {
    HASH_ADD_INT(table,id,s );    
 }
 // find
 struct my_struct *find_entry(int entry_id) {
    struct my_struct *s;

    HASH_FIND_INT( table, &entry_id, s );  
    return s;
 }
 //delete from table
 void delete_entry(struct my_struct *entry) {
    HASH_DEL( table, entry);  
 }


 //-----------------------------------------------------------------//
 //-----------------------------------------------------------------//
 //-----------------------------------------------------------------//
 
 void* mc_receive(void * none)
 {
    int sock;                     /* socket descriptor */
    int flag_on = 1;              /* socket option flag */
    struct sockaddr_in mc_addr;   /* socket address structure */
    void * recv_packet=malloc(MAX_LEN);     /* buffer to receive string */
    int recv_len;                 /* length of string received */
    struct ip_mreq mc_req;        /* multicast request structure */
    char* mc_addr_str;            /* multicast IP address */
    unsigned short mc_port;       /* multicast port */
    struct sockaddr_in from_addr; /* packet source */
    uint32_t from_len;        /* source addr length */


    mc_addr_str = "224.0.0.9";      /* arg 1: multicast ip address */
    mc_port = atoi("520");    /* arg 2: multicast port number */


    /* create socket to join multicast group on */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("socket() failed");
      exit(1);
    }
    
    /* set reuse port to on to allow multiple binds per host */
    if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag_on,
         sizeof(flag_on))) < 0) {
      perror("setsockopt() failed");
      exit(1);
    }

    /* construct a multicast address structure */
    memset(&mc_addr, 0, sizeof(mc_addr));
    mc_addr.sin_family      = AF_INET;
    mc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    mc_addr.sin_port        = htons(mc_port);

    /* bind to multicast address to socket */
    if ((bind(sock, (struct sockaddr *) &mc_addr, 
         sizeof(mc_addr))) < 0) {
      perror("bind() failed");
      exit(1);
    }

    /* construct an IGMP join request structure */
    mc_req.imr_multiaddr.s_addr = inet_addr(mc_addr_str);
    mc_req.imr_interface.s_addr = htonl(INADDR_ANY);

    /* send an ADD MEMBERSHIP message via setsockopt */
    if ((setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
         (void*) &mc_req, sizeof(mc_req))) < 0) {
      perror("setsockopt() failed");
      exit(1);
    }

    for (;;) {          /* loop forever */

      /* clear the receive buffers & structs */
      memset(recv_packet, 0, MAX_LEN);
      from_len = sizeof(from_addr);
      memset(&from_addr, 0, from_len);

      /* block waiting to receive a packet */
      if ((recv_len = recvfrom(sock, recv_packet, MAX_LEN, 0, 
           (struct sockaddr*)&from_addr, &from_len)) < 0) {
        perror("recvfrom() failed");
        exit(1);
      }
      uint32_t ip,mask;
      /* output received string */
      
      uint32_t recv_rom; //am interfeisze momivida
      inet_pton(AF_INET,inet_ntoa(from_addr.sin_addr),&recv_rom);
      int num_entry=(recv_len-4)/20;
      void* curr_pointer=recv_packet;
      
      curr_pointer=(char*)curr_pointer+4; //
      int i;
      char * dst=(char*)malloc(32);
      printf("recived new packet from: %s\n",inet_ntoa(from_addr.sin_addr));

      sem_wait(&map_semaphore); //icvleba mapi da davblokot
      printf("number_of_entrys: %d\n",num_entry);

      //wamovigot satitaod yvela entry paketidan
      for(i=0; i<num_entry; i++)
      {
          printf("entry_N %d\n",i+1);
          curr_pointer=(char*)curr_pointer+4;
          
          uint32_t network_ip=*((uint32_t*)curr_pointer);
          curr_pointer=(char*)curr_pointer+4;//gadavaxtune pirdapir subnet ze;
          
          uint32_t subnet_mask=*((uint32_t*)curr_pointer);
          curr_pointer=(char*)curr_pointer+8; //gadavaxti nexthops da gadavedi metricze
          
          uint32_t metric=ntohl(*((uint32_t*)curr_pointer));
          curr_pointer=(char*)curr_pointer+4;//gadaviyvane axal entry ze
          
          memset(dst, 0,32);
          inet_ntop(AF_INET, &network_ip,dst,32);
          printf("network_ip: %s\n",dst);

          memset(dst, 0,32);
          inet_ntop(AF_INET, &subnet_mask,dst,32);
          printf("subnet_mask: %s\n",dst);

          printf("metric: %u\n",metric);
          
          if(metric>15) //tu metia agar vagdeb cxrilshi
            continue;
          if(network_ip==127) //anu aq tu 127.0.0.0 ia ar vixilav, anu networ ordershi chawerili 27 ia.
            continue;

          struct my_struct * update_entry=find_entry(network_ip);
          if(update_entry!=NULL)
          {
          	  //shevamowmeb tu umjobesdeba an isev igive interfeisidan tu movida mashin
          	  // uechveli shevcvli tu gauzvirda imas mec unda gamizvirdes
              if(update_entry->metric > metric || update_entry->received_from==recv_rom)
              {
                 printf("%s\n","table_update:");
                 
                 memset(dst, 0,32);
                 inet_ntop(AF_INET, &(update_entry->id),dst,32);
                 printf("entry_network_ip: %s\n",dst);
    
                 printf("metric was %u now is %u\n",update_entry->metric,metric);
                 update_entry->subnet_mask=subnet_mask;
                 update_entry->metric=metric;
                 update_entry->received_from=recv_rom;
              }
          }
          else
          {
          	 //tu cxrilshi jer saertod ar maq
             memset(dst, 0,32);
             inet_ntop(AF_INET, &network_ip,dst,32);
             printf("new network with ip: %s added in the table\n",dst);
             
             memset(dst, 0,32);
             inet_ntop(AF_INET, &subnet_mask,dst,32);
             printf("subnet_mask: %s\n",dst);

             printf("metric: %u\n",metric);

             struct my_struct * new_entry=(struct my_struct *)malloc(sizeof(struct my_struct));
             new_entry->id=network_ip;
             new_entry->subnet_mask=subnet_mask;
             new_entry->metric=metric;
             new_entry->received_from=recv_rom;
             add_entry(new_entry);
          }
          printf("end_description_entry_N %d\n",i+1);
          printf("%s\n","");

      }
      sem_post(&map_semaphore);
    }

    /* send a DROP MEMBERSHIP message via setsockopt */
    if ((setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
         (void*) &mc_req, sizeof(mc_req))) < 0) {
      perror("setsockopt() failed");
      exit(1);
    }

    close(sock);  
 }

 //-----------------------------------------------------------------//
 //-----------------------------------------------------------------//
 //-----------------------------------------------------------------//
  struct rip_packet{
    void* raw_memory;
    int byte_used;
  };
  //ramdeni entry meqneba gavigo
  int number_of_entry(struct concrete_interfeice * inFc)
  {
    int answ=0;
    struct my_struct *s;
    for(s=table; s != NULL; s=(struct my_struct *)s->hh.next)
      if(s->received_from!=inFc->ip)
        answ++;
    return answ;
  }
  //avawyot rip paketi
  struct rip_packet* make_rip_packet(struct concrete_interfeice * inFc)
  {
     struct rip_packet* packet=(struct rip_packet*)malloc(sizeof(struct rip_packet));
     packet->byte_used=4+number_of_entry(inFc)*20;
     packet->raw_memory=malloc(packet->byte_used);
     ((char*)(packet->raw_memory))[0]=2;
     ((char*)(packet->raw_memory))[1]=2;
     ((char*)(packet->raw_memory))[2]=0;
     ((char*)(packet->raw_memory))[3]=0;
     
     //movilie hederi
     void * curr_pointer=(char*)packet->raw_memory;
     curr_pointer=(char*)curr_pointer+4;
     struct my_struct *s;
     for(s=table; s != NULL; s=(struct my_struct *)s->hh.next) {
         if(s->received_from!=inFc->ip)
         {
            *((uint16_t*)curr_pointer)=htons(2);
            curr_pointer=(char*)curr_pointer+2;
            *((uint16_t*)curr_pointer)=htons(0);
            curr_pointer=(char*)curr_pointer+2;
            *((uint32_t*)curr_pointer)=s->id;
            curr_pointer=(char*)curr_pointer+4;
            *((uint32_t*)curr_pointer)=s->subnet_mask;
            curr_pointer=(char*)curr_pointer+4;
            *((uint32_t*)curr_pointer)=htonl(0);
            curr_pointer=(char*)curr_pointer+4;
            *((uint32_t*)curr_pointer)=htonl(s->metric+1);
            curr_pointer=(char*)curr_pointer+4;
         }    
     }
    return packet;
  }
  //-----------------------------------------------------------------//
  //-----------------------------------------------------------------//
  //-----------------------------------------------------------------//
 void mc_send(struct all_interfeices * allIn)
 {
    int sock;                   /* socket descriptor */
    struct sockaddr_in mc_addr; /* socket address structure */
    uint32_t send_len;      /* length of string to send */
    char* mc_addr_str;          /* multicast IP address */
    unsigned short mc_port;     /* multicast port */
    unsigned char mc_ttl=1;     /* time to live (hop count) */


    mc_addr_str ="224.0.0.9";       /* arg 1: multicast IP address */
    mc_port     = atoi("520"); /* arg 2: multicast port number */

    /* create a socket for sending to the multicast address */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("socket() failed");
      exit(1);
    }
    
    /* set the TTL (time to live/hop count) for the send */
    if ((setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, 
         (void*) &mc_ttl, sizeof(mc_ttl))) < 0) {
      perror("setsockopt() failed");
      exit(1);
    }
    while(1)
    {
        int i;
        for(i=0; i<allIn->size-1; i++)
        {
            struct concrete_interfeice* inFc=&(allIn->array[i]);
            struct in_addr localInterface;

            char dst[32];
            localInterface.s_addr=inFc->ip;
            if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
            {
                perror("Setting local interface error");
                exit(1);
            }

            /* construct a multicast address structure */
            memset(&mc_addr, 0, sizeof(mc_addr));
            mc_addr.sin_family      = AF_INET;
            mc_addr.sin_addr.s_addr = inet_addr(mc_addr_str);
            mc_addr.sin_port        = htons(mc_port);

            sem_wait(&map_semaphore); //aq viwyeb mesigis awyobas amitom maps vblokav rom ar sheicvalos

            struct rip_packet* packet=make_rip_packet(inFc);
            void* send_content=packet->raw_memory;
            send_len=packet->byte_used;
            /* send string to multicast address */
            if ((sendto(sock, send_content, send_len, 0, 
                 (struct sockaddr *) &mc_addr, 
                 sizeof(mc_addr))) != send_len) {
              perror("sendto() sent incorrect number of bytes");
              exit(1);
            }
            /* clear send buffer */
            free(send_content);
            free(packet);
            sem_post(&map_semaphore);
        }
        sleep(30);
    }
    close(sock);  

    exit(0);
 }

//-----------------------------------------------------------------//
//-----------------------------------------------------------------//
//-----------------------------------------------------------------//
 int main(int argc, char *argv[])
 {
     struct ifaddrs *ifaddr, *ifa;
     int family, s,s1;
     char host[NI_MAXHOST];
     char mask[NI_MAXHOST];

     if (getifaddrs(&ifaddr) == -1) {
         perror("getifaddrs");
         exit(EXIT_FAILURE);
     }
     sem_init(&map_semaphore,0,1);

     /* Walk through linked list, maintaining head pointer so we
        can free list later */
     struct all_interfeices * allIn; 
     allIn=(struct all_interfeices*)malloc(sizeof(struct all_interfeices));
     allIn->size=1;
     allIn->array=(struct concrete_interfeice*)malloc(sizeof(struct concrete_interfeice));
     
     pthread_t receiver_thread;
     pthread_create(&receiver_thread , NULL , mc_receive ,(void*)NULL);

     for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
         if (ifa->ifa_addr == NULL)
             continue;
         family = ifa->ifa_addr->sa_family;
          /* Display interface name and family (including symbolic
            form of the latter for the common families) */
         if (family == AF_INET) {
             
             struct sockaddr_in *ip_address = (struct sockaddr_in *)ifa->ifa_addr;
             struct sockaddr_in *subnet_mask = (struct sockaddr_in *)ifa->ifa_netmask;
             uint32_t adress = ip_address->sin_addr.s_addr;
             uint32_t netmask = subnet_mask->sin_addr.s_addr;

             uint32_t network_addr=adress&netmask;
             if(network_addr==127)
                continue;

             //chamateba hashtableshi
             struct my_struct* curr=(struct my_struct*)malloc(sizeof(struct my_struct));
             curr->id= network_addr;
             curr->subnet_mask=netmask;
             curr->metric=0;
             curr->received_from=-11; //nebismierad tavidan      
             
             add_entry(curr); //davamate mapshi

             struct concrete_interfeice curr_interface;
             curr_interface.ip=adress;
             curr_interface.network_addr=network_addr;
             curr_interface.mask=netmask;
             
             //interfeisi davamate  structurashi
             allIn->array[allIn->size-1]=curr_interface;
             allIn->size++;
             allIn->array=(struct concrete_interfeice*)realloc(allIn->array,allIn->size*sizeof(struct concrete_interfeice));
         }
     }
     mc_send(allIn);
     freeifaddrs(ifaddr);
     pthread_exit(NULL);
     exit(EXIT_SUCCESS);
 }
