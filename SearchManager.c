#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include "longest_word_search.h"
#include "queue_ids.h"

size_t strlcpy(char*dst, const char *src, size_t size);
int getMSQID();
response_buf getResponse(int msqid);
void sendMessage(int type, int id,int msqid, char* prefix);
void sigIntHandler(int sig_num);

char** globalPrefixArray;
int  globalPassageCount = 0;
int globalPrefixCount = 0;
sem_t globalCurrentPrefix;
sem_t globalCurrentPassage;


int main(int argc, char**argv){
    int prefixIndexes[argc];//This will store the index in argv[] of each valid prefiix
    int validPrefixes = 0;
    if (argc < 3 ) {//Error out because no prefix
        fprintf(stderr,"Error: please provide at least one prefix of at least two characters, and a time\n");
        fprintf(stderr,"Usage: %s <time> <prefix>\n",argv[0]);
        exit(-1);
    }
    for(int i = 2; i < argc; i++){//if prefixes are invalid ignore them
      if (strlen(argv[i]) <=2){//Prefix too short
        fprintf(stderr,"\"%s\" is too short to be a valid prefix.\nIgnoring ... \n\n",argv[i]);
      }
      else if(strlen(argv[i]) >=21){//Prefix too long
        fprintf(stderr,"\"%s\" is too long to be a valid prefix.\nIgnoring ... \n\n",argv[i]);
      }
      else{//prefix is fine, add the index to our array
        prefixIndexes[validPrefixes] = i;
        validPrefixes ++;
      }
    }
    if (validPrefixes < 1){//Check to see if we have disqaulified all prefixes
      fprintf(stderr,"Error: please provide at least one valid prefix of at least two characters for search\n");
      exit(-1);
    }

//We have valid input
    int msqid = getMSQID();
    int delay= atoi(argv[1]);
    response_buf rbuf;

    char* localPrefixArray[validPrefixes]; //declares a local array to assign global pointer to
    for (int i = 0; i < validPrefixes; i++){ //for each valid prefix, add it to the array
      strlcpy(localPrefixArray[i],argv[prefixIndexes[i]],WORD_LENGTH); //copy it in
    }
    globalPrefixArray = localPrefixArray; //assign the global array to the local one
    globalPrefixCount = validPrefixes; //assgins the number of prefixes to the global value
    sem_init(&globalCurrentPrefix, 0, 0); //intialize current prefix to 0
    sem_init(&globalCurrentPassage, 0, 0); //initialize passage count to zero
    signal(SIGINT, sigIntHandler); //enable sigIntHandler


    for (int j = 0; j < validPrefixes; j++){//this loop runs for each valid prefix
      sendMessage(1, j+1, msqid, argv[prefixIndexes[j]]);//send a message of this prefix
      sem_post(&globalCurrentPrefix);//increment atomically the number of the prefix we're on
      if (j!=0) {sem_init(&globalCurrentPassage, 0, 0);} //set passage count to zero on all but the first loop

      rbuf = getResponse(msqid); //get a response

      response_buf responses[rbuf.count]; //creates array of size(number of passages)
      int passageCount = rbuf.count; //takes down thenumber of passages for the loop
      responses[rbuf.index] = rbuf; //adds the message to its slot in the order in rbuf

      if (j == 0) {globalPassageCount = passageCount;} //put the passage count in the global variable
      sem_post(&globalCurrentPrefix); //increment atomically the number of the passage we're on


      for(int i = 1; i < passageCount; i++){//loop for all responses back for this prefix
        rbuf = getResponse(msqid);//grabs the response
          sem_post(&globalCurrentPassage); //increment atomically the number of the passage we're on
        responses[rbuf.index] = rbuf;//adds the message to its slot in the order in rbuf
      }
      fprintf(stdout,"Report \"%s\"\n", argv[prefixIndexes[j]]); // Report "prefix"
      for (int i = 0; i < passageCount; i++){ //runs for each passage and prints "Passage %d - %s - %s\n" if found, "Passage %d - %s - no word found\n" if not
        if (responses[i].present == 1){//checks if each message struct has a longest Word and displays it if it does
            fprintf(stdout,"Passage %d - %s - %s\n", responses[i].index, responses[i].location_description, responses[i].longest_word);
        }
        else{//No word found
            fprintf(stdout,"Passage %d - %s - no word found\n", responses[i].index, responses[i].location_description);
        }
      }
      //fprintf(stderr,"\n Delaying by %d\n", delay);//spacing out each report at the end for clarity
      if(delay > 0){sleep(delay);}//if we have a delay, wait before sending the next prefix
    }

    sendMessage(1, 0, msqid, "   ");//send empty message to tell PP to quit
    fprintf(stdout,"Exiting ... \n");//Finished up, let the user know
    exit(0);//exit
}

int getMSQID() {
  int msqid;
  key_t key = ftok(CRIMSON_ID,QUEUE_NUMBER); //grab queue key
  int msgflg = IPC_CREAT | 0666;
  if ((msqid = msgget(key, msgflg)) < 0) {//connecting to queue and error message if < 0
      int errnum = errno;
      fprintf(stderr, "Value of errno: %d\n", errno);
      perror("(msgget)");
      fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
  }
  else{//successful connection, success message
      fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);
  }
  return msqid;
}
response_buf getResponse(int msqid) {
  int ret;
  response_buf rbuf;
  do {
    ret = msgrcv(msqid, &rbuf, sizeof(response_buf), 2, 0);//receive type 2 message
    int errnum = errno;
    if (ret < 0 && errno !=EINTR){//Error that is not "no message"
      fprintf(stderr, "Value of errno: %d\n", errno);
      perror("Error printed by perror");
      fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
    }
  } while ((ret < 0 ) && (errno == 4)); //does the above loop while there is no message found

  return rbuf;
}
void sendMessage(int type, int id,int msqid, char* prefix){
  prefix_buf sbuf;
  size_t buf_length;

  sbuf.mtype = type; //this is a type one message
  strlcpy(sbuf.prefix,prefix,WORD_LENGTH); //put the first valid prefix in the struct
  sbuf.id= (id); //id is the number of the message, starting at 1 because zero is for exit message
  buf_length = strlen(sbuf.prefix) + sizeof(int)+1;//struct size without long int type

  if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {//sending message and error if <0
      int errnum = errno;
      fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.prefix, (int)buf_length);
      perror("(msgsnd)");
      fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
      exit(1);
  }
  else{//SUCCESS, PRINT SUCCESS
      fprintf(stdout,"Message(%d): \"%s\" Sent (%d bytes)\n\n", sbuf.id, sbuf.prefix,(int)buf_length);
  }

}
size_t /* O - Length of string */ strlcpy(char*dst /* O - Destination string */, const char *src /* I - Source string */, size_t size /* I - Size of destination string buffer */){
  size_t  srclen;         // Length of source string
  size --;
  srclen = strlen(src);
  if (srclen > size)
      srclen = size;
  memcpy(dst, src, srclen);
  dst[srclen] = '\0';
  return (srclen);
}
void sigIntHandler(int sig_num){
  int sigintCurrentPrefixCount;
  int sigintCurrentPassageCount;
  sem_getvalue(&globalPrefixCount, &sigintCurrentPrefixCount);
  sem_getvalue(&globalPassageCount, &sigintCurrentPassageCount);

  for(int i = 0; i < globalPrefixCount;i++){
    if(sigintCurrentPrefixCount < i ){
      fprintf(stdout,"%s - pending\n" ,globalPrefixArray[i]);
    }
    else if(sigintCurrentPrefixCount == i){
      fprintf(stdout,"%s - %d out of %d\n" ,globalPrefixArray[i],sigintCurrentPassageCount,globalPassageCount);
    }
    else{
      fprintf(stdout,"%s - done\n" ,globalPrefixArray[i]);
    }
  }
  return;
}
