#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/time.h>
using namespace std;

int main(int argc, const char ** argv){

    //User must provide a filename
    if(argc != 3){
        cout << "Error: requires a filename and a number of iterations" << endl;
        return -1;
    }

    /////////////////////// READ IN GRAPH FROM FILE ///////////////////////
    timespec start;
    timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    //Read in file from user input
    ifstream fin(argv[1]);

    //Read the first 2 values from the file to get # of vertices and edges
    int numVertices, numEdges;
    fin >> numVertices;
    fin >> numEdges;
    cout << numVertices << " : " << numEdges << endl;

    float adjMatrix[numVertices][numVertices];
    memset(adjMatrix, 0, sizeof(float) * numVertices * numVertices);

    //populate adjacency matrix
    string line;
    while(getline(fin, line)){
        //read in the edge: source --> dest
        if(line.size() == 0) continue;

        int sourceVert;
        int destVert;

        stringstream ss(line);
        ss >> sourceVert >> destVert;

        adjMatrix[destVert][sourceVert] = 1;
    }

    fin.close();

    //END TIME TO READ IN FILE
    clock_gettime(CLOCK_MONOTONIC, &end);

    double diff = (1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / (double)1000000000;
    cout << endl << "Time to read in file: " << diff << " seconds" << endl;

    /////////////////////// SET INITIAL WEIGHTS ///////////////////////

    //Time to get number of edges and set initial weights in matrix
    clock_gettime(CLOCK_MONOTONIC, &start);
    /************ Set proper values in adjMatrix ************/
    for(int i = 0; i < numVertices; i++){
        //get the number of edges for each vertex in the current mat column
        int currNumEdges = 0;
        for(int j = 0; j < numVertices; j++){
            if(adjMatrix[j][i] == 1){
                currNumEdges += 1;
            }
        }
        //then go back and set the correct weight of each edge weight in the col
        if(currNumEdges != 0){
            for(int j = 0; j < numVertices; j++){
                if(adjMatrix[j][i] == 1){
                    adjMatrix[j][i] = 1.0f / currNumEdges;
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    diff = (1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / (double)1000000000;
    cout << endl << "Time to set initial weights: " << diff << " seconds" << endl;

    //print matrix
    for(int i = 0; i < numVertices; i++){
        int currNumEdges = 0;
        for(int j = 0; j < numVertices; j++){
            cout << adjMatrix[i][j] << " ";
        }
        cout << endl;
    }

    /************ Pagerank code ************/

    //Time for pagerank algorithm
    clock_gettime(CLOCK_MONOTONIC, &start);

    //initialize ranks for each vertex in the graph
    float ranks[numVertices];
    for(int i = 0; i < numVertices; i++){
        ranks[i] = 1.0f / numVertices;
    }

    int numIterations = atoi(argv[2]);
    int iter = 0;
    while(iter < numIterations){
        cout << "Iteration " << iter << "... " << endl;
        for(int i = 0; i < numVertices; i++){
            float rowTotal = 0;
            for(int j = 0; j < numVertices; j++){
                rowTotal += adjMatrix[i][j] * ranks[j];
            }
            ranks[i] = rowTotal;
        }
        iter++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    diff = (1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / (double)1000000000;
    cout << endl << "Time to Pagerank: " << diff << " seconds" << endl;

    for(int i = 0; i < numVertices; i++){
        cout << ranks[i] << endl;
    }
}
