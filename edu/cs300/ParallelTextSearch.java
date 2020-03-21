package edu.cs300;
import CtCILibrary.*;
import java.io.*;
import java.util.ArrayList;
import java.util.concurrent.*;
import java.util.Scanner; 

public class ParallelTextSearch{
    
  public static void main(String[] args){

    int treeCount=1;
      
      
    File text = new File("/home/Spring_cs300_project/Peter_Pan.txt");
    Scanner sc = new Scanner(text); 
    ArrayList<String> list = new ArrayList<String>();
    while(sc.hasNextLine()){
        list.add(sc.next());         
    }      
      
    String[] samples = new String[list.size()];
     samples = list.toArray(samples);
      
      
    //{"conspicuous", "parallel", "parachute","coping", "figure", "withering"};
    ArrayBlockingQueue[] workers = new ArrayBlockingQueue[treeCount];
    ArrayBlockingQueue resultsOutputArray=new ArrayBlockingQueue(treeCount*10);

    if (args.length == 0 || args[0].length() <= 2 ){
        System.out.println("Provide prefix (min 3 characters) for search i.e. con\n");
        System.exit(0);
    }

     for (int i=0;i<treeCount;i++) {
       workers[i]=new ArrayBlockingQueue(10);
    }

    new Worker(samples,0,workers[0],resultsOutputArray).start();
    //new Worker(samples[1],1,workers[1],resultsOutputArray).start();

    try {
      workers[0].put(args[0]);
     //workers[1].put(args[0]);
    } catch (InterruptedException e) {};
      
    int counter=0;

    while (counter<treeCount){
      try {
        String results = (String)resultsOutputArray.take();
        System.out.println("results:"+results);
        counter++;
      } catch (InterruptedException e) {};
    }
  }

}
