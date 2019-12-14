#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/time.h>
#include <vector>
#include <map>
using namespace std;

struct csrFormat{
    vector<int> rowPtrs;
    vector<int> colIdx;
    vector<double> values;
    vector<int> rowIdx;

    int numNodes;
};

csrFormat *readFile(string fileName){
    csrFormat *csr = new csrFormat;

    ifstream inputFile;
    inputFile.open(fileName);

    string line;
    bool gotFirstLine = false;
    int numNodes;

    //also create a dictionary to map row to column
    cout << "Reading matrix from " << fileName << "..." << endl;
    map<int, vector<int>> node1_to_node2;
    map<int, vector<int>> node2_to_node1;
    while(getline(inputFile, line)){
        if(line.size() == 0 or line[0] == '%') continue;    //don't read comment lines

        if(!gotFirstLine){  //read the rows, cols, numvals from the first line in the file
            stringstream ss(line);
            string temp;
            ss >> temp;
            gotFirstLine = true;

            numNodes = stoi(temp);
            csr->numNodes = numNodes;
        } else {
            int node1;
            int node2;

            stringstream ss(line);
            ss >> node1;
            ss >> node2;

            node1_to_node2[node1-1].push_back(node2-1);
            node2_to_node1[node2-1].push_back(node1-1);
        }
    }
    inputFile.close();

    //create the correct rows vector in csr from rowCounts
    cout << "Populating CSR data structure..." << endl;

    int currRow = 0;
    csr->rowPtrs.push_back(0);  //initial index
    for(int i = 0; i < numNodes; i++){
        for(int j = 0; j < node1_to_node2[i].size(); j++){ //loop through the vector of columns with values in the current row
            csr->colIdx.push_back(node1_to_node2[i][j]);
            csr->values.push_back(1.0 / node2_to_node1[node1_to_node2[i][j]].size());  // 1 / number of out-edges
            // cout << csr->colIdx[csr->colIdx.size()-1] << " : " << csr->values[csr->values.size()-1] << endl;
        }
        csr->rowPtrs.push_back(csr->rowPtrs[i] + node1_to_node2[i].size());    //rowPtrs[i] = rowPtrs[i-1] + num values in row i
    }

    // for (auto list : node2_to_node1) {
    //     cout << list.first << ": ";
    //     for(auto v : list.second){
    //         cout << v << " ";
    //     }
    //     cout << endl;
    // }
    // cout << csr->rowPtrs.size() << endl;
    // cout << csr->colIdx.size() << endl;
    // cout << csr->values.size() << endl;



    return csr;
}

int main(int argc, const char ** argv){

    //User must provide a filename
    if(argc != 3){
        cout << "Error: requires a filename and a number of iterations" << endl;
        return -1;
    }

    //TIME TO READ IN GRAPH FROM FILE
    timespec start;
    timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    //Read in file from user input and get CSR struct
    csrFormat *csr = readFile(string(argv[1]));

    //END TIME TO READ IN FILE
    clock_gettime(CLOCK_MONOTONIC, &end);

    double diff = (1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / (double)1000000000;
    cout << endl << "Time to read in file: " << diff << " seconds" << endl;

    /************ Pagerank code ************/

    //Time for pagerank algorithm
    clock_gettime(CLOCK_MONOTONIC, &start);

    // initialize ranks for each vertex in the graph
    // for(int i = 0; i < csr->numNodes; i++){
    //     ranks[i] = 1.0f;
    // }

    // int minIdx = 1000000;
    // int maxIdx = 0;
    // for(int i = 0; i < csr->colIdx.size(); i++){
    //     if(csr->colIdx[i] < minIdx){
    //         minIdx = csr->colIdx[i];
    //     }
    //     if(csr->colIdx[i] > maxIdx){
    //         maxIdx = csr->colIdx[i];
    //     }
    // }
    //
    // cout << minIdx << " : " << maxIdx << endl;
    // cout << ranks.size() << endl;
    // for(auto i : csr->colIdx){
    //     cout << i << endl;
    // }
    //
    // for(int i = 0; i < ranks.size(); i++){
    //     cout << ranks[i] << endl;
    // }

    //initialize ranks
    vector<double> ranks(csr->numNodes);
    vector<double> newRanks(csr->numNodes, 1);

    int numIterations = atoi(argv[2]);
    int iter = 0;
    while(iter < numIterations){
        cout << "Iteration " << iter+1 << "... " << endl;
        ranks = newRanks;

        #pragma omp parallel for
        for(int i = 1; i < csr->rowPtrs.size(); i++){       // i-1 = current row
            double rowTotal = 0.0;
            for(int j = csr->rowPtrs[i-1]; j < csr->rowPtrs[i]; j++){
                int currCol = csr->colIdx[j];
                double currVal = csr->values[j];

                // cout << currCol << " : " << currVal << endl;
                rowTotal += currVal * ranks[currCol];
            }
            newRanks[i-1] = 0.15 + 0.85 * rowTotal;
            // cout << rowTotal << endl;
        }
        iter++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    diff = (1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / (double)1000000000;
    cout << endl << "Time to Pagerank: " << diff << " seconds" << endl;


    // for(int i = 0; i < ranks.size(); i++){
    //     cout << newRanks[i] << endl;
    // }

    ofstream textOut;
    textOut.open ("cpu_log.txt");
    for(int i = 0; i < csr->numNodes; i++){
        // cout << ranks[i] << endl;
        textOut << newRanks[i] << " ";
        if(i % 20 == 0 && i != 0){
            textOut << endl;
        }
    }

    textOut.close();

}
















//
