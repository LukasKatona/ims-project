#include "simlib.h"
#include <string.h>
#include <vector>

using namespace std;

bool debugPrints = true;
const int DEFAULT_DAYS_TO_SIMULATE = 28;

// Model inputs
double postArrivalTime = 10;
double addArrivalTime = 1000;
double attentionSpan = 40;

double lengthOfPostLow = 5;
double lengthOfPostHigh = 30;

double lengthOfAddLow = 10;
double lengthOfAddHigh = 30;

bool autoregulate = true;

int numberOfDaysToSimulate = DEFAULT_DAYS_TO_SIMULATE;

// Global objects
Facility User("User");

Histogram PostTable("Length of Posts", 5, 1, 25);
Histogram AddTable("Length of Adds", 10, 1, 20);

Histogram PostsPerDay("Posts seen per day", 0,1,numberOfDaysToSimulate);
Histogram AddsPerDay("Adds seen per day", 0,1,numberOfDaysToSimulate);

Histogram PostsPerHour("Posts seen per hour", 0,1,numberOfDaysToSimulate*24);
Histogram AddsPerHour("Adds seen per hour", 0,1,numberOfDaysToSimulate*24);

Stat addArrivalTimeStat("Add arrival time");

Stat PostsPerScrolling("Number of posts viewed per scrolling phase");
Stat AdsPerScrolling("Number of ads viewed per scrolling phase");

// Variables
int postCount = 0;
int addCount = 0;

int addCount6 = 0;

int numberOfRelevantPosts = 0;
int numberOfRelevantAdds = 0;

int numberOfIrrelevantPosts = 0;
int numberOfIrrelevantAdds = 0;

int numberOfSkippedPosts = 0;
int numberOfSkippedAdds = 0;

double AddArrivalTimeUpScale = 1.6;
double AddArrivalTimeDownScale = 0.9;
int autoregulateThreshold = 4;

int productiveLow = 30 * 60;
int productiveHigh = 45 * 60;
int scrollingLow = 5 * 60;
int scrollingHigh = 8 * 60;
int timeUntilWakeUp;

string parseTime(int time)
{
  int day = time / (24 * 60 * 60);
  int hours = (time % (24 * 60 * 60)) / 3600;
  int minutes = (time % 3600) / 60;
  int seconds = time % 60;
  return to_string(day) + "d " + to_string(hours) + "h " + to_string(minutes) + "m " + to_string(seconds) + "s";
}

string parseShortTime(int time)
{
  int day = time / (24 * 60 * 60);
  int hours = (time % (24 * 60 * 60)) / 3600;
  int minutes = (time % 3600) / 60;
  int seconds = time % 60;
  return to_string(minutes) + "m " + to_string(seconds) + "s";
}

int getDayFromTime(int time)
{
  int day = time / (24 * 60 * 60);
  return day;
}

int getHourFromTime(int time)
{
  int hour = time / (60 * 60);
  return hour;
}

// periodicaly decrease fatigue
class AddFatigue6Digester : public Event {
  void Behavior() {
    if (addCount6 > 0) {
      addCount6--;
    }
  }
};

class Post : public Process
{
public:
  Post(int lengthOfPost) : Process()
  {
    this->lengthOfPost = lengthOfPost;
  }
  int lengthOfPost;

  void Behavior()
  {
    Seize(User); // Attempt to seize the User facility (queue if busy)

    postCount++;

    double currentAttentionSpan = Normal(attentionSpan, 5);
    if (currentAttentionSpan < lengthOfPost)
    {
      Wait(currentAttentionSpan);
      numberOfSkippedPosts++;
    }
    else
    {
      Wait(lengthOfPost);
      double probabilityOfGoodPost = Uniform(0, 1);
      if (probabilityOfGoodPost > 0.44)
      {
        numberOfRelevantPosts++;
      }
      else
      {
        numberOfIrrelevantPosts++;
      }
    }

    Release(User);
    PostTable(lengthOfPost);
    PostsPerDay(getDayFromTime(Time));
    PostsPerHour(getHourFromTime(Time));
  }
};

class Add : public Process
{
public:
  Add(int lengthOfAdd) : Process()
  {
    this->lengthOfAdd = lengthOfAdd;
  }
  int lengthOfAdd;

  void Behavior()
  {
    Seize(User);

    addCount++;
    addCount6++;
    (new AddFatigue6Digester)->Activate(Time + (24 * 60 * 60 / 6));

    if (addCount6 == autoregulateThreshold) {
      if (autoregulate) {
        addArrivalTime = addArrivalTime * AddArrivalTimeUpScale;
        addArrivalTimeStat(addArrivalTime);
      }
      addCount6 = 0;
    }

    double currentAttentionSpan = Normal(attentionSpan, 5);
    if (currentAttentionSpan < lengthOfAdd)
    {
      Wait(currentAttentionSpan);
      numberOfSkippedAdds++;
    }
    else
    {
      Wait(lengthOfAdd);
      double probabilityOfGoodAdd = Uniform(0, 1);
      if (probabilityOfGoodAdd > 0.44)
      {
        numberOfRelevantAdds++;
      }
      else
      {
        numberOfIrrelevantAdds++;
      }
    }

    Release(User);
    AddTable(lengthOfAdd);
    AddsPerDay(getDayFromTime(Time));
    AddsPerHour(getHourFromTime(Time));
  }
};

class PostGenerator : public Event
{
  void Behavior()
  {
    (new Post(Uniform(lengthOfPostLow, lengthOfPostHigh)))->Activate();
    Activate(Time + Exponential(postArrivalTime));
  }
};

class AddGenerator : public Event
{
  void Behavior()
  {
    (new Add(Uniform(lengthOfAddLow, lengthOfAddHigh)))->Activate();
    Activate(Time + addArrivalTime);
  }
};

// make statistics about day
class DayStatistics : public Event
{
  void Behavior()
  {
    if (autoregulate)
    {
      addArrivalTime = addArrivalTime * AddArrivalTimeDownScale;
      addArrivalTimeStat(addArrivalTime);
    }
    DayStatistics::Activate(Time + (24 * 60 * 60));
  }
};

class UserActivityManager : public Process
{
  public:
    UserActivityManager() : Process(1) {}
    
  void Behavior() override
  {
    double startTime = Time;
    double duration = 0.0;

    while (1)
    {
      Seize(User);
      if (debugPrints)
      {
        Print("User is productive at ");
        Print((parseTime(Time) + "\n").c_str());
      }
      Wait(Uniform(productiveLow, productiveHigh)); // 30-45 minutes of productive activity

      duration = Time - startTime;
      if (debugPrints)
        Print(("Productive time duration: " + parseShortTime(duration) + "\n").c_str());
      startTime = Time;
      Release(User);

      if (debugPrints)
      {
        Print("User start scrolling at ");
        Print((parseTime(Time) + "\n").c_str());
      }
      Wait(Uniform(scrollingLow, scrollingHigh)); // 5-8 minutes of scrolling time

      duration = Time - startTime;
      if (debugPrints)
        Print(("Scrolling time duration: " + parseShortTime(duration) + "\n").c_str());
      startTime = Time;
    }
  }
};

class DayPhaseManager : public Process
{
  public:
    DayPhaseManager() : Process(2) {}
  void Behavior() override
  {
    (new UserActivityManager)->Activate();
    while (true)
    {
      // Morning Phase
      if (debugPrints)
      {
        Print("----------------------------------------\n");
        Print((parseTime(Time) + "\n").c_str());
        Print("Starting Morning Phase\n");
      }

      productiveHigh = 45 * 60;
      productiveLow = 30 * 60;
      scrollingLow = 5 * 60;
      scrollingHigh = 8 * 60;
      Wait(Uniform(2 * 60 * 60, 4 * 60 * 60)); // Morning phase lasts for 2-4 hours

      // Midday Phase
      if (debugPrints)
      {
        Print("----------------------------------------\n");
        Print((parseTime(Time) + "\n").c_str());
        Print("Starting Midday Phase\n");
      }
      productiveHigh = 15 * 60;
      productiveLow = 10 * 60;
      scrollingLow = 30 * 60;
      scrollingHigh = 45 * 60;
      Wait(Uniform(2 * 60 * 60, 4 * 60 * 60)); // Midday phase lasts for 2-4 hours

      // Afternoon Phase
      if (debugPrints)
      {
        Print("----------------------------------------\n");
        Print((parseTime(Time) + "\n").c_str());
        Print("Starting Afternoon Phase\n");
      }
      productiveHigh = 45 * 60;
      productiveLow = 30 * 60;
      scrollingLow = 5 * 60;
      scrollingHigh = 8 * 60;
      Wait(Uniform(8 * 60 * 60, 10 * 60 * 60)); // Afternoon phase lasts for 8-10 hours

      // Night Phase
      if (debugPrints)
      {
        Print("----------------------------------------\n");
        Print((parseTime(Time) + "\n").c_str());
        Print("Starting Night Phase\n");
      }

      Seize(User);
      int currentTime = (int)Time % (24 * 60 * 60);
      if (debugPrints)
        Print(("Current time: " + parseTime(currentTime) + "\n").c_str());
      int wakeUpTime = 6 * 60 * 60; // 6:00 AM

      if (currentTime < wakeUpTime)
      {
        timeUntilWakeUp = wakeUpTime - currentTime;
      }
      else
      {
        timeUntilWakeUp = (24 * 60 * 60) - currentTime + wakeUpTime;
      }

      if (debugPrints)
        Print(("User will wake up in " + parseTime(timeUntilWakeUp) + "\n").c_str());
      Wait(timeUntilWakeUp);
      Release(User);

      if (debugPrints)
        Print("User wakes up at 6:00 AM\n");
    }
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
  int numberOfDaysToSimulate = DEFAULT_DAYS_TO_SIMULATE) {
    
  // set output file
  if (system(("test -d tests/" + testOutput).c_str()) != 0) {
    if (system(("mkdir tests/" + testOutput).c_str())) {
      printf("Error creating tests folder\n");
      return;
    }
  }
  SetOutput(("tests/"+testOutput+"/raw.out").c_str());

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

  // print model description
  Print((testOutput+"\n").c_str());
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
  Init(0, numberOfDaysToSimulate * 24 * 60 * 60);
  User.Clear();
  (new PostGenerator)->Activate();
  (new AddGenerator)->Activate();
  (new DayPhaseManager)->Activate();
  (new DayStatistics)->Activate(24 * 60 * 60);
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

  PostTable.Output();
  AddTable.Output();
  PostsPerDay.Output();
  PostsPerHour.Output();
  AddsPerDay.Output();
  AddsPerHour.Output();
  addArrivalTimeStat.Output();

  // clear variables
  postCount = 0;
  addCount = 0;
  addCount6 = 0;
  numberOfRelevantPosts = 0;
  numberOfRelevantAdds = 0;
  numberOfIrrelevantPosts = 0;
  numberOfIrrelevantAdds = 0;
  numberOfSkippedPosts = 0;
  numberOfSkippedAdds = 0;

  // clear statistics
  PostTable.Clear();
  AddTable.Clear();
  PostsPerDay.Clear();
  PostsPerHour.Clear();
  AddsPerDay.Clear();
  AddsPerHour.Clear();
  addArrivalTimeStat.Clear();
}

int main()
{
  if (system("test -d tests") != 0)
  {
    if (system("mkdir tests"))
    {
      printf("Error creating tests folder\n");
      return 1;
    }
  }

  printf("test\n");
  makeTest("test", 10, 1000, 40, 5, 30, 10, 30, false);
  printf("test-autoregulate\n");
  makeTest("test-autoregulate", 10, 1000, 40, 5, 30, 10, 30, true);

  // printf("test-small-attention-span\n");
  // makeTest("test-small-attention-span", 10, 1000, 10, 5, 30, 10, 30, false);
  // printf("test-small-attention-span-autoregulate\n");
  // makeTest("test-small-attention-span-autoregulate", 10, 1000, 10, 5, 30, 10, 30, true);
}
