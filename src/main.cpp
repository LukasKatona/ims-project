#include "simlib.h"
#include <string.h>
#include <vector>

using namespace std;

bool debugPrints = false;
const int DEFAULT_DAYS_TO_SIMULATE = 1;

// Model inputs
double postArrivalTime = 10;
double adArrivalTime = 1000;
double attentionSpan = 40;

double lengthOfPostLow = 15;
double lengthOfPostHigh = 20;

double lengthOfAdLow = 15;
double lengthOfAdHigh = 25;

bool autoregulate = true;

int numberOfDaysToSimulate = DEFAULT_DAYS_TO_SIMULATE;

// Global objects
Facility User("User");

Histogram PostsPerDay("Shlédnuté příspěvky za den", 0,1,numberOfDaysToSimulate);
Histogram AdsPerDay("Shlédnuté reklamy za den", 0,1,numberOfDaysToSimulate);
Histogram PostsPerHour("Shlédnuté příspěvky za hodinu", 0,1,numberOfDaysToSimulate*24);

Stat adArrivalTimeStat("Čas mezi příchody reklam");

// Variables
int postCount = 0;
int adCount = 0;
int adCountPerDay = 0;

int adFatigue = 0;

int numberOfRelevantPosts = 0;
int numberOfRelevantAds = 0;

int numberOfIrrelevantPosts = 0;
int numberOfIrrelevantAds = 0;

int numberOfSkippedPosts = 0;
int numberOfSkippedAds = 0;

double AdArrivalTimeUpScale = 1.6;
double AdArrivalTimeDownScale = 0.9;
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
class AdFatigueDecrementor : public Event {
  void Behavior() {
    if (adFatigue > 0) {
      adFatigue--;
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

    double currentAttentionSpan = abs(Normal(attentionSpan, 5));
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
    PostsPerDay(getDayFromTime(Time));
    PostsPerHour(getHourFromTime(Time));
  }
};

class Ad : public Process
{
public:
  Ad(int lengthOfAd) : Process()
  {
    this->lengthOfAd = lengthOfAd;
  }
  int lengthOfAd;

  void Behavior()
  {
    Seize(User);

    adCount++;
    adCountPerDay++;
    adFatigue++;
    (new AdFatigueDecrementor)->Activate(Time + (24 * 60 * 60 / 6));

    if (adFatigue == autoregulateThreshold) {
      if (autoregulate) {
        adArrivalTime = adArrivalTime * AdArrivalTimeUpScale;
        adArrivalTimeStat(adArrivalTime);
      }
      adFatigue = 0;
    }

    double currentAttentionSpan = abs(Normal(attentionSpan, 5));
    if (currentAttentionSpan < lengthOfAd)
    {
      Wait(currentAttentionSpan);
      numberOfSkippedAds++;
    }
    else
    {
      Wait(lengthOfAd);
      double probabilityOfGoodAd = Uniform(0, 1);
      if (probabilityOfGoodAd > 0.44)
      {
        numberOfRelevantAds++;
      }
      else
      {
        numberOfIrrelevantAds++;
      }
    }

    Release(User);
    AdsPerDay(getDayFromTime(Time));
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

class AdGenerator : public Event
{
  void Behavior()
  {
    (new Ad(Uniform(lengthOfAdLow, lengthOfAdHigh)))->Activate();
    Activate(Time + adArrivalTime);
  }
};

// make statistics about day
class AdArrivalTimeRegulation : public Event
{
  void Behavior()
  {
    if (autoregulate)
    {
      adArrivalTime = adArrivalTime * AdArrivalTimeDownScale;
      if (adCountPerDay <= 2) {
        adArrivalTime = adArrivalTime * AdArrivalTimeDownScale;
      }
      adArrivalTimeStat(adArrivalTime);
      adCountPerDay = 0;
    }
    AdArrivalTimeRegulation::Activate(Time + (24 * 60 * 60));
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
  double adArrivalTime,
  double attentionSpan,
  double lengthOfPostLow,
  double lengthOfPostHigh,
  double lengthOfAdLow,
  double lengthOfAdHigh,
  bool autoregulate,
  bool multiTest = false,
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
  ::adArrivalTime = adArrivalTime;
  ::attentionSpan = attentionSpan;
  ::lengthOfPostLow = lengthOfPostLow;
  ::lengthOfPostHigh = lengthOfPostHigh;
  ::lengthOfAdLow = lengthOfAdLow;
  ::lengthOfAdHigh = lengthOfAdHigh;
  ::autoregulate = autoregulate;

  adArrivalTimeStat(adArrivalTime);

  // print model description
  Print((testOutput+"\n").c_str());
  Print(" Model inputs:\n");
  Print(" - postArrivalTime: %f\n", postArrivalTime);
  Print(" - adArrivalTime: %f\n", adArrivalTime);
  Print(" - attentionSpan: %f\n", attentionSpan);
  Print(" - lengthOfPostLow: %f\n", lengthOfPostLow);
  Print(" - lengthOfPostHigh: %f\n", lengthOfPostHigh);
  Print(" - lengthOfAdLow: %f\n", lengthOfAdLow);
  Print(" - lengthOfAdHigh: %f\n", lengthOfAdHigh);
  Print(" - autoregulate: %d\n", autoregulate);

  // run simulation
  Init(0, numberOfDaysToSimulate * 24 * 60 * 60);
  User.Clear();
  (new PostGenerator)->Activate();
  (new AdGenerator)->Activate();
  (new DayPhaseManager)->Activate();
  (new UserActivityManager)->Activate();
  (new AdArrivalTimeRegulation)->Activate(24 * 60 * 60);
  Run();

  // print statistics
  Print("\nPost saw: %d\n", postCount);
  Print("Relevant posts: %d\n", numberOfRelevantPosts);
  Print("Irrelevant posts: %d\n", numberOfIrrelevantPosts);
  Print("Skipped posts: %d\n", numberOfSkippedPosts);

  Print("\nAds saw: %d\n", adCount);
  Print("Relevant ads: %d\n", numberOfRelevantAds);
  Print("Irrelevant ads: %d\n", numberOfIrrelevantAds);
  Print("Skipped ads: %d\n", numberOfSkippedAds);

  if (!multiTest) {
    PostsPerDay.Output();
    PostsPerHour.Output();
  }
  AdsPerDay.Output();
  adArrivalTimeStat.Output();

  // clear variables
  postCount = 0;
  adCount = 0;
  adFatigue = 0;
  numberOfRelevantPosts = 0;
  numberOfRelevantAds = 0;
  numberOfIrrelevantPosts = 0;
  numberOfIrrelevantAds = 0;
  numberOfSkippedPosts = 0;
  numberOfSkippedAds = 0;

  // clear statistics
  PostsPerDay.Clear();
  PostsPerHour.Clear();
  AdsPerDay.Clear();
  adArrivalTimeStat.Clear();
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

  //gneral parameters
  printf("test-with-general-parameters\n");
  makeTest("test-with-general-parameters", 10, 1000, 40, 15, 20, 15, 25, false);
  printf("test-with-general-parameters-autoregulate\n");
  makeTest("test-with-general-parameters-autoregulate", 10, 1000, 40, 15, 20, 15, 25, true);
/*
  //post arrival time
  for (int i = 10; i <= 120; i += 10)
  {
    printf("test-with-post-arrival-time: %d s\n", i);
    makeTest("test-with-post-arrival-time-" + to_string(i), i, 1000, 40, 15, 20, 15, 25, true, true);
  }

  // user attention span
  for (int i = 10; i <= 120; i += 10)
  {
    printf("test-with-attention-span: %d s\n", i);
    makeTest("test-with-attention-span-" + to_string(i), 10, 1000, i, 15, 20, 15, 25, true, true);
  }

  // length of post
  for (int i = 10; i <= 120; i += 5)
  {
    printf("test-with-length-of-post: %d s\n", i);
    makeTest("test-with-length-of-post-" + to_string(i), 10, 1000, 120, i-5, i, 15, 25, true, true); // we need to increase attention span to see the effect, otherwise the longer posts would be skipped
  }*/
}
