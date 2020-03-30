package edu.cs300;
import CtCILibrary.*;
import java.io.*;
import java.util.ArrayList;
import java.util.concurrent.*;
import java.util.Scanner;


public class ParallelTextSearch{

  public static void main(String[] args){
    try{
        File passages = new File("/home/ecjackson5/Spring_cs300_project/passages.txt");
        Scanner passageNameReader = new Scanner(passages);
        ArrayList<String> fileList = new ArrayList<String>();
        while(passageNameReader.hasNext()){
            fileList.add(passageNameReader.next());
        }
        int numPassages = fileList.size();
        //at this point filelist should contain all the file names
        ArrayList<String[]> passageArrays = new ArrayList<String[]>();
        for(int i=0;i<numPassages;i++){

            File text = new File("/home/ecjackson5/Spring_cs300_project/"+fileList.get(i));
            Scanner sc = new Scanner(text);
            sc.useDelimiter("\\W|(?=\\S*['-])([a-zA-Z'-]+)|(\\b\\w{1,2}\\b)"); //checking regex to obtain the correct delimiter

            ArrayList<String> passageArrayList = new ArrayList<String>();
            while(sc.hasNext()){
                passageArrayList.add(sc.next());
            }

            String[] passageBasicArray = new String[passageArrayList.size()];
            passageBasicArray = passageArrayList.toArray(passageBasicArray);
            passageArrays.add(passageBasicArray);
        }
        //Theoretically the passageArrays arraylist now contains all the file strings
        int treeCount = numPassages;
        ArrayBlockingQueue[] workerQueues = new ArrayBlockingQueue[treeCount];
        ArrayBlockingQueue resultsOutputArray = new ArrayBlockingQueue(treeCount*10);

        for (int i=0;i<treeCount;i++) {
          workerQueues[i] = new ArrayBlockingQueue(10);
        }
        ArrayList<Worker> workers = new ArrayList<Worker>();
        for (int i = 0; i < numPassages; i++){
          workers.add(new Worker(passageArrays.get(i),i,workerQueues[i],resultsOutputArray));
          workers.get(i).start();
        }


        while(true){
          SearchRequest test = new MessageJNI().readPrefixRequestMsg();
          String prefix = test.prefix;
          if (prefix.length() <= 2 ){
            System.out.println("Provide prefix (min 3 characters) for search i.e. con\n");
            System.exit(0);
            }
          if (prefix == "   " ){
            //Exit
            try {
              for (int i = 0; i < numPassages; i++){
                workerQueues[i].put("exit");
              }
                       
            for (Worker worker : workers) {              
                worker.join();
            }
            } catch (InterruptedException e) {}; 
            System.exit(0);
          }


          try {
            for (int i = 0; i < numPassages; i++){
            workerQueues[i].put(prefix);
            }
          } catch (InterruptedException e) {};


          int counter=0;
          while (counter<treeCount){
            try {
              String results = (String)resultsOutputArray.take();
              Scanner resultScanner = new Scanner(results);
                  resultScanner.useDelimiter(":");
              String longestWord = resultScanner.next();
              int threadID = resultScanner.nextInt();
              //System.out.println("results:"+results);
              if (longestWord == "   "){
                new MessageJNI().writeLongestWordResponseMsg(0, prefix, threadID, fileList.get(threadID), longestWord, treeCount, 0);
                System.out.println("Longest Word doesnt exist and here it is = \""+ longestWord +"\"");
              }
              else{
                new MessageJNI().writeLongestWordResponseMsg(0, prefix, threadID, fileList.get(threadID), longestWord, treeCount, 1);
                System.out.println("Longest Word exists and here it is = \""+ longestWord +"\"");
              }
              counter++;
            } catch (InterruptedException e) {};
          }
        }
      }
      catch (IOException e) {
        System.out.println("Error: File not found\n");
        return;
      }
    }
  }
