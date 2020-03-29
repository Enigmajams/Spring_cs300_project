gcc -std=c99 -D_GNU_SOURCE msgsnd_pr.c -o msgsnd
gcc -std=c99 -D_GNU_SOURCE msgrcv_lwr.c -o msgrcv
javac edu/cs300/*java
javac CtCILibrary/*.java

./msgsnd par
./middle.sh
./msgrcv
./msgrcv
./msgrcv
./msgrcv
./msgrcv
