
#include <stdio.h>
#include <time.h>

//Max number for each object
#define MAXEVENTS 10
#define MAXPROCESS 10
#define MAXMONITORS 10

typedef struct {
  int next;
  Process p;
  int topIndex; //index for the last element in the monitor stack
  int monitors[MAXMONITORS];
  int minstances[MAXMONITORS];//counts instances for each monitor
} ProcessDescriptor;

typedef struct {
  int occupied;
  int lockQueue;
  int signalQueue;

} MonitorDescriptor;

typedef struct{
  int survenu;
  int waitingList;
 } EventDescriptor; 

 //Global variables

 //Pointer to the head of the list of ready processes
 int readyList = -1;
 //list of process descriptors
 ProcessDescriptor processes[MAXPROCESS];
 int nextProcessId = 0;

 //list of event descriptors
 EventDescriptor events[MAXEVENTS];
 int nextEventId = 0;

//list of monitor descriptors
MonitorDescriptor monitors[MAXMONITORS];
int nextMonitorId = 0;

/***********Utility functions for list manipulations******************/

//add element to the tail of the list

void addLast(int* list, int processId){

  if (*list == -1){
    //list is empty
    *list=processId;
  }
  else {
    int temp = *list;
    while (processes[temp].next != -1){
        temp = processes[temp].next;
      }
      processes[temp].next = processId;
      processes[processId].next = -1;
  }
}


//add element to the head of the list

void addFirst(int* list,int processId){
  if (*list == -1){
    *list = processId; 
  }
  else{
    processes[processId].next = *list;
    *list = processId;
  }

}

//remove element that is head of the list
int removeHead(int* list){
  if (*list == -1){
    printf("List is empty!\n");
    return(-1);
  }
  else{
    int head = *list;
    int next = processes[*list].next;
    processes[*list].next = -1;
    *list = next;
    return head;
  }
}

//returns head of the list
int head(int* list){
  if (*list == -1){
    printf("List if empty!\n");
    return(-1);
  }
  else{
    return *list;
  }
 
}

int len(int list){
  int c = 0;
  while(list != -1){
  list = processes[list].next;  
  c++;
  }
  return c;
}
int removeAt(int* list, int procIndex){
  if (procIndex<0 || procIndex>=len(*list)){
    printf("Error: Index out of bounds\n");
    exit(1);
  }
  int prev = -1;
  int curr = *list;
  while (procIndex != 0){
    prev = curr;
    curr = processes[curr].next;
    procIndex --;
  }
  processes[prev].next = processes[curr].next;
  processes[curr].next = -1;
  return curr;
  
}
/*************KERNEL FUNCTIONS**************/


void createProcess (void (*f)(), int stackSize) {
  if (nextProcessId == MAXPROCESS){
      printf("Error: Maximum number of processes reached!\n");
      exit(1);
  }
  
  Process process;
  int* stack = malloc(stackSize);
  process = newProcess(f, stack, stackSize);

  processes[nextProcessId].topIndex = -1;
  processes[nextProcessId].next = -1;
  processes[nextProcessId].p = process;
  int i;
  for (i=0; i < MAXMONITORS; i++){
  processes[nextProcessId].minstances[i] = 0;
  }
  // add process to the list of ready Processes
  addLast(&readyList, nextProcessId);
  nextProcessId++;
  
}

/*************Event functions*****************/
int createEvent(){
  if (nextEventId == MAXEVENTS){
    printf("Error: Maximum number of events reached\n");
    exit(1);
  }
  events[nextEventId].survenu = 0;
  events[nextEventId].waitingList = -1;
  int oldId = nextEventId;
  nextEventId ++;
  return oldId;
}
void attendre(int eventID){
  int pId = head(&readyList);
  if (processes[pId].topIndex != -1){
    printf("Error: It's not allowed to call call attendre inside a monitor\n");
    exit(1);
  }
  if (events[eventID].survenu == 0){
    int pId = removeHead(&readyList);
    addLast(&(events[eventID].waitingList),pId);
    Process process = processes[head(&readyList)].p;
    transfer(process);
  }  
}
void declencher(int eventID){
  events[eventID].survenu = 1;
  while (events[eventID].waitingList != -1){
    int p = removeHead(&(events[eventID].waitingList));
    addLast(&readyList,p);
  }
}
void reinitialiser(int eventID){
  events[eventID].survenu = 0;  
}
/****************Monitor Functions**********************/
int createMonitor(){
  if (nextMonitorId == MAXMONITORS){
    printf("Error: Maximum number of monitors reached\n");
    exit(1);
  }

  monitors[nextMonitorId].occupied = 0;
  monitors[nextMonitorId].lockQueue = -1;
  monitors[nextMonitorId].signalQueue = -1;
  int OldId = nextMonitorId;
  nextMonitorId ++;
  return OldId;
}
void lockMutex(int monitorID){
  if (monitors[monitorID].occupied == 1){
	printf("someone has the lock for MONITOR %d \n",monitorID);
    int pId = removeHead(&readyList);
    addLast(&(monitors[monitorID].lockQueue),pId);
    Process process = processes[head(&readyList)].p;
    transfer(process);
  }
  else{
	printf("free lock FOR MONITOR %d \n",monitorID);
    monitors[monitorID].occupied = 1;
  }
}

void unlockMutex(int monitorID){
  if (monitors[monitorID].lockQueue != -1){
	printf("some process in the lockqueue  for %d monitor\n",monitorID);
    int p = removeHead(&(monitors[monitorID].lockQueue));
    addLast(&readyList,p);
  }
  else{
	printf("no process in the lockqueue  for %d monitor\n",monitorID);
    monitors[monitorID].occupied = 0;
  }
}


void enterMonitor(int monitorID){
  ProcessDescriptor* process = &processes[head(&readyList)];
  if (process->topIndex == MAXMONITORS - 1 ){
    printf("This process is occupying all monitors\n");
    return;
  }
  /*We don't want deadlock*/
  if(process->minstances[monitorID]==0){
    lockMutex(monitorID);
  }
  process->topIndex ++;
  printf("topindex %d  , monitorID %d, pId %d\n",process->topIndex,monitorID,head(&readyList));
  process->monitors[process->topIndex] = monitorID;
  
  
  
}

void exitMonitor(){
  ProcessDescripptor* process = &processes[head(&readyList)];
  int mId = process->monitors[process->topIndex];
  if (process->topIndex >= 0){
    /*Exiting the last instance of this monitor will yield*/
    process->monitors[mId] = -1;
    process->topIndex --;
    if(process->minstances[monitorID] == 1){
      unlockMutex(mId); 
      yield();
    }  
  }
}

void wait(){
  int pId= removeHead(&readyList);
  printf("pId %d working\n",pId);
  ProcessDescriptor process = processes[pId];
  int mId = process.monitors[process.topIndex];
  printf("topindex %d  , monitorID %d\n",process.topIndex,mId);
  addLast(&(monitors[mId].signalQueue),pId);
  printf("topindex %d  , monitorID %d\n",process.topIndex,mId);
  unlockMutex(mId);
  Process next = processes[head(&readyList)].p;
  transfer(next);
}
void notify(){
  ProcessDescriptor notifyingprocess = processes[head(&readyList)];
  int mId = notifyingprocess.monitors[notifyingprocess.topIndex];
  if (monitors[mId].signalQueue != -1){
    int pId = removeHead(&(monitors[mId].signalQueue));
    /*NON-FIFO
    int r = rand() % len(monitors[mId].signalQueue);
    int p = removeAt(&(monitors[mId].signalQueue),r);
    */
	printf("process %d  on buffer %d notified\n",pId,mId);
    addLast(&(monitors[mId].lockQueue),pId);

  }
}
void notifyAll(){
  ProcessDescriptor notifyingprocess = processes[head(&readyList)];
  int mId = notifyingprocess.monitors[notifyingprocess.topIndex];
  while (monitors[mId].signalQueue != -1){
    notify();
  }
}


void yield(){
  int pId = removeHead(&readyList);
  addLast(&readyList, pId);
  Process process = processes[head(&readyList)].p;
  transfer(process);
}

   
void start(){
    
  printf("Starting kernel...\n");
  if (readyList == -1){
      printf("Error: No process in the ready list!\n");
      exit(1);
  }
  Process process = processes[head(&readyList)].p;
  transfer(process);
}    





