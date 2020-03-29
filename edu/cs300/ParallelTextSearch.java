package edu.cs300;
import CtCILibrary.*;
import java.io.*;
import java.util.ArrayList;
import java.util.concurrent.*;
import java.util.Scanner; 


public class ParallelTextSearch{
    
  public static void main(String[] args){
           
        SearchRequest test = new MessageJNI().readPrefixRequestMsg();
        // System.out.println("message: " + test.prefix + " \n");
        String prefix = test.prefix;
      if (prefix.length() <= 2 ){
        System.out.println("Provide prefix (min 3 characters) for search i.e. con\n");
        System.exit(0);
    }
      
      
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
        
        /*File text = new File("/home/ecjackson5/Spring_cs300_project/Peter_Pan.txt");
        Scanner sc = new Scanner(text); 
        sc.useDelimiter("\\W|(?=\\S*['-])([a-zA-Z'-]+)"); //checking regex to obtain the correct delimiter

        ArrayList<String> list = new ArrayList<String>();
        while(sc.hasNext()){
            list.add(sc.next());         
        }      
      
        String[] samples = new String[list.size()];
        samples = list.toArray(samples);
      */
                  
   int treeCount = numPassages;
        
    ArrayBlockingQueue[] workers = new ArrayBlockingQueue[treeCount];
    ArrayBlockingQueue resultsOutputArray=new ArrayBlockingQueue(treeCount*10);

    

     for (int i=0;i<treeCount;i++) {
       workers[i]=new ArrayBlockingQueue(10);
    }
    
    for (int i = 0; i < numPassages; i++){     
    new Worker(passageArrays.get(i),i,workers[i],resultsOutputArray).start();
    //System.out.println("Created worker " + i + "\n");
    }
    try {
      for (int i = 0; i < numPassages; i++){  
      workers[i].put(prefix);
      }
    } catch (InterruptedException e) {};
      
    int counter=0;

    while (counter<treeCount){
      try {
        String results = (String)resultsOutputArray.take();
        System.out.println("results:"+results);
        counter++;
          if (results == "   ")
          MessageJNI message =  new MessageJNI().writeLongestWordResponseMsg(0, prefix, counter, "/home/ecjackson5/Spring_cs300_project/"+fileList.get(counter), results, 5, 0);          
          else 
          MessageJNI message =  new MessageJNI().writeLongestWordResponseMsg(0, prefix, counter, "/home/ecjackson5/Spring_cs300_project/"+fileList.get(counter), results, 5, 1);          
      } catch (InterruptedException e) {};
    }
  }

catch (IOException e) {
    System.out.println("Error: File not found\n");
      return;
      }
  }
}
