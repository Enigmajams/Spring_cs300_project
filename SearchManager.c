#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "longest_word_search.h"
#include "queue_ids.h"

size_t                        /* O - Length of string */
strlcpy(char       *dst,      /* O - Destination string */
        const char *src,      /* I - Source string */
        size_t      size){     /* I - Size of destination string buffer */

    size_t    srclen;         /* Length of source string */
    //Figure out how much room is needed...
    size --;
    srclen = strlen(src);
    // Copy the appropriate amount...
    if (srclen > size)
        srclen = size;
    memcpy(dst, src, srclen);
    dst[srclen] = '\0';
    return (srclen);
}

int main(int argc, char**argv) //msgsnd
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    prefix_buf sbuf;
    size_t buf_length;
    response_buf rbuf;
    int delay;
    
    int prefixIndexes[argc];//This will store the index in argv[] of each
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
    delay = atoi(argv[1]);


    key = ftok(CRIMSON_ID,QUEUE_NUMBER); //grab queue key
    if ((msqid = msgget(key, msgflg)) < 0) {//connecting to queue and error message if < 0
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else{//success message
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);
    }



    for (int j = 0; j < validPrefixes; j++){//this loop runs for each valid prefix
      sbuf.mtype = 1; //this is a tpye one message
      strlcpy(sbuf.prefix,argv[prefixIndexes[j]],WORD_LENGTH); //put the first valid prefix in the struct
      sbuf.id= (j+1); //id is the number of the message, starting at 1 because zero is for exit message
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


      int passageCount = 1;//this variable gets updated as soon as we receive one message back
/*Hd*/response_buf responses[10];
      for(int i = 0; i < passageCount; i++){//wait for all responses back for this prefix
        int ret;
        do {
          ret = msgrcv(msqid, &rbuf, sizeof(response_buf), 2, 0);//receive type 2 message
          int errnum = errno;
          if (ret < 0 && errno !=EINTR){//Error that is not "no message"
            fprintf(stderr, "Value of errno: %d\n", errno);
            perror("Error printed by perror");
            fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
          }
        } while ((ret < 0 ) && (errno == 4)); //does the above loop while there is no message found

        responses[rbuf.index] = rbuf;//adds the message to its slot in the order in rbuf
        passageCount = rbuf.count;//updates passageCount to the number of messages that PP is sending
      }
      fprintf(stdout,"Report \"%s\"\n", argv[prefixIndexes[j]]); // Report "prefix"
      for (int i = 0; i < passageCount; i++){ //runs for each passage
        if (responses[i].present == 1){//checks if each message struct has a longest Word and displays it if it does
            fprintf(stdout,"Passage %d - %s - %s\n", responses[i].index, responses[i].location_description, responses[i].longest_word);
        }
        else{//No word found
            fprintf(stdout,"Passage %d - %s - no word found\n", responses[i].index, responses[i].location_description);
        }
      }
      fprintf(stdout,"\n Delaying by %d\n", delay);//spacing out each report at the end
      if(delay > 0){sleep(delay);}//if we have a delay, wait before sending the next prefix
      }

      {//send empty message to tell PP to quit
        sbuf.mtype = 1;
        strlcpy(sbuf.prefix,"   ",sizeof("   "));
        sbuf.id=0;
        buf_length = strlen(sbuf.prefix) + sizeof(int)+1;

          if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {//SEND MESSAGE AND CHECK FOR ERROR
            int errnum = errno;
            fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.prefix, (int)buf_length);
            perror("(msgsnd)");
            fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
            exit(1);
        }
        else{//successful message sent, print success message
            fprintf(stdout,"Message(%d): \"%s\" Sent (%d bytes)\n\n", sbuf.id, sbuf.prefix,(int)buf_length);
        }
      }
  fprintf(stdout,"Exiting ... \n");//Finished up, let the user know

  exit(0);//exit
}
