#include <cstdio>
#include <random>
#include "events.h"

int main() {

  std::mt19937_64 rng(1);

  for (int it = 0; it < 20000; ++it) {

    int n = rng() % 13;  // 0..12 events
    std::vector<Event> ev;

    for (int i = 0; i < n; ++i)
    {
      std::int64_t s = rng() % 20, d = 1 + rng() % 8;
      ev.push_back({s, s + d, (std::int64_t)(1 + rng() % 30)});
    }

    std::int64_t best = 0;
    for (int mask = 0; mask < (1 << n); ++mask)
    {
      bool ok = true; std::int64_t sum = 0;
      for (int i = 0; i < n && ok; ++i) if (mask >> i & 1)
      {
        sum += ev[i].reward;
        for (int j = i + 1; j < n; ++j) if (mask >> j & 1)
          if (ev[i].start < ev[j].finish && ev[j].start < ev[i].finish) 
          { 
            ok = false; break;
          }
      }
      if (ok) 
        best = std::max(best, sum);
    }

    auto got = best_events(ev);
    std::int64_t sum = 0;

    for (size_t a = 0; a < got.size(); ++a) 
    {
      if (a && got[a] <= got[a-1])
      {
        std::puts("not ascending"); return 1;
      }

      sum += ev[got[a]].reward;
      for (size_t b = a + 1; b < got.size(); ++b)
      {
        const Event &x = ev[got[a]], &y = ev[got[b]];
        if (x.start < y.finish && y.start < x.finish)
        { 
          std::puts("overlap"); return 1;
        }
      }
    }

    if (sum != best) 
    { 
      std::printf("FAIL it=%d got %lld want %lld\n", it, (long long)sum, (long long)best);
      return 1;
    }
  }

  std::puts("best_events OK on 20000 random small instances");
}
