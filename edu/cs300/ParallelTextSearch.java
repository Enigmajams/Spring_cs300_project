package edu.cs300;
import CtCILibrary.*;
import java.io.*;
import java.util.ArrayList;
import java.util.concurrent.*;
import java.util.Scanner;

public class ParallelTextSearch{
  public static void main(String[] args){ 
        ArrayList<String> fileList = new ArrayList<String>(); 
        try{
        File passages = new File("/home/ecjackson5/Spring_cs300_project/passages.txt");
        Scanner passageNameReader = new Scanner(passages);        
        while(passageNameReader.hasNext()){ //for each passage name in passage text add it to the file list
            fileList.add(passageNameReader.next());
        }
        }catch(IOException e){
          System.out.println("Error, Passage.txt unable to be opened. Exiting ... \n");
          return;
        }
        //at this point filelist should contain all the file names
        int numPassages = fileList.size(); //numpassages will contain the total number of file names
        int numValidPassages = numPassages; //numValidPassages will contain the number of passages that we can open

        ArrayList<String[]> passageArrays = new ArrayList<String[]>();//declares an arraylist of string arrays to store the string arrays take from each file
        for(int i=0;i<numPassages;i++){ //loop for each file name
            try{
            File text = new File("/home/ecjackson5/Spring_cs300_project/"+fileList.get(i)); //for each file name, open it
            Scanner sc = new Scanner(text); //make a scanner on that file
            sc.useDelimiter("\\W|(?=\\S*['-])([a-zA-Z'-]+)|(\\b\\w{1,2}\\b)"); //delimiter is, anything not a word, any word with ' or - in it, and any word less than 3 characters
            ArrayList<String> passageArrayList = new ArrayList<String>(); //make an arraylist for each string you read in
            while(sc.hasNext()){ //loop until there are no more valid tokens
                passageArrayList.add(sc.next()); //add a token to the arraylist
            }

            String[] passageBasicArray = new String[passageArrayList.size()];
            passageBasicArray = passageArrayList.toArray(passageBasicArray);
            passageArrays.add(passageBasicArray);
            }catch(IOException e){
              System.out.println("Error, File unable to be opened. Ignoring ... \n");
              numValidPassages--; // reduce the number of passages we can read in from
              fileList.remove(i); //remove the file name
              continue; //skip the rest of the for loop
            }
        }
        //Theoretically the passageArrays arraylist now contains all the file name strings
        int treeCount = numValidPassages; //sets the number of workers to make
        ArrayBlockingQueue[] workerQueues = new ArrayBlockingQueue[treeCount]; //makes an array of queues for each worker
        ArrayBlockingQueue resultsOutputArray = new ArrayBlockingQueue(treeCount*10); // makes a queue for returning values

        for (int i=0;i<treeCount;i++) {//for each worker, assigns a queue to the array
          workerQueues[i] = new ArrayBlockingQueue(10); //assigns queues to array slots
        }
        ArrayList<Worker> workers = new ArrayList<Worker>(); //makes an arraylist of type worker to reference later when we need to start and exit
        for (int i = 0; i < numPassages; i++){ //for each worker, intialize and start the thread
          workers.add(new Worker(passageArrays.get(i),i,workerQueues[i],resultsOutputArray));//intialize a worker and put it in the array list
          workers.get(i).start();//start a worker
        }

        while(true){ //loop until we recieve a message of "   "
          SearchRequest prefixRequest = new MessageJNI().readPrefixRequestMsg(); //grab a new message
          String prefix =prefixRequest.prefix; //grab the prefix from the message struct
          int prefixID = prefixRequest.requestID; //grab the prefix ID fromn the struct
          System.out.println("**Prefix("+ prefixID +") " + prefix + " recieved"); //print out that you've recieved a prefix
          if (prefix.equals("   ")){ //if we got our exit message, tell all threads to quit, then join them, then exit
            try {
              for (int i = 0; i < numPassages; i++){
                workerQueues[i].put("exit");
              }

            for (Worker worker : workers) {
                worker.join();
            }
            } catch (InterruptedException e) {};
            System.out.println("Terminating ... \n");
            System.exit(0);
          }
          //otherwise process the prefix
          try {
          for (int i = 0; i < numPassages; i++){ //for each passage, dump the current prefix in their queue
            workerQueues[i].put(prefix); //put a prefix in a workes queue
          }
          } catch (InterruptedException e) {};

          for(int counter=0;counter<treeCount;counter++){ //this loop runs until we get all the messages from all the workers on the current prefix and sends each message as we get it
            try {
              String results = (String)resultsOutputArray.take(); //pull a result off the queue
              Scanner resultScanner = new Scanner(results); //we use a scanner to interpret the results
                  resultScanner.useDelimiter(":"); //using a : as a delimiter instead of whitespace to avoid any weird whitespace issues
              String longestWord = resultScanner.next(); //grab the longestWord
              int threadID = resultScanner.nextInt(); //grab the threads ID
              if (longestWord.equals("___")){ //if there is no word with that prefix, we send a message back saying there isnt one (present = 0)
                new MessageJNI().writeLongestWordResponseMsg(prefixID, prefix, threadID, fileList.get(threadID), longestWord, treeCount, 0);
              }
              else{ //otherwise we send back the longest word, with present = 1
                new MessageJNI().writeLongestWordResponseMsg(prefixID, prefix, threadID, fileList.get(threadID), longestWord, treeCount, 1);
              }
            } catch (InterruptedException e) {};
          }
        }
     }
  }
