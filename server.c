#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define REQSIZE 2048 // the size of the client request (maximum character size for url)
#define LISTENQ 10 // number of clients allowed

/*
 Takes in the filename as argument, if the file exists it returns 0 if it does not exist it returns 1
*/
int if_file_exists(const char * filename){
    /* try to open file to read */
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        return 0;
    }
    return 1;
}

/*
Takes in the filename and a buffer, returns the bytes read which can be used in our return argument
*/
int file_to_buffer(char *filename, char** buff){ // read from file into a buffer
  FILE *fp;
  int fileLen;
  fp = fopen(filename, "rb");
  struct stat st;
  stat(filename, &st);
  fileLen = st.st_size;
  *buff = NULL;
  *buff = (char*)malloc(fileLen*sizeof(char)); // allocate correct size for buffer
  bzero(*buff,fileLen);
  int count = fread(*buff,sizeof(char),fileLen,fp);
  fclose(fp);
  return count;
}


/*
This function takes in the client request and parses it by separating into method, uri, and version
*/
void http_request(char req[], char** method, char** uri, char** version ){
  char *saveptr;
  *method = strtok_r(req," ", &saveptr);
  *uri = strtok_r(NULL," ", &saveptr);
  *version = strtok_r(NULL,"\n", &saveptr);
}


/*
Takes in filename, and char** return 0 if the file is of one of the types else it return 1
*/
int file_extensions(char filename[], char** extension){
  char* temp = strrchr(filename, '.');
  if (!temp){
    *extension = "text/html";
    return 1;
  }
  else{
    if(strcmp(temp+1, "html") == 0){
      *extension = "text/html";
      return 0;
    }
    if(strcmp(temp+1, "txt") == 0){
      *extension = "text/plain";
      return 0;
    }
    if(strcmp(temp+1, "png") == 0){
      *extension = "image/png";
      return 0;
    }
    if(strcmp(temp+1, "gif") == 0){
      *extension = "image/gif";
      return 0;
    }
    if(strcmp(temp+1, "jpg") == 0){
      *extension = "image/jpg";
      return 0;
    }
    if(strcmp(temp+1, "css") == 0){
      *extension = "text/css";
      return 0;
    }
    if(strcmp(temp+1, "js") == 0){
      *extension = "application/javascript";
      return 0;
    }
  }
}


int main(int argc, char **argv) {
  // variables used

  int serverfd; /* server socket */
  int clientfd; // after accept holds the requests from the client
  int serv_port_no; /* port to listen on */
  int optval; /* flag value for setsockopt */
  pid_t child_proc; //child process
  char request_buf[REQSIZE]; // holds the request
  socklen_t clientlen;
  struct sockaddr_in clientaddr, serveraddr; // client and server Address

// Make sure there are exactly two arguments when compiling
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  serv_port_no = atoi(argv[1]); // port number of the server

  //create the socket
  if ((serverfd = socket (AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Problem in creating the socket");
    exit(2);
  }


  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

  // Create the server socket Address
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)serv_port_no);

  //binding the socket
  if(bind(serverfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
    perror("Problem in binding the socket");
    exit(2);
  }

  // listen in on the server socket by creating a connection queue, wait for clients
  listen(serverfd, LISTENQ);

  printf("The server is now running waiting for clients \n");

  while(1){

    //accept the client connection
    clientfd = accept(serverfd, (struct sockaddr *) &clientaddr, &clientlen);

    // create a child process to handle the client request
    if((child_proc = fork()) == 0){
      close(serverfd); // close the server socket since we have already accepted a clients

      // keep getting requests from the client
      if((recv(clientfd, request_buf, REQSIZE, 0)) > 0){
        //printf("%s\n","Error 1");
        //printf("The request from the client is: %s \n", request_buf);

        // parse the method, uri, version from the buffer
        char *method, *uri, *version;
        http_request(request_buf, &method, &uri, &version);
        printf("%s, %s, %s\n", method, uri, version);

        //check extension of the file, if it not a file redirect to the index.html
        char *extension;
        int isFile = file_extensions(uri, &extension);
        printf("The file etension type is: %s, %d\n", extension, isFile );
        char *file_to_send = (char*)malloc((strlen(uri)+4)* sizeof(char));
        bzero(file_to_send, (strlen(uri)+4));
        strcpy(file_to_send, "www");
        strcat(file_to_send, uri);
        printf("%s\n", file_to_send);
        if(isFile == 0){ // it's a file type
          if(if_file_exists(file_to_send) == 0){ // check if the file exists on the server
            printf("%s\n", "The file exists -----------------------");
            char *file_buff; // holds the file content MAKESURE TO FREE FILEBUFF
            int FileSize = file_to_buffer(uri, &file_buff); // holds the size of file to send to the client
            int msg_size = 1000 + REQSIZE + FileSize;
            printf("Error1");
            char *msg_buff;
            //printf("%c\n", file_buff);
            printf("%s\n", "----------------------------------------------------");
            msg_buff = (char*)malloc(msg_size * sizeof(char)); //allocate space for the msg to send to the client MAKESURE TO FREE
            bzero(msg_buff, msg_size);
            //edit the msg to send to the clients

            printf("%s\n", "about to send");
            sprintf(msg_buff, "%s 200 OK\nContent-Type: %s\nContent-Length: %d\n\n", version, extension, FileSize);
            memmove((msg_buff + strlen(msg_buff)), file_buff, FileSize); //add the contents of the file to the end

            //send the file to the client
            send(clientfd,msg_buff,msg_size,0);

            free(file_buff); //freeing up the allocated space
            free(msg_buff); // freeing up the allocated space

          }

          else{ // the file doesn't exist so send error msg
            printf("%s\n", "The file does not exists -----------------------");
            int msg_size = 1000;
            char *msg_buff;
            msg_buff = (char*)malloc(msg_size * sizeof(char));
            bzero(msg_buff, msg_size);
            sprintf(msg_buff, "%s 500 Internal Server Error\n",version);

            //send the file to the client
            send(clientfd,msg_buff,strlen(msg_buff),0);
            free(msg_buff); // freeing up the allocated space
          }
        }

        else{ // set it to index.html
          char myfile[] = "www/index.html";
          char *file_buff; // holds the file content MAKESURE TO FREE FILEBUFF
          int FileSize = file_to_buffer(myfile, &file_buff); // holds the size of file to send to the client
          int msg_size = 1000 + REQSIZE + FileSize;
          char *msg_buff;

          msg_buff = (char*)malloc(msg_size * sizeof(char)); //allocate space for the msg to send to the client MAKESURE TO FREE
          bzero(msg_buff, msg_size);

          printf("%s\n", file_buff);
          printf("%s\n", "----------------------------------------------------");

          //edit the msg to send to the clients
          sprintf(msg_buff, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %d\n\n", extension, FileSize);
          memmove((msg_buff + strlen(msg_buff)), file_buff, FileSize); //add the contents of the file to the end
          printf("This is the msg:   %s\n", msg_buff);
          //send the file to the client
          send(clientfd,msg_buff,msg_size,0);

          free(file_buff); //freeing up the allocated space
          free(msg_buff); // freeing up the allocated space
        }
        // close the client when the conditions have been met and terminate the child
        free(file_to_send); //clean the file to send memory
      }
      close(clientfd);
      exit(0);
    }
    close(clientfd);
  }

  return 0;
}
