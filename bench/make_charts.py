import csv
from pathlib import Path
import matplotlib.pyplot as plt
root=Path(__file__).resolve().parent
rows=[]; order=[]
for line in (root/'results'/'measurements.csv').read_text().splitlines():
    cols=line.split(',')
    if cols[0]=='RESULT':
        rows.append(dict(graph=cols[1],n=int(cols[2]),edges=int(cols[3]),dijkstra=float(cols[5]),bf=float(cols[6]),rounds=int(cols[7])))
    elif cols[0]=='ORDER':
        order.append(dict(n=int(cols[1]),order=cols[2],passes=int(cols[3])))
for family in ('sparse','grid'):
    data=[x for x in rows if x['graph']==family]
    fig,ax=plt.subplots(figsize=(7.5,4.5))
    ax.plot([x['n'] for x in data],[x['dijkstra'] for x in data],marker='o',label='Dijkstra')
    ax.plot([x['n'] for x in data],[x['bf'] for x in data],marker='s',label='Bellman-Ford')
    ax.set_title(f'Shortest-path runtime — {family} graphs')
    ax.set_xlabel('Islands (V)');ax.set_ylabel('Median time per run (ms)')
    ax.legend();ax.grid(alpha=.3);fig.tight_layout()
    fig.savefig(root/'results'/f'{family}_runtime.png',dpi=180)
    plt.close(fig)
fig,ax=plt.subplots(figsize=(7.5,4.5))
for ordering in ('forward','reverse'):
    data=[x for x in order if x['order']==ordering]
    ax.plot([x['n'] for x in data],[x['passes'] for x in data],marker='o',label=ordering)
ns=sorted({x['n'] for x in order})
avgs=[sum(x['passes'] for x in order if x['n']==n and x['order'].startswith('random'))/5 for n in ns]
ax.plot(ns,avgs,marker='s',label='random (mean of 5)')
ax.set_title('Bellman-Ford passes — same path, different edge order')
ax.set_xlabel('Islands (V)');ax.set_ylabel('Full edge-list passes')
ax.legend();ax.grid(alpha=.3);fig.tight_layout()
fig.savefig(root/'results'/'edge_order_rounds.png',dpi=180)
plt.close(fig)
