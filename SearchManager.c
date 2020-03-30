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
        size_t      size)     /* I - Size of destination string buffer */
{
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


    if (argc <= 1 || strlen(argv[1]) <2) {
        printf("Error: please provide at least one prefix of at least two characters for search\n");
        printf("Usage: %s <prefix>\n",argv[0]);
        exit(-1);
    }//TODO: fix checks for inputs

    key = ftok(CRIMSON_ID,QUEUE_NUMBER);
    if ((msqid = msgget(key, msgflg)) < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

    int validPrefix = argc;

    for (int i = 1; i < validPrefix; i++){
      // We'll send message type 1
      sbuf.mtype = 1;
      strlcpy(sbuf.prefix,argv[i],WORD_LENGTH);
      sbuf.id=0;
      buf_length = strlen(sbuf.prefix) + sizeof(int)+1;//struct size without long int type

      if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {//SEND MESSAGE AND CHECK FOR ERROR
          int errnum = errno;
          fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.prefix, (int)buf_length);
          perror("(msgsnd)");
          fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
          exit(1);
      }
      else{//SUCESS, PRINT SUCESS
          fprintf(stderr,"Message(%d): \"%s\" Sent (%d bytes)\n", sbuf.id, sbuf.prefix,(int)buf_length);
      }



      for(int i = 0; i < 6; i++){//WAIT FOR RESPONSES
        int ret;
        do {
          ret = msgrcv(msqid, &rbuf, sizeof(response_buf), 2, 0);//receive type 2 message
          int errnum = errno;
          if (ret < 0 && errno !=EINTR){
            fprintf(stderr, "Value of errno: %d\n", errno);
            perror("Error printed by perror");
            fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
          }
        } while ((ret < 0 ) && (errno == 4));

        if (rbuf.present == 1){
            fprintf(stderr,"%ld, %d of %d, %s\n", rbuf.passageName, rbuf.index,rbuf.count,rbuf.longest_word);
        }
        else{
            fprintf(stderr,"%ld, %d of %d, not found\n", rbuf.passageName, rbuf.index,rbuf.count);
        }
      }


      }
  //SEND EXIT
  exit(0);
}
