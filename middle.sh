javac edu/cs300/*.java
javac CtCILibrary/*.java

java -cp . -Djava.library.path=.  edu/cs300/ParallelTextSearch
