// Greedy-vs-optimal study for the best-events
//
// Three greedy rules, each repeatedly picking the best COMPATIBLE event by
// largest reward, earliest finish, largest reward per unit time.
// For each size and reward model, generate INSTANCES random instances and
// record rule_reward / best_events_reward.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <map>
#include <numeric>
#include <random>
#include <vector>
#include "events.h"

using i64 = std::int64_t;

template <typename Better>
i64 greedy(const std::vector<Event> &ev, Better better) {

  std::vector<int> ord(ev.size());
  std::iota(ord.begin(), ord.end(), 0);

  std::stable_sort(ord.begin(), ord.end(), [&](int a, int b)
  {
    return better(ev[a], ev[b]);
  });

  std::map<i64, i64> taken;
  i64 total = 0;

  for (int i : ord)
  {
    const Event &e = ev[i];
    auto nx = taken.lower_bound(e.start);  // first taken with start >= e.start
    bool ok = true;

    if (nx != taken.end() && nx->first < e.finish)
    {
      ok = false;  // next starts before e ends
    }

    if (ok && nx != taken.begin())
    {
      auto pv = std::prev(nx);
      if (pv->second > e.start)
      {
        ok = false;  // prev ends after e starts
      }
    }

    if (ok)
    {
      taken[e.start] = e.finish;
      total += e.reward;
    }
  }

  return total;
}

i64 rule_reward(const std::vector<Event> &ev) {

  return greedy(ev, [](const Event &a, const Event &b)
  {
    return a.reward > b.reward;
  });
}

i64 rule_finish(const std::vector<Event> &ev) {

  return greedy(ev, [](const Event &a, const Event &b)
  {
    return a.finish < b.finish;
  });
}

i64 rule_density(const std::vector<Event> &ev) {

  // a.reward/(a.finish-a.start) > b.reward/(b.finish-b.start), by cross-multiplying
  return greedy(ev, [](const Event &a, const Event &b)
  {
    return a.reward * (b.finish - b.start) >
           b.reward * (a.finish - a.start);
  });
}

// Model "independent": reward uniform in [1,1000], unrelated to duration.
// Model "grows": reward = duration * u + noise, u uniform in [1,10]; longer is worth more.
std::vector<Event> make_instance(int n, bool grows, std::mt19937_64 &rng) {

  std::uniform_int_distribution<i64> start(0, 10LL * n), dur(1, 100), indep(1, 1000),
      mult(1, 10), noise(0, 20);
  std::vector<Event> ev(n);

  for (auto &e : ev)
  {
    i64 s = start(rng), d = dur(rng);
    i64 r = grows ? d * mult(rng) + noise(rng) : indep(rng);
    e = {s, s + d, r};
  }

  return ev;
}

int main() {

  const int INSTANCES = 1000;
  std::mt19937_64 rng(2183);

  std::puts("model,n,rule,mean_ratio,min_ratio,instances");

  for (bool grows : {false, true})
  {
    for (int n : {10, 100, 1000})
    {
      double sum[3] = {0, 0, 0}, mn[3] = {2, 2, 2};

      for (int it = 0; it < INSTANCES; ++it)
      {
        auto ev = make_instance(n, grows, rng);

        i64 opt = 0;
        for (int i : best_events(ev))
        {
          opt += ev[i].reward;
        }

        i64 g[3] = {rule_reward(ev), rule_finish(ev), rule_density(ev)};

        for (int r = 0; r < 3; ++r)
        {
          double q = (double)g[r] / (double)opt;
          sum[r] += q;
          mn[r] = std::min(mn[r], q);
        }
      }

      const char *names[3] = {"largest_reward", "earliest_finish", "reward_per_time"};

      for (int r = 0; r < 3; ++r)
      {
        std::printf("%s,%d,%s,%.4f,%.4f,%d\n", grows ? "grows" : "independent", n,
                    names[r], sum[r] / INSTANCES, mn[r], INSTANCES);
      }
    }
  }
}