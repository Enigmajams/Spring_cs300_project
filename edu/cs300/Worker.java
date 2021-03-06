package edu.cs300;
import CtCILibrary.*;
import java.util.concurrent.*;

class Worker extends Thread{

  Trie textTrieTree;
  ArrayBlockingQueue prefixRequestArray;
  ArrayBlockingQueue resultsOutputArray;
  int id;
  int prefixCounter;
  String passageName;

  public Worker(String[] words,int id,ArrayBlockingQueue prefix, ArrayBlockingQueue results){
    this.textTrieTree=new Trie(words);
    this.prefixRequestArray=prefix;
    this.resultsOutputArray=results;
    this.id=id;
    this.passageName="Passage-"+Integer.toString(id)+".txt";//put name of passage here
  }

  public void run() {
    System.out.println("Worker-"+this.id+" ("+this.passageName+") thread started ...");
    prefixCounter = 0;
    while (true){
      try {
        String prefix=(String)this.prefixRequestArray.take();
        if(prefix == "exit"){
          return;
        }
        boolean found = this.textTrieTree.contains(prefix);
        
        if (!found){          
          System.out.println("Worker-"+this.id+" " + (++prefixCounter) + ":"+ prefix+" ==> not found ");
          resultsOutputArray.put("___:"+id);
        } else{
          System.out.println("Worker-"+this.id+" "+ (++prefixCounter) +":"+ prefix+" ==> "+this.textTrieTree.getLongest(prefix));
          resultsOutputArray.put(this.textTrieTree.getLongest(prefix) + ":" + id);
        }
      } catch(InterruptedException e){
        System.out.println(e.getMessage());
      }
    }
  }

}
