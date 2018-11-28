#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <ctype.h>
#include <signal.h>
#include <sys/un.h>
//a lot of headers
#define PORT 21574
#define BUFMAX 2048
#define MAX 1024
#define STRUCTSIZE 32

struct persons { //size if 32 (no padding)
  int acctnum;
  char name[20]; //if char is 21, then the total size if 36 because 3 bytes of padding
  float value;
  int age;
};

//IS THIS NECESSARY?? look up zombie heavyweight child processes
void signal_catcher(int the_sig) {
  wait(0); //cleanup the zommbie?
}

//*******************************************************************************************
//to get the info from db18
char * query(int acctnum, char * final) {
  //don't need lockf()
  int fd;
  int flag = 0;
  int search = acctnum;
  char invalid[30] = "acc num not found";
  //printf("search is : %d\n", search); //ntohs() here will get 26411

  char res[40]; //make sure if big enough

  char buffer[21];
  int acc;
  float value;

  char floatbuff[30];
  int numchars;//_RDONLY

  if ((fd = open("db18", O_RDONLY, 0)) == -1) {
    perror("open failed");
    exit(1);
  }
  //now to search
  while ((numchars = read(fd, &acc, sizeof(int))) > 0) {
    //printf("while loop accnum is: %d\n", acc);

    if (acc == search) {
	  flag = 1;
      read(fd, &buffer, 20);
      read(fd, &value, sizeof(float));

      //printf("found name is: %s\n", buffer);
      //printf("found value is: %f\n", value);

      memset(res, 0, strlen(res)); //clear the result string
      strcat(res, buffer);
      //printf("adding name to res: %s\n", res);
      strcat(res, " ");
	  char numbuf[20];
      sprintf(numbuf, "%d", search);
	   strcat(res, numbuf);
      //printf("add acctnum to name: %s\n", res);
      strcat(res, " ");
      gcvt(value, 6, floatbuff); //6 for 6 digits, converts float to string
      //printf("gcvt float string is: %s\n", floatbuff);
      strcat(res, floatbuff); //this turns string to int as well?
      break;
    }
    lseek(fd, 28, SEEK_CUR); //think about file alignment!!!!!!!!!!!!!!!!!!!!!!!!!
  }
  close(fd);
  if (flag == 1) {
    strcpy(final,res); //does this work or do i need to change the value of final??
  }
  else { //account number not found
    strcpy(final, invalid);
  }
  //printf("final return string is: %s\n", final);
  return final;
}

//*******************************************************************************************
char * update(int acctnum, float amount, char * final) {
  int fd;
  int search = acctnum;
  int flag = 0;
  char invalid[30] = "acc num not found";
  char res[40];

  char buffer[21];
  int acc;
  float value;

  char floatbuff[30];
  int numchars;

  float chgamt = amount;
  //printf("chgamt is: %f\n", chgamt);
  float chgres;

  if ((fd = open("db18", O_RDWR, 0)) == -1) {
    perror("open failed\n");
    exit(1);
  }
  //now to search
  while ((numchars = read(fd, &acc, sizeof(int))) > 0) {
    if (acc == search) {
      flag = 1;
      //lockf the record****************
      //fseek(fd, -4, SEEK_CUR);
      lockf(fd, F_LOCK, 28); //don't need last int "Age"

	//test to see if lockf works
	    //sleep(10);

      //fseek(fd, -24, SEEK_CUR); //BUG: lockf does not move the file pointer
        //this is beacuse the file is open in two places, 2 file pointers

      read(fd, &buffer, 20);
      read(fd, &value, (sizeof(float)));
      //printf("value of record is: %f\n", value);
      chgres = value + chgamt;
      //printf("chg result is: %f\n", chgres);
      lseek(fd, -sizeof(float), SEEK_CUR);
      write(fd, &chgres, sizeof(float));
      //unlockf the record
      lseek(fd, sizeof(float), SEEK_CUR);
      //sleep(7);
      lockf(fd, F_ULOCK,-28);
	  char numbuf[20];
      sprintf(numbuf, "%d", acctnum);
      memset(res, 0, strlen(res)); //clear the result string
      strcat(res, buffer);
      strcat(res, " ");
      strcat(res, numbuf);
      strcat(res, " ");
	  sprintf(numbuf, "%f", chgres);
      strcat(res, numbuf);
      break;
    }
    lseek(fd, 28, SEEK_CUR); //think about file alignment!!!!!!!!!!!!!!!!!!!!!!!!!
  }
  close(fd);
  if (flag == 1) {
    strcpy(final, res);
  }
  else { //acc not found
	strcpy(final, invalid);
  }
  return final;
}


int main(int argc, char* argv[]) {
  //printf("server started\n");
  //printf("%ld\n", sizeof(struct persons));

  //char bufin[BUFMAX]; //to hold incoming message
  //char bufout[BUFMAX]; //to send outgoing message

//BUG: if i move broadcasting bind to after TCP bind, bind fails??

  //tcp with client---------------------------------------------------
  int orig_sk, new_sk;
  struct sockaddr_in clnt_adr, serv_adr;
  socklen_t clnt_len;
  int serv_len = sizeof(struct sockaddr_in);
  serv_len = sizeof (struct sockaddr);
  int l, i;
  if ((orig_sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("cannot create a socket");
    exit(1);
  }
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = INADDR_ANY;
  serv_adr.sin_port = 0; //let system assign port number

  //bind socket to the port number (assigned by system) (if no bind then port num is 0)
  if (bind(orig_sk, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0) {
    close(orig_sk);
    perror("orig_sk bind error");
    exit(1);
  }

  //port num was getting zero because last argument passed in need to be an initialized int
  getsockname(orig_sk, (struct sockaddr *) &serv_adr, &serv_len);
//  printf("tcp port num is %d\n", htons(serv_adr.sin_port));


  //broadcasting ---------------------------------------------------
  struct sockaddr_in remote;
  int sk;
  char buf[BUFMAX];
  //char buf20[BUFMAX];
  int remotelen = sizeof(remote);
  if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create a socket");
    exit(1);
  }

  remote.sin_family = AF_INET; /* Define the socket domain */
  remote.sin_addr.s_addr = inet_addr("137.148.205.255"); //broadcast addr
  remote.sin_port = ntohs(PORT); /* Let the system assign a port */
  //printf("whats going on \n");
  /*
  if(bind(sk, (struct sockaddr *) &local, sizeof(local)) < 0) {
    printf("udp sock bind error\n");
    exit(1);
  }

  //getsockname(sk, (struct sockaddr *) &local, &len);
  //printf("udp sk has port %d\n", htons(local.sin_port));
  */
  //create message to broadcast to servicemap----------------------------
  int portn = htons(serv_adr.sin_port);
  char message[50] = "PUT CISBANK ";
  sprintf(message, "%s%d", message, portn);
  //message now contains that thing to send
//  printf("tcp num string is %s\n", message);

  //broadcasting start------------------------------------
  setsockopt(sk, SOL_SOCKET, SO_BROADCAST, (struct sockaddr *) &remote, sizeof(remote));
  sendto(sk, message, strlen(message) + 1, 0, (struct sockaddr *) &remote, sizeof(remote));
  //read or recvfrom? recv means connected transmission, TCP
//BUG: We have to use recvfrom instead of read because read doesn't upate the remote socket struct
	//with the IP address of the servicemap while recvfrom addresses this issue
  recvfrom(sk, buf, BUFMAX, 0, (struct sockaddr *)&remote, &remotelen);
  char str[INET_ADDRSTRLEN]; //store the IP as string
  if (strncmp(buf, "OK", 2) != 0) {
    printf("Registraion NOT OK\n");
    exit(1);
  }
  //bug: before was printing out remote.sin_addr.s_addr and getting garbage
    //inet_ntop will convert sin_addr to IP stting address
  inet_ntop(AF_INET, &(remote.sin_addr), str, INET_ADDRSTRLEN);
  printf("Registration OK from %s\n", str);
  //no longer need to talk to servicemap
  close(sk);

  //tcp wait for client to come here--------------------------------------------
  //SHOUDL THE ZOMBIE CHILD BE CAUGHT HERE???????
  if (signal(SIGCHLD, signal_catcher) == SIG_ERR) {
    perror("SIGCHILD");
    exit(1);
  }
  char recvbuf[50];
  char recvbuf2[50];
  char * res;
  res = malloc(30);
  //res = malloc(sizeof(float) + sizeof(int) + 21);
  if (listen(orig_sk, 5) < 0) { //why check here?
    close(orig_sk);
    printf("error listening tcp\n");
    exit(1);
  }
    //printf("database service started (listen successful)\n");
    clnt_len = sizeof(clnt_adr);
	do{
    if ((new_sk = accept(orig_sk, (struct sockaddr *) &clnt_adr, &clnt_len)) < 0) {
      close(orig_sk);
      close(new_sk);
      printf("error accepting tcp\n");
      exit(1);
    }
    //now for the forking part
    if (fork() == 0) { //child
      close(orig_sk);
	  int recint;
	  int recacc;
	  do {
      recv(new_sk, &recint,MAX, 0);
	  inet_ntop(AF_INET, &(clnt_adr.sin_addr), str, INET_ADDRSTRLEN);
 	  //printf("Service Requested from %s\n", str);
	  recint = ntohl(recint);
      if (recint == 1000) {
        printf("Service Requested from %s\n", str);
        l = recv(new_sk, &recacc, MAX, 0);
        //printf("received acctnum: %d\n", ntohl(recacc));
        query(ntohl(recacc), res);
        //printf("length of message to send: %d\n", l);
        send(new_sk, res, strlen(res), 0);
       // printf("1000 result: %s\n", res);
        free(res); //allocated memory in database update
      }
      else if (recint == 1001) {
        printf("Service Requested from %s\n", str);
		float recval;
		int * iptr = (int *)&recval;
        l = recv(new_sk, &recacc, sizeof(int), 0);
        l += recv(new_sk, &recval, sizeof(int), 0);
		int temp = ntohl(*iptr);
		recval = *(float *)&temp;
        //printf("received acctnum: %d, change amt: %f\n", ntohl(recacc),recval);
        update(ntohl(recacc), recval, res);
        //printf("length of message to send: %d\n", l);
        send(new_sk, res, strlen(res), 0);
		strcpy(res," ");
        //printf("1001 result: %s\n", res);
       //allocated memory in database update
      }
      else {
		free(res);
		exit(0);
      }
	  }while(1);

      close(new_sk);
      exit(0);
    }
	close(new_sk);
	}while(1);
    //this is the parent

    //printf("finished one database service request\n");

  close(orig_sk); //after no more clients, close original socket
    //we will never get here, program ends with ctrl + C
}
