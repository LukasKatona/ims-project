#include "simlib.h"
#include <string.h>

// Model inputs
double postArrivalTime = 10;
double addArrivalTime = 100;
double attentionSpan = 40;
double lengthOfPost = 20;


// Global objects
Facility  User("User");
Histogram PostTable("Table of time posts spent in system",0,25,20);
Histogram AddTable("Table of time adds spent in system",0,25,20);

// Variables
int postCount = 0;

int addCount = 0;
int addCount6 = 0;
int addFatigue6 = 0;

int addCount11 = 0;
int addFatigue11 = 0;

class Post : public Process {
  double ArrivalTime;
  void Behavior() {
    Seize(User);
    ArrivalTime = Time;    
    
    postCount++;

    // TODO add logic 
    Wait(40);   
    
    Release(User);
    PostTable(Time-ArrivalTime);
  }
};

class Add : public Process {
  double ArrivalTime;
  void Behavior() {
    Seize(User);
    ArrivalTime = Time;    
    
    addCount++;
    addCount6++;
    addCount11++;

    if (addCount6 > 5) {
      addCount6 = 0;
      addFatigue6++;
    }

    if (addCount11 > 10) {
      addCount11 = 0;
      addFatigue11++;
    }

    Print("Add seeing\n");

    // TODO add logic 
    Wait(20);   
    
    Release(User);
    AddTable(Time-ArrivalTime);
  }
};

class PostGenerator : public Event {
  void Behavior() {
    (new Post)->Activate();
    Activate(Time+Exponential(postArrivalTime));
  }
};

class AddGenerator : public Event {
  void Behavior() {
    (new Add)->Activate();
    Activate(Time+Exponential(addArrivalTime));
  }
};

// periiodicaly decrease fatigue
class AddFatigue6Digester : public Event {
  void Behavior() {
    if (addCount6 > 0) {
      addCount6--;
      Print("Decrease fatigue 6\n");
    }
    AddFatigue6Digester::Activate(Time+(24*60*60/6));
  }
};

// periiodicaly decrease fatigue
class AddFatigue11Digester : public Event {
  void Behavior() {
    if (addCount11 > 0) {
      addCount11--;
      Print("Decrease fatigue 11\n");   
    }
    AddFatigue11Digester::Activate(Time+(24*60*60/11));
  }
};

int main() {
  SetOutput("main.out");
  Print(" Model simulating user activity on social media, keeping track of how many adds user sees in comparison to posts.\n");
  Init(0,24*60*60);
  (new PostGenerator)->Activate();
  (new AddGenerator)->Activate();
  (new AddFatigue6Digester)->Activate(24*60*60/6);
  (new AddFatigue11Digester)->Activate(24*60*60/11);
  Run();
  User.Output();
  PostTable.Output();
  AddTable.Output();
  SIMLIB_statistics.Output();

  Print("Post saw: %d\n", postCount);
  Print("Add user saw: %d\n", addCount);
  Print("Fatigue 6: %d\n", addFatigue6);
  Print("Fatigue 11: %d\n", addFatigue11);
}

