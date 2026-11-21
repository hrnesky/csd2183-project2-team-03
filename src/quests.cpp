// Your quest schedule. The function below is a stub that compiles but does
// nothing useful yet; replace it with your implementation.

#include "quests.h"
#include <cstdint>
#include <queue>
#include <vector>

std::optional<QuestSchedule> quest_schedule(
    const std::vector<std::int64_t> &duration,
    const std::vector<Prerequisite> &prerequisites)
{
  const int k = static_cast<int>(duration.size());

  std::vector<std::vector<int>> adj(k);
  std::vector<int> indegree(k, 0);
  for (const auto &prereq : prerequisites)
  {
    adj[prereq.before].push_back(prereq.after);
    indegree[prereq.after]++;
  }

  // start[v] = max finish time of any prerequisite of v, or 0 if v has no prerequisites
  std::vector<std::int64_t> start(k, 0);
  std::vector<int> critical(k, -1);

  std::queue<int> q;
  for (int v = 0; v < k; ++v)
  {
    if (indegree[v] == 0)
    {
      q.push(v);
    }
  }

  int count = 0;
  while (!q.empty())
  {
    int u = q.front();
    q.pop();
    count++;

    std::int64_t finish_time = start[u] + duration[u];

    for (int v : adj[u])
    {
      if (critical[v] == -1 || finish_time > start[v])
      {
        start[v] = finish_time;
        critical[v] = u;
      }
      if (--indegree[v] == 0)
      {
        q.push(v);
      }
    }
  }

  if (count != k)
  {
    return std::nullopt; // cycle detected
  }

  QuestSchedule schedule;
  schedule.finish.resize(k);
  schedule.critical = std::move(critical);
  for (int i = 0; i < k; ++i)
  {
    schedule.finish[i] = start[i] + duration[i];
  }
  return schedule;
}
