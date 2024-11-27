#include "simlib.h"
#include <string.h>
#include <vector>

using namespace std;

// Model inputs
double postArrivalTime = 10;
double addArrivalTime = 1000;
double attentionSpan = 40;

double lengthOfPostLow = 5;
double lengthOfPostHigh = 30;

double lengthOfAddLow = 10;
double lengthOfAddHigh = 30;


// Global objects
Facility  User("User");
Histogram PostTable("Table of time posts spent in system",5,1,25);
Histogram AddTable("Table of time adds spent in system",10,1,20);

// Variables
int postCount = 0;
int addCount = 0;

int addCount6 = 0;
vector<int> addFatigue6Vector;

int addCount11 = 0;
vector<int> addFatigue11Vector;

int numberOfRelevantPosts = 0;
int numberOfRelevantAdds = 0;

int numberOfIrrelevantPosts = 0;
int numberOfIrrelevantAdds = 0;

int numberOfSkippedPosts = 0;
int numberOfSkippedAdds = 0;

class Post : public Process {
  public:
    Post(int lengthOfPost) : Process() {
      this->lengthOfPost = lengthOfPost;
    }
    int lengthOfPost;

  double ArrivalTime;
  void Behavior() {
    Seize(User);
    ArrivalTime = Time;    
    
    postCount++;

    double currentAttentionSpan = Exponential(attentionSpan);
    if (currentAttentionSpan < lengthOfPost) {
      Wait(currentAttentionSpan);
      numberOfSkippedPosts++;
    } else {
      Wait(lengthOfPost);
      double probabilityOfGoodPost = Uniform(0,1);
      if (probabilityOfGoodPost > 0.44) {
        numberOfRelevantPosts++;
      } else {
        numberOfIrrelevantPosts++;
      }
    }
       
    Release(User);
    PostTable(Time-ArrivalTime);
  }
};

class Add : public Process {
  public:
    Add(int lengthOfAdd) : Process() {
      this->lengthOfAdd = lengthOfAdd;
    }
    int lengthOfAdd;

  double ArrivalTime;
  void Behavior() {
    Seize(User);
    ArrivalTime = Time;    
    
    addCount++;
    addCount6++;
    addCount11++;

    if (addCount6 > 5) {
      addCount6 = 0;
      addFatigue6Vector.push_back(Time);
    }

    if (addCount11 > 10) {
      addCount11 = 0;
      addFatigue11Vector.push_back(Time);
    }

    double currentAttentionSpan = Exponential(attentionSpan);
    if (currentAttentionSpan < lengthOfAdd) {
      Wait(currentAttentionSpan);
      numberOfSkippedAdds++;
    } else {
      Wait(lengthOfAdd);
      double probabilityOfGoodAdd = Uniform(0,1);
      if (probabilityOfGoodAdd > 0.44) {
        numberOfRelevantAdds++;
      } else {
        numberOfIrrelevantAdds++;
      }
    }
    
    Release(User);
    AddTable(Time-ArrivalTime);
  }
};

class PostGenerator : public Event {
  void Behavior() {
    (new Post(Uniform(lengthOfPostLow, lengthOfPostHigh)))->Activate();
    Activate(Time+Exponential(postArrivalTime));
  }
};

class AddGenerator : public Event {
  void Behavior() {
    (new Add(Uniform(lengthOfAddLow, lengthOfAddHigh)))->Activate();
    Activate(Time+Exponential(addArrivalTime));
  }
};

// periiodicaly decrease fatigue
class AddFatigue6Digester : public Event {
  void Behavior() {
    if (addCount6 > 0) {
      addCount6--;
    }
    AddFatigue6Digester::Activate(Time+(24*60*60/6));
  }
};

// periiodicaly decrease fatigue
class AddFatigue11Digester : public Event {
  void Behavior() {
    if (addCount11 > 0) {
      addCount11--;
    }
    AddFatigue11Digester::Activate(Time+(24*60*60/11));
  }
};

string parseTime(int time) {
  int hours = time / 3600;
  int minutes = (time % 3600) / 60;
  int seconds = time % 60;
  return to_string(hours) + ":" + to_string(minutes) + ":" + to_string(seconds);
}

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
  Print("Relevant posts: %d\n", numberOfRelevantPosts);
  Print("Irrelevant posts: %d\n", numberOfIrrelevantPosts);
  Print("Skipped posts: %d\n", numberOfSkippedPosts);


  Print("Adds saw: %d\n", addCount);
  Print("Relevant adds: %d\n", numberOfRelevantAdds);
  Print("Irrelevant adds: %d\n", numberOfIrrelevantAdds);
  Print("Skipped adds: %d\n", numberOfSkippedAdds);

  Print("User saw more than 5 adds in a day: %d\n", addFatigue6Vector.size());
  for (int i = 0; i < addFatigue6Vector.size(); i++) {
    Print("Time: %s\n", parseTime(addFatigue6Vector[i]).c_str());
  }

  Print("User saw more than 10 adds in a day: %d\n", addFatigue11Vector.size());
  for (int i = 0; i < addFatigue11Vector.size(); i++) {
    Print("Time: %s\n", parseTime(addFatigue11Vector[i]).c_str());
  }
}

