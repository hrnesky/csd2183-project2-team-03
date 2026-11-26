// shortest-path benchmark.
#include "graph.h"
#include "routes.h"
#include "nanobench.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using std::vector;
struct CountResult { int passes; std::uint64_t checks; vector<std::int64_t> dist; };

// Instrumented copy of the same in-place relaxation policy a measuring
// stick that mirrors the real algorithm.
CountResult count_bf(int n, const vector<Edge>& edges, int source) {
  vector<std::int64_t> dist(n, kUnreachable);
  dist[source]=0;
  int passes=0;
  std::uint64_t checks=0;
  for (int round=0; round<n; ++round) {
    bool updated=false;
    ++passes;
    for (const auto& e: edges) {
      ++checks;
      if (dist[e.from] == kUnreachable) continue;
      auto candidate=dist[e.from]+e.weight;
      if (candidate<dist[e.to]) {dist[e.to]=candidate;updated=true;}
    }
    if (!updated) break;
  }
  return {passes,checks,std::move(dist)};
}

vector<Edge> sparse_graph(int n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> weight(1,100);
  std::uniform_int_distribution<int> vertex(0,n-1);
  vector<Edge> edges;
  edges.reserve(4*n);
  // Chain 0->1->...->n-1 first so every vertex is reachable from 0;
  // then add random extra edges on top.
  for (int i=1;i<n;++i) edges.push_back({i-1,i,weight(rng)});
  while (edges.size()<std::size_t(4*n)) {
    int u=vertex(rng),v=vertex(rng);
    if(u!=v)edges.push_back({u,v,weight(rng)});
  }
  std::shuffle(edges.begin(),edges.end(),rng);
  return edges;
}

vector<Edge> grid_graph(int side, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> weight(1,100);
  vector<Edge> edges;
  int n=side*side;
  edges.reserve(4*n);
  for(int y=0;y<side;++y)for(int x=0;x<side;++x) {
    int u=y*side+x;
    if(x+1<side){int v=u+1; edges.push_back({u,v,weight(rng)});edges.push_back({v,u,weight(rng)});}
    if(y+1<side){int v=u+side; edges.push_back({u,v,weight(rng)});edges.push_back({v,u,weight(rng)});}
  }
  std::shuffle(edges.begin(),edges.end(),rng);
  return edges;
}

void compare(ankerl::nanobench::Bench& bench,const char* family,int n,const vector<Edge>& edges) {
  auto d=dijkstra(n,edges,0);
  auto b=bellman_ford(n,edges,0);
  auto c=count_bf(n,edges,0);
  if(!b.negative_cycle.empty() || d.dist != b.paths.dist || d.dist != c.dist) {
    throw std::runtime_error("Dijkstra/Bellman-Ford distance mismatch");
  }
  int reachable=0;
  for(auto x: d.dist) if(x!=kUnreachable) ++reachable;
  auto label=std::string(family)+", n="+std::to_string(n);
  bench.run("Dijkstra "+label,[&] {auto r=dijkstra(n,edges,0);ankerl::nanobench::doNotOptimizeAway(r.dist.back());});
  auto dj=bench.results().back().median(ankerl::nanobench::Result::Measure::elapsed)*1000;
  bench.run("Bellman-Ford "+label,[&] {auto r=bellman_ford(n,edges,0);ankerl::nanobench::doNotOptimizeAway(r.paths.dist.back());});
  auto bf=bench.results().back().median(ankerl::nanobench::Result::Measure::elapsed)*1000;
  std::cout<<"RESULT,"<<family<<","<<n<<","<<edges.size()<<","<<reachable<<","<<std::fixed<<std::setprecision(6)<<dj<<","<<bf<<","<<c.passes<<","<<c.checks<<",PASS\n";
}

vector<Edge> path_graph(int n) {
  vector<Edge> edges;
  for(int i=0;i<n-1;++i)edges.push_back({i,i+1,1});
  return edges;
}
void order_study(int n){
  auto base=path_graph(n);
  auto baseline=count_bf(n,base,0);
  auto run=[&](const char* order,const vector<Edge>& edges){
    auto count=count_bf(n,edges,0);
    if(count.dist!=baseline.dist || bellman_ford(n,edges,0).paths.dist!=baseline.dist)
      throw std::runtime_error("Edge ordering changed final distances");
    std::cout<<"ORDER,"<<n<<","<<order<<","<<count.passes<<","<<count.checks<<",PASS\n";
  };
  run("forward",base);
  std::reverse(base.begin(),base.end());
  run("reverse",base);
  for(int rep=0;rep<5;++rep){
    auto shuffled=base;
    std::mt19937_64 rng(2183+rep);
    std::shuffle(shuffled.begin(),shuffled.end(),rng);
    auto name="random_"+std::to_string(rep+1);
    run(name.c_str(),shuffled);
  }
}
// Tiny independent Floyd-Warshall oracle for small graphs, correctness only,
// never timed, so it can't contaminate benchmark numbers.
void verify_small_graphs() {
  constexpr int n=8;
  std::mt19937_64 rng(9182);
  std::uniform_int_distribution<int> vertex(0,n-1),weight(0,30);
  for(int trial=0;trial<100;++trial) {
    vector<Edge> edges;
    vector<vector<std::int64_t>> fw(n,vector<std::int64_t>(n,kUnreachable));
    for(int i=0;i<n;++i)fw[i][i]=0;
    for(int e=0;e<24;++e){
      int u=vertex(rng),v=vertex(rng);
      if(u==v)continue;
      int w=weight(rng);
      edges.push_back({u,v,w});
      fw[u][v]=std::min(fw[u][v],std::int64_t(w));
    }
    for(int k=0;k<n;++k)for(int i=0;i<n;++i)for(int j=0;j<n;++j)
      if(fw[i][k]!=kUnreachable && fw[k][j]!=kUnreachable)
        fw[i][j]=std::min(fw[i][j],fw[i][k]+fw[k][j]);
    for(int source=0;source<n;++source){
      auto d=dijkstra(n,edges,source);
      auto b=bellman_ford(n,edges,source);
      if(!b.negative_cycle.empty()||d.dist!=fw[source]||b.paths.dist!=fw[source])
        throw std::runtime_error("Small-graph Floyd-Warshall oracle mismatch");
    }
  }
  // Negative edges, no negative cycle: shortest paths still well-defined.
  vector<Edge> negative_no_cycle={{0,1,5},{1,2,-8},{0,2,4},{2,3,2}};
  auto b=bellman_ford(4,negative_no_cycle,0);
  if(!b.negative_cycle.empty() || b.paths.dist!=vector<std::int64_t>({0,5,-3,-1}))
    throw std::runtime_error("Negative edge shortest-path check failed");
  // Negative cycle reachable from 0 but not from source 3.
  vector<Edge> cycle={{0,1,1},{1,2,-4},{2,1,2}};
  auto c=bellman_ford(4,cycle,0);
  if(c.negative_cycle.empty())throw std::runtime_error("Reachable cycle not detected");
  // Walk the reported cycle edge-by-edge and confirm it's genuinely negative.
  std::int64_t total=0;
  for(std::size_t i=0;i<c.negative_cycle.size();++i){
    int u=c.negative_cycle[i],v=c.negative_cycle[(i+1)%c.negative_cycle.size()];
    bool found=false;
    for(auto const& e:cycle)if(e.from==u&&e.to==v){total+=e.weight;found=true;break;}
    if(!found)throw std::runtime_error("Extracted negative cycle contains a missing directed edge");
  }
  if(total>=0)throw std::runtime_error("Extracted cycle is not negative");
  if(!bellman_ford(4,cycle,3).negative_cycle.empty())
    throw std::runtime_error("Unreachable negative cycle incorrectly detected");
  std::cerr<<"VALIDATION: 800 Floyd-Warshall source cases, negative edge, reachable/unreachable cycle PASS\n";
}

int main(){
  verify_small_graphs();
  ankerl::nanobench::Bench bench;
  bench.title("Kestrel Isles shortest-path benchmark").unit("run").epochs(7)
      .minEpochTime(std::chrono::milliseconds(2)).timeUnit(std::chrono::milliseconds(1),"ms");
  std::cout<<"TYPE,GRAPH,V,E,REACHABLE,DIJKSTRA_MS,BELLMAN_FORD_MS,BF_PASSES,BF_EDGE_CHECKS,VALIDATION\n";
  for(int side : {16,24,32,48}){
    int n=side*side;
    compare(bench,"sparse",n,sparse_graph(n,2183+n));
    compare(bench,"grid",n,grid_graph(side,3183+n));
  }
  std::cout<<"TYPE,V,ORDER,BF_PASSES,BF_EDGE_CHECKS,VALIDATION\n";
  for(int n:{64,256,1024,4096})order_study(n);
}