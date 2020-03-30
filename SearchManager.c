#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
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

    int prefixIndexes[argc];//This will store the index in argv[] of each
    int validPrefixes = 0;
    if (argc <= 1 ) {//Error out because no prefix
        fprintf(stderr,"Error: please provide at least one prefix of at least two characters for search\n");
        fprintf(stderr,"Usage: %s <prefix>\n",argv[0]);
        exit(-1);
    }
    for(int i = 1; i < argc; i++){//if prefixes are invalid ignore them
      if (strlen(argv[i]) <=2){//Prefix too short
        fprintf(stderr,"\"%s\" is too short to be a valid prefix.\nIgnoring ... \n\n",argv[i]);
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
      strlcpy(sbuf.prefix,argv[validPrefixes[j]],WORD_LENGTH); //put the first valid prefix in the struct
      sbuf.id=0; //id is zero cause its an actual message
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
      fprintf(stdout,"Report \"%s\"\n", argv[validPrefixes[j]]); // Report "prefix"
      for (int i = 0; i < passageCount; i++){ //runs for each passage
        if (responses[i].present == 1){//checks if each message struct has a longest Word and displays it if it does
            //fprintf(stderr,"%s, %d of %d, %s\n", responses[i].location_description, responses[i].index,responses[i].count,responses[i].longest_word);
            fprintf(stdout,"Passage %d - %s - %s\n", responses[i].index, responses[i].location_description, responses[i].longest_word);
        }
        else{//No word found
            //fprintf(stderr,"%s, %d of %d, not found\n", responses[i].location_description, responses[i].index,responses[i].count);
            fprintf(stdout,"Passage %d - %s - no word found\n", responses[i].index, responses[i].location_description);
        }
      }
      fprintf(stdout,"\n");//spacing out each report at the end
      }

      {//send empty message to tell PP to quit
        sbuf.mtype = 1;
        strlcpy(sbuf.prefix,"   ",sizeof("   "));
        sbuf.id=1;
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
  fprintf(stdout," Exiting ... \n");//Finished up, let the user know

  exit(0);//exit
}
