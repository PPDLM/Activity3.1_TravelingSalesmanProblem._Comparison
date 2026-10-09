
// ============================================================
// tsp_reader.cpp
//
// What this program does:
//   1. Opens the file dsj1000.tsp
//   2. Saves the 1000 cities (their x and y position)
//   3. Measures the distance between cities
//   4. Picks N random cities for each experiment
//
// How to compile:  g++ -O2 -std=c++17 tsp_reader.cpp -o tsp_reader
// How to run:      ./tsp_reader dsj1000.tsp
// ============================================================
 
#include <bits/stdc++.h>
using namespace std;
 
 
// ------------------------------------------------------------
// A City is just a point on a map.
//
//   id = the city number from the file (1, 2, 3, ... 1000)
//   x  = horizontal position
//   y  = vertical position
//
// We use "double" for x and y because:
//   - some numbers are negative (like -838)
//   - the numbers are very big, and "int" can't hold the math
// ------------------------------------------------------------
struct City {
    int id;
    double x, y;
};
 
 
// ------------------------------------------------------------
// trim() cleans a line of text.
// It removes the extra spaces at the start and at the end.
//
// Example:  "   DIMENSION : 1000   "  ->  "DIMENSION : 1000"
// ------------------------------------------------------------
static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r");  // first real letter
    size_t end   = s.find_last_not_of(" \t\r");   // last real letter
 
    if (start == string::npos) return "";         // line was empty
 
    return s.substr(start, end - start + 1);      // keep the middle part
}
 
 
// ------------------------------------------------------------
// readTSP() reads the whole file and gives back a list of cities.
//
// The file looks like this:
//
//   NAME : dsj1000            <- PART 1: information
//   DIMENSION : 1000          <-   (how many cities)
//   EDGE_WEIGHT_TYPE : CEIL_2D<-   (how to measure distance)
//   NODE_COORD_SECTION        <- "the cities start here"
//    1 981036 508139          <- PART 2: one city per line
//    2 534120 -42453          <-   (number, x, y)
//   ...
//   EOF                       <- "the file ends here"
// ------------------------------------------------------------
vector<City> readTSP(const string& path) {
 
    // Open the file. Stop with an error if it doesn't exist.
    ifstream file(path);
    if (!file) throw runtime_error("Cannot open " + path);
 
    string line;
    int totalCities = 0;   // we will read this from "DIMENSION"
 
 
    // ===== PART 1: read the information lines =====
    while (getline(file, line)) {      // read one line at a time
        line = trim(line);
 
        // When we see this line, the cities come next. Stop here.
        if (line == "NODE_COORD_SECTION") break;
 
        // Info lines look like "KEY : VALUE". Find the ':'
        size_t colon = line.find(':');
        if (colon == string::npos) continue;   // no ':' -> skip line
 
        // Cut the line in two pieces:
        //   "DIMENSION : 1000"  ->  key = "DIMENSION", value = "1000"
        string key   = trim(line.substr(0, colon));
        string value = trim(line.substr(colon + 1));
 
        // We only care about how many cities there are.
        if (key == "DIMENSION") totalCities = stoi(value);  // stoi = text to number
 
        // Our distance formula is for CEIL_2D. Warn us if it's something else.
        if (key == "EDGE_WEIGHT_TYPE" && value != "CEIL_2D")
            cerr << "Warning: distance type is " << value << "\n";
    }
 
 
    // ===== PART 2: read the cities =====
    vector<City> cities;
    cities.reserve(totalCities);   // make room for 1000 cities
 
    while (getline(file, line)) {
        line = trim(line);
 
        if (line.empty()) continue;    // skip empty lines
        if (line == "EOF") break;      // end of file -> stop
 
        // A city line is "number x y", like "2 534120 -42453"
        // istringstream lets us take the 3 numbers out one by one.
        istringstream parts(line);
        City c;
        if (parts >> c.id >> c.x >> c.y)
            cities.push_back(c);       // add the city to the list
    }
 
 
    // Check: did we get all the cities we expected?
    if ((int)cities.size() != totalCities)
        cerr << "Warning: expected " << totalCities
             << " cities but read " << cities.size() << "\n";
 
    return cities;
}
 
 
// ------------------------------------------------------------
// dist() gives the distance between two cities.
//
// It's the normal straight-line distance (Pythagoras):
//      distance = sqrt( (x1-x2)^2 + (y1-y2)^2 )
//
// Then we ROUND UP to a whole number (that's what "ceil" does).
//   Example: 10.2 -> 11
//
// The file says CEIL_2D, which means exactly this rule.
// If you use another rule, your results won't match 18659688.
// ------------------------------------------------------------
long long dist(const City& a, const City& b) {
    double dx = a.x - b.x;   // horizontal difference
    double dy = a.y - b.y;   // vertical difference
    return (long long)ceil(sqrt(dx * dx + dy * dy));
}
 
 
// ------------------------------------------------------------
// sampleCities() picks N random cities out of the 1000.
//
// How it works (like shuffling a deck of cards):
//   1. Make a list: 0, 1, 2, ... 999
//   2. Shuffle it randomly
//   3. Take the first N
//
// The "seed" controls the randomness:
//   - same seed      -> same cities every time (you can repeat it)
//   - different seed -> different cities (for your 3 runs)
// ------------------------------------------------------------
vector<City> sampleCities(const vector<City>& all, int n, unsigned seed) {
 
    // Step 1: list of positions 0..999
    vector<int> positions(all.size());
    iota(positions.begin(), positions.end(), 0);   // fills 0,1,2,3...
 
    // Step 2: shuffle using the seed
    mt19937 randomGen(seed);
    shuffle(positions.begin(), positions.end(), randomGen);
 
    // Step 3: keep the first n cities
    vector<City> chosen;
    for (int i = 0; i < n; ++i)
        chosen.push_back(all[positions[i]]);
 
    return chosen;
}
 
 
// ------------------------------------------------------------
// buildMatrix() makes a table with ALL the distances.
//
//   D[i][j] = distance from city i to city j
//
// Why? So we calculate each distance only ONCE.
// Then NN, Cheapest-link and MST just look it up in the table,
// which is much faster than calculating it again and again.
//
//          city0  city1  city2
//   city0    0     50     80
//   city1   50      0     30
//   city2   80     30      0
//
// Notice: D[i][j] == D[j][i] (going A->B is the same as B->A)
// ------------------------------------------------------------
vector<vector<long long>> buildMatrix(const vector<City>& c) {
    int n = c.size();
 
    // Create an n x n table full of zeros
    vector<vector<long long>> D(n, vector<long long>(n, 0));
 
    // Fill it. We only compute half and copy to the other half.
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            D[i][j] = D[j][i] = dist(c[i], c[j]);
 
    return D;
}
 
 
// ------------------------------------------------------------
// tourLength() adds up the length of a full trip.
//
// "tour" is the order we visit the cities, for example:
//      tour = {3, 0, 2, 1}
// means: 3 -> 0 -> 2 -> 1 -> back to 3
//
// The "back to the start" part is important: a TSP tour is a loop.
// ------------------------------------------------------------
long long tourLength(const vector<int>& tour, const vector<vector<long long>>& D) {
    long long total = 0;
    int n = tour.size();
 
    for (int i = 0; i < n; ++i) {
        int from = tour[i];
        int to   = tour[(i + 1) % n];   // % n makes the last city go back to the first
        total += D[from][to];
    }
    return total;
}
 
 
// ------------------------------------------------------------
// main() is where the program starts.
// Right now it only TESTS that reading the file works.
// Later you will add your 3 algorithms here.
// ------------------------------------------------------------
int main(int argc, char* argv[]) {
 
    // Use the file name you typed, or "dsj1000.tsp" by default.
    string path = (argc > 1) ? argv[1] : "dsj1000.tsp";
    vector<City> all = readTSP(path);
 
 
    // ----- Test 1: we should have 1000 cities -----
    cout << "Cities read: " << all.size() << "\n";
 
 
    // ----- Test 2: first and last city must match the file -----
    //   City 1    should be (981036, 508139)
    //   City 1000 should be (471287, 119659)
    cout << "First: " << all.front().id << " ("
         << (long long)all.front().x << ", " << (long long)all.front().y << ")\n";
    cout << "Last:  " << all.back().id << " ("
         << (long long)all.back().x << ", " << (long long)all.back().y << ")\n";
 
 
    // ----- The experiments -----
    // Each N is one row of your table.
    // Each seed is one run (3 runs -> you can get average and std deviation).
    vector<int> Ns = {100, 500, 1000};   // change these to your table's N values
    vector<unsigned> seeds = {1, 2, 3};
 
    for (int N : Ns) {
        for (unsigned seed : seeds) {
 
            // 1. Pick N random cities
            vector<City> sub = sampleCities(all, N, seed);
 
            // 2. Make the distance table for those cities
            auto D = buildMatrix(sub);
 
            // 3. Make a tour.
            //    FOR NOW this is a fake tour: 0, 1, 2, ..., N-1 (just in order).
            //    It only proves tourLength() works.
            //    REPLACE it with your algorithms:
            //       nearestNeighbor(D), cheapestLink(D), mstTour(D)
            vector<int> tour(N);
            iota(tour.begin(), tour.end(), 0);
 
            // 4. Print the result
            cout << "N=" << N << " seed=" << seed
                 << " length=" << tourLength(tour, D) << "\n";
        }
    }
 
    // Remember: the best possible answer (18659688) is ONLY for N = 1000.
    // Your algorithms on N = 1000 should give a number BIGGER than that.
    return 0;
}
 
