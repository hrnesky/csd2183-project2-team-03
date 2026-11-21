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

std::vector<int> best_events(const std::vector<Event> & events) {

  const int n = static_cast<int>(events.size());
  std::vector<int> order(n);
  std::iota(order.begin(), order.end(), 0);

  std::stable_sort(order.begin(), order.end(), [&](int a, int b)
  {
    return events[a].finish < events[b].finish;
  });

  // finish times in sorted order, for binary search
  std::vector<std::int64_t> finish(n);
  for (int i = 0; i < n; ++i) finish[i] = events[order[i]].finish;

  std::vector<int> p(n); // p[i]: events [0, p[i]) are compatible with i
  std::vector<std::int64_t> dp(n + 1, 0);

  for (int i = 0; i < n; i++)
  {
    p[i] = static_cast<int>(std::upper_bound(finish.begin(), finish.begin() + i,
                            events[order[i]].start) - finish.begin());

    dp[i + 1] = std::max(dp[i], dp[p[i]] + events[order[i]].reward);
  }

  // Walk back through the table to rebuild the chosen set.
  std::vector<int> chosen;
  for (int i = n; i > 0;) {
    const int e = order[i - 1];
    if (dp[i] == dp[i - 1]) {
      --i;  // skipping was just as good
    } else {
      chosen.push_back(e);
      i = p[i - 1];
    }
  }

  std::sort(chosen.begin(), chosen.end());
  return chosen;  // TODO
}
