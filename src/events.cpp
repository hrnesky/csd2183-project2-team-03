// Your event selections. Every function below is a stub that compiles but
// does nothing useful yet; replace each one with your implementation.

#include "events.h"
#include <algorithm>
#include <numeric>

std::vector<int> most_events(const std::vector<Event> & events) {
  const int n = static_cast<int>(events.size());
  if(n == 0) {
    return {};
  }

  //Sort indices of events by finish time (earliest finish first)
  std::vector<int> indices(n);
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(), [&events](int i, int j) {
    return events[i].finish < events[j].finish;
  });

  std::vector<int> result;
  std::int64_t last_finish = 0;
  bool have_one = false;

  for (int i : indices) {
    if(!have_one || events[i].start >= last_finish) {
      result.push_back(i); 
      last_finish = events[i].finish;
      have_one = true;
    }
  }

  //Spec requires indices to be returned in ascending order
  std::sort(result.begin(), result.end());

  return  result;
}

std::vector<int> best_events(const std::vector<Event> & /*events*/) {
  return {};  // TODO
}
