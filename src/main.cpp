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

bool autoregulate = true;

int numberOfDaysToSimulate = 28;

// Global objects
Facility  User("User");

Histogram PostTable("Length of Posts",5,1,25);
Histogram AddTable("Length of Adds",10,1,20);

Histogram PostsPerDay("Posts per day", 0,1,numberOfDaysToSimulate);
Histogram AddsPerDay("Adds per day", 0,1,numberOfDaysToSimulate);

Histogram PostsPerHour("Posts per hour", 0,1,numberOfDaysToSimulate*24);
Histogram AddsPerHour("Adds per hour", 0,1,numberOfDaysToSimulate*24);

Histogram MoreThan5AddsPerDay("More than 5 adds per day", 0,1,numberOfDaysToSimulate);
Histogram MoreThan10AddsPerDay("More than 10 adds per day", 0,1,numberOfDaysToSimulate);

Stat addArrivalTimeStat("Add arrival time");

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

string parseTime(int time) {
  int day = time / (24*60*60);
  int hours = (time % (24*60*60)) / 3600;
  int minutes = (time % 3600) / 60;
  int seconds = time % 60;
  return to_string(day) + "d " + to_string(hours) + "h " + to_string(minutes) + "m " + to_string(seconds) + "s";
}

int getDayFromTime(int time) {
  int day = time / (24*60*60);
  return day;
}

int getHourFromTime(int time) {
  int hour = time / (60*60);
  return hour;
}

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
    PostsPerDay(getDayFromTime(Time));
    PostsPerHour(getHourFromTime(Time));
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

    if (addCount6 == 4) {
      if (autoregulate) {
        addArrivalTime = addArrivalTime * 1.2;
        addArrivalTimeStat(addArrivalTime);
      }
    } else if (addCount6 > 5) {
      addCount6 = 0;
      addFatigue6Vector.push_back(Time);
      MoreThan5AddsPerDay(getDayFromTime(Time));
    }

    if (addCount11 == 9) {
      if (autoregulate) {
        addArrivalTime = addArrivalTime * 1.2;
        addArrivalTimeStat(addArrivalTime);
      }
    } else if (addCount11 > 10) {
      addCount11 = 0;
      addFatigue11Vector.push_back(Time);
      MoreThan10AddsPerDay(getDayFromTime(Time));
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
    AddsPerDay(getDayFromTime(Time));
    AddsPerHour(getHourFromTime(Time));
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

// periodicaly decrease fatigue
class AddFatigue6Digester : public Event {
  void Behavior() {
    if (addCount6 > 0) {
      addCount6--;
    }
    AddFatigue6Digester::Activate(Time+(24*60*60/6));
  }
};

// periodicaly decrease fatigue
class AddFatigue11Digester : public Event {
  void Behavior() {
    if (addCount11 > 0) {
      addCount11--;
    }
    AddFatigue11Digester::Activate(Time+(24*60*60/11));
  }
};

//make statistics about day
class DayStatistics : public Event {
  void Behavior() {
    if (autoregulate) {
      addArrivalTime = addArrivalTime * 0.99;
      addArrivalTimeStat(addArrivalTime);
    }
    DayStatistics::Activate(Time+(24*60*60*2));
  }
};




void makeTest(
  string testOutput,
  double postArrivalTime,
  double addArrivalTime,
  double attentionSpan,
  double lengthOfPostLow,
  double lengthOfPostHigh,
  double lengthOfAddLow,
  double lengthOfAddHigh,
  bool autoregulate,
  int numberOfDaysToSimulate = 28) {
  
  // set inputs
  ::postArrivalTime = postArrivalTime;
  ::addArrivalTime = addArrivalTime;
  ::attentionSpan = attentionSpan;
  ::lengthOfPostLow = lengthOfPostLow;
  ::lengthOfPostHigh = lengthOfPostHigh;
  ::lengthOfAddLow = lengthOfAddLow;
  ::lengthOfAddHigh = lengthOfAddHigh;
  ::autoregulate = autoregulate;
  addArrivalTimeStat(addArrivalTime);

  // set output
  SetOutput(testOutput.c_str());

  // print model description
  Print(" Model simulating user activity on social media, keeping track of how many adds user sees in comparison to posts.\n");
  Print(" Model inputs:\n");
  Print(" - postArrivalTime: %f\n", postArrivalTime);
  Print(" - addArrivalTime: %f\n", addArrivalTime);
  Print(" - attentionSpan: %f\n", attentionSpan);
  Print(" - lengthOfPostLow: %f\n", lengthOfPostLow);
  Print(" - lengthOfPostHigh: %f\n", lengthOfPostHigh);
  Print(" - lengthOfAddLow: %f\n", lengthOfAddLow);
  Print(" - lengthOfAddHigh: %f\n", lengthOfAddHigh);
  Print(" - autoregulate: %d\n", autoregulate);
 
  // run simulation
  Init(0,numberOfDaysToSimulate*24*60*60);
  User.Clear();
  (new PostGenerator)->Activate();
  (new AddGenerator)->Activate();
  (new AddFatigue6Digester)->Activate(24*60*60/6);
  (new AddFatigue11Digester)->Activate(24*60*60/11);
  (new DayStatistics)->Activate(24*60*60);
  Run();

  // print statistics
  Print("\nPost saw: %d\n", postCount);
  Print("Relevant posts: %d\n", numberOfRelevantPosts);
  Print("Irrelevant posts: %d\n", numberOfIrrelevantPosts);
  Print("Skipped posts: %d\n", numberOfSkippedPosts);

  Print("\nAdds saw: %d\n", addCount);
  Print("Relevant adds: %d\n", numberOfRelevantAdds);
  Print("Irrelevant adds: %d\n", numberOfIrrelevantAdds);
  Print("Skipped adds: %d\n", numberOfSkippedAdds);

  Print("\nUser saw more than 5 adds in a day: %d\n", addFatigue6Vector.size());
  Print("User saw more than 10 adds in a day: %d\n", addFatigue11Vector.size());

  PostTable.Output();
  AddTable.Output();
  PostsPerDay.Output();
  PostsPerHour.Output();
  AddsPerDay.Output();
  AddsPerHour.Output();
  MoreThan5AddsPerDay.Output();
  MoreThan10AddsPerDay.Output();
  addArrivalTimeStat.Output();

  // clear statistics
  postCount = 0;
  addCount = 0;
  addCount6 = 0;
  addFatigue6Vector = vector<int>();
  addCount11 = 0;
  addFatigue11Vector = vector<int>();
  numberOfRelevantPosts = 0;
  numberOfRelevantAdds = 0;
  numberOfIrrelevantPosts = 0;
  numberOfIrrelevantAdds = 0;
  numberOfSkippedPosts = 0;
  numberOfSkippedAdds = 0;

  PostTable.Clear();
  AddTable.Clear();
  PostsPerDay.Clear();
  PostsPerHour.Clear();
  AddsPerDay.Clear();
  AddsPerHour.Clear();
  MoreThan5AddsPerDay.Clear();
  MoreThan10AddsPerDay.Clear();
  addArrivalTimeStat.Clear();
}

int main() {
  if (system("test -d tests") != 0) {
    if (system("mkdir tests")) {
      printf("Error creating tests folder\n");
      return 1;
    }
  }

  printf("test\n");
  makeTest("tests/test.out", 10, 1000, 40, 5, 30, 10, 30, false);
  printf("test-autoregulate\n");
  makeTest("tests/test-autoregulate.out", 10, 1000, 40, 5, 30, 10, 30, true);

  printf("test-small-attention-span\n");
  makeTest("tests/test-small-attention-span.out", 10, 1000, 10, 5, 30, 10, 30, false);
  printf("test-small-attention-span-autoregulate\n");
  makeTest("tests/test-small-attention-span-autoregulate.out", 10, 1000, 10, 5, 30, 10, 30, true);

  printf("test-longer-simulation-time-autoregulate\n");
  makeTest("tests/test-longer-simulation-time-autoregulate.out", 10, 1000, 40, 5, 30, 10, 30, true, 28*5);
}

