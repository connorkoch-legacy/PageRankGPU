#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/time.h>
#include <vector>
#include <map>
using namespace std;

__global__ void pagerank(int* rowPtrs, int* colIdx, double* values, double* ranks, double* newRanks, int numRows) {

    unsigned int row = blockIdx.x*blockDim.x + threadIdx.x;

    if(row < numRows){
        unsigned int rowStart = rowPtrs[row];
        unsigned int rowEnd = rowPtrs[row+1];

        double rowSum = 0;
        for(int i = rowStart; i < rowEnd; i++){
            rowSum += (values[i] * ranks[colIdx[i]]);
        }
        newRanks[row] = 0.15 + 0.85 * rowSum;      //Damping factor added into the equation
    }
}

struct csrFormat{
    vector<int> rowPtrs;
    vector<int> colIdx;
    vector<double> values;

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

    ////////// Transfer data into CUDA kernel /////////////
    int* rowPtrs;
    int* colIdx;
    double* values;
    double* ranks;
    double* newRanks;
    //initialize ranks
    vector<double> ranksV(csr->numNodes, 10);
    vector<double> newRanksV(csr->numNodes, 10);

    int numIterations = atoi(argv[2]);

    cudaMalloc((void **)&rowPtrs, sizeof(int) * csr->rowPtrs.size());
    cudaMalloc((void **)&colIdx, sizeof(int) * csr->colIdx.size());
    cudaMalloc((void **)&values, sizeof(double) * csr->values.size());
    cudaMalloc((void **)&ranks, sizeof(double) * csr->numNodes);
    cudaMalloc((void **)&newRanks, sizeof(double) * csr->numNodes);

    cudaMemcpy(rowPtrs, csr->rowPtrs.data(), sizeof(int) * csr->rowPtrs.size(), cudaMemcpyHostToDevice);
    cudaMemcpy(colIdx, csr->colIdx.data(), sizeof(int) * csr->colIdx.size(), cudaMemcpyHostToDevice);
    cudaMemcpy(values, csr->values.data(), sizeof(double) * csr->values.size(), cudaMemcpyHostToDevice);
    cudaMemcpy(ranks, ranksV.data(), sizeof(double) * csr->numNodes, cudaMemcpyHostToDevice);
    // cudaMemcpy(newRanks, newRanksV.data(), sizeof(double) * csr->numNodes, cudaMemcpyHostToDevice);

    //Time for pagerank algorithm
    clock_gettime(CLOCK_MONOTONIC, &start);

    //Call function
	int threadsPerBlock = 256;
	int blocksPerGrid = (1024 + threadsPerBlock - 1) / threadsPerBlock;

    int iter = 0;
    while(iter < numIterations){
        cudaMemcpy(ranks, newRanksV.data(), sizeof(double) * csr->numNodes, cudaMemcpyHostToDevice);
        pagerank<<<blocksPerGrid, threadsPerBlock>>>(rowPtrs, colIdx, values, ranks, newRanks, csr->numNodes);
        cudaMemcpy(newRanksV.data(), newRanks, sizeof(double) * csr->numNodes, cudaMemcpyDeviceToHost);  //copy the shared memory array back to the original vector

        iter++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    diff = (1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / (double)1000000000;
    cout << endl << "Time to Pagerank: " << diff << " seconds" << endl;

    cudaFree(rowPtrs);
    cudaFree(colIdx);
    cudaFree(values);
    cudaFree(ranks);
    cudaFree(newRanks);


    ofstream textOut;
    textOut.open ("gpu_log.txt");
    for(int i = 0; i < csr->numNodes; i++){
        textOut << newRanksV[i] << " ";
        if(i % 20 == 0 && i != 0){
            textOut << endl;
        }
    }

    textOut.close();
}
















//
