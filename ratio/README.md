# Ratio study (greedy rules vs best_events)

Files:
- ratio.cpp       runs the experiment and prints results as CSV
- check_best.cpp  brute-force check of best_events on small random instances
- results.csv     raw results (mean and minimum ratio per model, size and rule)
- plot.py         draws ratio.png from results.csv
- ratio.png       the plot used in the report

Dependencies: Python 3.14, pandas 3.1.1, matplotlib 3.11.2
  (pip install pandas matplotlib)

Environment: g++ 16.1.0 (MinGW-w64), Windows, <your processor model>

Build and run (from the repo root, Windows):
  g++ -std=c++23 -O2 -Iinclude ratio/ratio.cpp src/events.cpp -o build/ratio.exe
  build\ratio.exe > ratio\results.csv
  python ratio\plot.py

  the python will generate the grapth for the use of the report

Brute-force check:
  g++ -std=c++23 -O1 -g -Iinclude ratio/check_best.cpp src/events.cpp -o build/check_best.exe
  build\check_best.exe