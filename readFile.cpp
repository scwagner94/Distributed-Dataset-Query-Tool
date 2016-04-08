//Sean Wagner
//CSC 330
//Project 3
#include <string>
#include <mpi.h>
#include <fstream>
#include "readFile.h"
#include <sstream>
using namespace std;

//list of dataset file names for printing and accessing
string documentFileNames [4] = {"kos","nytimes","enron","nips"};
//main directory for the files
string mainDir = "/home/mcconnel/BagOfWords/";

int main(int argc, char **argv) {
   int rank, numProcessors; //for storing the rank and number of processors for MPI
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &numProcessors);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if(rank == 0) {//this is the parent node, gets the vocab array for all datasets
      cout<<"LOADING PROGRAM, PLEASE WAIT\n";
      string* kosVocab = getVocabArray(KOS);
      string* nytimesVocab = getVocabArray(NYTIMES);
      string* enronVocab = getVocabArray(ENRON);
      string* nipsVocab = getVocabArray(NIPS);
      string* masterVocabList [4];//stores all datasets in a master array for easier access
      masterVocabList[0] = kosVocab;
      masterVocabList[1] = nytimesVocab;
      masterVocabList[2] = enronVocab;
      masterVocabList[3] = nipsVocab;

      //this part sets up the storage in each baby... should only be done once
      for(int j = 0; j<4;j++) { //for each database
         allDatabases currentDB = static_cast<allDatabases>(j);
         int* masterDocByteStart = readDocStart(currentDB);
         for(int i = 1; i < numProcessors; i++) { //for each processor
            //Payload: start doc number, end doc number, starting doc byte, dataset enum, starting arr size
            int* babyPayload = getDocumentPayloadArr(i,numProcessors,currentDB,masterDocByteStart);
            MPI_Send(babyPayload,5,MPI_INT,i,READDATA,MPI_COMM_WORLD);
         }
      }
      //check that babies have the data
      for(int i = 1; i < numProcessors; i++) {
         int loadedData [1];
         MPI_Recv(loadedData,1,MPI_INT,i,DATADONE,MPI_COMM_WORLD,0); //if receieve then they have the power
      }
      //have loaded the babies with data
      cout<<"BABY DATA LOADED\n\n\n";
      while(true) { //begin main dispatch loop
         cout<<"\n\nQuery List:\n1. What percent of documents in X use any one word more than ___ times?\n2. What words in X are used more than ___ times in any document?\n3. In which data set does the word ______ appear most frequently?\n4. Does _____ appear more frequently in X or Y?\n\n\n";
         string stringIn = "";
         bool quitter = false; //value to check if quit the program
         cout<<"Please input a query number, or type q to quit: ";
         getline(cin, stringIn);
         if(stringIn[0]=='q') { //checks to see if user wants to quit
            break;
         }
         int inputVal = -1;
         try {
            inputVal = atoi(stringIn.c_str()); //cast input to int
         }
         catch(int e) {
            cout<<"CAST FAILED";
            continue;
         }
         if (inputVal == 1) {//begin query 1
            cout<<"Datasets are as follows:\n1. KOS\n2. NY Times\n3. Enron\n4. NIPS\n\n";
            int inputDBNum,inputNumTimes;
            cout<<"Input a dataset number to query on: ";
            string newNumberIn = "";
            getline(cin, newNumberIn);
            inputDBNum = atoi(newNumberIn.c_str()); //convert to int
            inputDBNum = inputDBNum - 1;
            cout<<"Input the number of times a word appears: ";
            string newNumberIn2 = "";
            getline(cin, newNumberIn2);
            inputNumTimes = atoi(newNumberIn2.c_str()); //convert to int
            cout<<"\n";
            int sum = 0;
            for(int i = 1; i < numProcessors; i++) { //for every processor, send the info for query
               //send that you want query 1 executed
               int packageToSend [2]; //DB num, numTimes in indexes
               packageToSend[0] = inputDBNum;
               packageToSend[1] = inputNumTimes;
               MPI_Send(packageToSend,2,MPI_INT,i,QUERY1,MPI_COMM_WORLD);
            }
            for(int i = 1; i < numProcessors; i++) { //recv from every processor the results
               //start doc number, end doc number, starting doc byte, dataset enum, starting arr size
               int numDocs [1]; //just the count in the response
               MPI_Recv(numDocs,1,MPI_INT,i,QUERYRESPONSE,MPI_COMM_WORLD,0);
               sum = sum + numDocs[0];
            }
            int docsLen = getDocumentCount(static_cast<allDatabases>(inputDBNum));
            float answer = (sum*1.0) / (docsLen*1.0); //gets the doc len and calcs the percentage
            answer = answer * 100.0;
            cout<<"There are "<<answer<<"\% of "<<documentFileNames[inputDBNum]<<" documents that use a word more than "<<inputNumTimes<<" times.\n";
            cout<<"\n\n\n";
         }
         else if (inputVal == 2) { //breakout to query 2 here
            int uniqueReturnCount = 0;
            int initialReturnWordsSize = 25;
            int* uniqueReturnWords = new int[initialReturnWordsSize]; //unique return values array
            cout<<"Datasets are as follows:\n1. KOS\n2. NY Times\n3. Enron\n4. NIPS\n\n";
            int inputDBNum,inputNumTimes;
            cout<<"Input a dataset number to query on: ";
            string newNumberIn = "";
            getline(cin, newNumberIn);
            inputDBNum = atoi(newNumberIn.c_str()); //conver to int
            inputDBNum = inputDBNum - 1;
            cout<<"Input the number of times a word appears: ";
            string newNumberIn2 = "";
            getline(cin, newNumberIn2);
            inputNumTimes = atoi(newNumberIn2.c_str()); //convert to int
            cout<<"\n";
            if(inputNumTimes<=15) { //some level of protection to avoid long queries, can be removed easily
               cout<<"Too many return words. Please try a larger number of times to avoid issues.\n";
               continue;
            }
            //send info for query 2
            for(int i = 1; i < numProcessors; i++) {
               int packageToSend [2]; //sends the db num, num times for word
               packageToSend[0] = inputDBNum;
               packageToSend[1] = inputNumTimes;
               MPI_Send(packageToSend,2,MPI_INT,i,QUERY2,MPI_COMM_WORLD);
            }
            for(int currentProcessor = 1; currentProcessor < numProcessors; currentProcessor++) { //recieve from babies
               //start doc number, end doc number, starting doc byte, dataset enum, starting arr size
               int sizeOfReturn;
               MPI_Status status;
               MPI_Probe(currentProcessor,QUERY2,MPI_COMM_WORLD,&status);
               MPI_Get_count(&status, MPI_INT, &sizeOfReturn); //gets the size of the return value to instantiate array
               int arrOfWords [sizeOfReturn];
               MPI_Recv(arrOfWords,sizeOfReturn,MPI_INT,currentProcessor,QUERY2,MPI_COMM_WORLD,0);
               //this (unfortunate) nested loop ensures that dupe values do not occur
               for(int j = 1; j<arrOfWords[0];j++) {
                  int currentWord = arrOfWords[j];
                  bool alreadyExists = false;
                  for(int k = 0; k<uniqueReturnCount;k++) {
                     int wordCheck = uniqueReturnWords[k];
                     if(wordCheck==currentWord) {
                        alreadyExists = true;
                        break;
                     }
                  }
                  if(!alreadyExists) { //adds to the array if it isnt already in there
                     if(uniqueReturnCount>=initialReturnWordsSize) {
                        uniqueReturnWords = expandIntArray(uniqueReturnWords,initialReturnWordsSize);
                        int newInitialReturnWordsSize = initialReturnWordsSize / 4;
                        initialReturnWordsSize = initialReturnWordsSize + newInitialReturnWordsSize;
                     }
                     uniqueReturnWords[uniqueReturnCount] = currentWord;
                     uniqueReturnCount++;
                  }
               }
            }
            if(uniqueReturnCount>100) { //limits the return so that print isnt too massive
               cout<<"Too many return words. Please try a larger number of times to avoid issues.\n";
            }
            else { //prints the results
               cout<<"\n\nIn "<<documentFileNames[inputDBNum]<<", the following words appear more than "<<inputNumTimes<<" times:\n";
               for(int wordToPrint = 0; wordToPrint<uniqueReturnCount; wordToPrint++) {
                  int wordOut = uniqueReturnWords[wordToPrint];
                  cout<<masterVocabList[inputDBNum][wordOut]<<"\n";
               }
            }
            cout<<"\n\n\n";
         }
         else if (inputVal == 3) { //breakout for query 3 here
            string wordToFind;
            cout<<"Input the word to find: ";
            getline(cin,wordToFind);
            cout<<"\n";
            int vocabIndexSizeFixed = 4;
            int vocabIndexNumbers [vocabIndexSizeFixed]; //size of num datasets
            for(int k = 0; k < vocabIndexSizeFixed;k++) {
               vocabIndexNumbers[k] = -1; //sets negative value so it doesnt find random counts of words
            }
            for(int i = 0; i < vocabIndexSizeFixed; i++) {//for each database
               allDatabases currentDB = static_cast<allDatabases>(i);
               int limit = getVocabularyLength(currentDB);
               for(int j = 0; j<=limit;j++) { //for everything in the vocab length
                  string currentString = masterVocabList[i][j];
                  if(currentString==wordToFind) {
                     vocabIndexNumbers[i] = j; //tries to find the word
                     break;
                  }
               }
            }
            for(int i = 1; i < numProcessors; i++) { //send query instructions to every processor
               for(int j = 0; j < 4; j++) {
                  int packageToSend [2]; //should contian the db and the word num to look for
                  packageToSend[0] = j;
                  packageToSend[1] = vocabIndexNumbers[j];
                  MPI_Send(packageToSend,2,MPI_INT,i,QUERY3,MPI_COMM_WORLD);
               }
            }
            int returnedCounts [4];//for the return values, set to 0 to start
            for(int i = 0;i<4;i++) {
               returnedCounts[i] = 0;
            }
            for(int i = 1; i < numProcessors; i++) { //for each processor
               //should return an array of the count of the word in each dataset
               for(int j = 0; j<4;j++) { //for each dataset
                  int returnedData [2];
                  MPI_Recv(returnedData,2,MPI_INT,i,QUERYRESPONSE,MPI_COMM_WORLD,0);
                  int currentDataSet = returnedData[0]; //the dataset
                  int countOfWord = returnedData[1]; //the count
                  int currentValOfDS = returnedCounts[currentDataSet];
                  returnedCounts[currentDataSet] = currentValOfDS + countOfWord;
               }
            }
            int maxCounts = -1;
            string dsMax = "";
            for(int i = 0; i <4;i++) { //prints the data sets
               cout<<documentFileNames[i]<<"   Count: "<<returnedCounts[i]<<"\n";
               if(returnedCounts[i]>maxCounts) {
                  maxCounts = returnedCounts[i];
                  dsMax = documentFileNames[i];
               }
            }
            cout<<"\nThe word appears the most in the "<<dsMax<<" dataset, with "<<maxCounts<<" counts.\n\n\n";
         }
         else if (inputVal == 4) { //breakout for query 4 here
            string wordToFind;
            int databasesToQuery [2]; //only 2 datasets to query on here
            cout<<"Input the word to find: ";
            getline(cin,wordToFind);
            cout<<"\n";
            cout<<"Datasets are as follows:\n1. KOS\n2. NY Times\n3. Enron\n4. NIPS\n\n";
            cout<<"Input the first dataset number to query on: ";
            string newNumberIn = "";
            getline(cin, newNumberIn);
            databasesToQuery[0] = atoi(newNumberIn.c_str());
            databasesToQuery[0] = databasesToQuery[0] - 1; //removes one for indexing purposes
            cout<<"Input the second dataset number to query on: ";
            string newNumberIn2 = "";
            getline(cin, newNumberIn2);
            databasesToQuery[1] = atoi(newNumberIn2.c_str());
            cout<<"\n\n";
            databasesToQuery[1] = databasesToQuery[1] - 1;
            int vocabIndexSizeFixed = 4;
            int vocabIndexNumbers [vocabIndexSizeFixed];
            for(int k = 0; k < vocabIndexSizeFixed;k++) {
               vocabIndexNumbers[k] = -1; //sets negative value so it doesnt find random counts of words
            }
            for(int i = 0; i < 2; i++) {//for each database
               allDatabases currentDB = static_cast<allDatabases>(databasesToQuery[i]);
               int limit = getVocabularyLength(currentDB);
               for(int j = 0; j<=limit;j++) {//this finds the word in a dataset
                  string currentString = masterVocabList[databasesToQuery[i]][j];
                  if(currentString==wordToFind) {
                     vocabIndexNumbers[databasesToQuery[i]] = j;//stores word num
                     break;
                  }
               }
            }
            for(int i = 1; i < numProcessors; i++) { //for every processor, split it up
               for(int j = 0; j < 2; j++) { //only need 2 for the two datasets
                  int packageToSend [2]; //should contian db and the word num to look for
                  packageToSend[0] = databasesToQuery[j]; //ds to query on
                  packageToSend[1] = vocabIndexNumbers[databasesToQuery[j]];//word num
                  MPI_Send(packageToSend,2,MPI_INT,i,QUERY4,MPI_COMM_WORLD);
               }
            }
            int returnedCounts [4]; //sets to 4 because i want to store data from any potential data set, just easier
            for(int i = 0;i<4;i++) {
               returnedCounts[i] = 0;
            }
            for(int i = 1; i < numProcessors; i++) { //for each processor
               //should return an array of the count of the word in each dataset
               for(int j = 0; j<2;j++) { //for each dataset
                  int returnedData [2];
                  MPI_Recv(returnedData,2,MPI_INT,i,QUERY4,MPI_COMM_WORLD,0);
                  int currentDataSet = returnedData[0]; //gets the info for ds back
                  int countOfWord = returnedData[1]; //gets the word count
                  int currentValOfDS = returnedCounts[currentDataSet]; 
                  returnedCounts[currentDataSet] = currentValOfDS + countOfWord;//stores the info in the array
               }
            }
            int db1Count = returnedCounts[databasesToQuery[0]];
            int db2Count = returnedCounts[databasesToQuery[1]];
            if(db1Count>db2Count) {//handles printing of the different options for return values
               cout<<"The word "<<wordToFind<<" appears more ("<<db1Count<<" times) in "<<documentFileNames[databasesToQuery[0]]<<" than in "<<documentFileNames[databasesToQuery[1]]<<" ("<<db2Count<<" times).\n";
            }
            else if (db2Count>db1Count) {
               cout<<"The word "<<wordToFind<<" appears more ("<<db2Count<<" times) in "<<documentFileNames[databasesToQuery[1]]<<" than in "<<documentFileNames[databasesToQuery[0]]<<" ("<<db1Count<<" times).\n";
            }
            else {
               cout<<"The word "<<wordToFind<<" appears the equally ("<<db1Count<<" times) in both "<<documentFileNames[databasesToQuery[0]]<<" & "<<documentFileNames[databasesToQuery[1]]<<".\n";
            }
            cout<<"\n\n\n";
         }
         else { //lets user try again if they mistype
            cout<<"\n\n\n"<<stringIn<<"\n\nInput not recognized";
            continue;
         }
         for(int i = 1; i < numProcessors; i++) {  //makes sure all the babies are ready  before moving on
            int returnData [1];
            MPI_Recv(returnData,1,MPI_INT,i,BABYREADY,MPI_COMM_WORLD,0);
         }
      }
      for(int i = 1; i < numProcessors; i++) { //tells babies to quit when the user is done
            int sendData[1] = {5};
            MPI_Send(sendData,1,MPI_INT,i,BABYQUIT,MPI_COMM_WORLD);
      } 
      MPI_Finalize();//finalizes MPI and quits on head node
   }
   else {
      //accepts the information, reads the data
      babyDataStore masterBabyData [4];
      for(int i = 0; i < 4; i++) {
         int babyData [5]; //uses the array to read the data into the babies
         MPI_Recv(babyData,5,MPI_INT,0,READDATA,MPI_COMM_WORLD,0);
         babyDataStore bds = readEntries(babyData[3],babyData[0],babyData[1],babyData[2],babyData[4]);
         masterBabyData[babyData[3]] = bds;
      }
      int dataLoaded [1];
      dataLoaded[0] = 1;
      MPI_Send(dataLoaded,1,MPI_INT,0,DATADONE,MPI_COMM_WORLD);//sends back that the data is read
      
      while(true) { //main dispatch loop for babies
         MPI_Status status;
         MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //gets the current tag to break out
         int currentTag = status.MPI_TAG;
         if(currentTag==QUERY3) { //query 3 here
            for(int i = 0; i < 4; i++) {
               int vocabWordIndex [2];
               //needs to know the dataset and the word number, the summing and stuff has to get handled by grendel
               MPI_Recv(vocabWordIndex,2,MPI_INT,0,QUERY3,MPI_COMM_WORLD,0);
               //this is returning the dataset queried on, and the count of the word appearances
               int returnArr [2];
               returnArr[0] = vocabWordIndex[0];
               //has the database to query, word num to query
               returnArr[1] = query3(masterBabyData[vocabWordIndex[0]],vocabWordIndex[1]);
               MPI_Send(returnArr,2,MPI_INT,0,QUERYRESPONSE,MPI_COMM_WORLD);
            }
         }
         else if(currentTag==QUERY1) { //this is query 1 here
            int pacakgeRecv [2];
            MPI_Recv(pacakgeRecv,2,MPI_INT,0,QUERY1,MPI_COMM_WORLD,0);
            int numDocsFound [1]; //this is where data is stored and then returned from
            numDocsFound[0] = query1(masterBabyData[pacakgeRecv[0]],pacakgeRecv[1]);
            MPI_Send(numDocsFound,1,MPI_INT,0,QUERYRESPONSE,MPI_COMM_WORLD);
         }
         else if(currentTag==QUERY2) { //for query 2
            int pacakgeRecv [2];
            MPI_Recv(pacakgeRecv,2,MPI_INT,0,QUERY2,MPI_COMM_WORLD,0);
            int* qr = query2(masterBabyData[pacakgeRecv[0]],pacakgeRecv[1],25);//array of query 2 response
            MPI_Send(qr,qr[0],MPI_INT,0,QUERY2,MPI_COMM_WORLD);
         }
         else if(currentTag==QUERY4) { //for query 4
            for(int i = 0; i < 2; i++) {
               int vocabWordIndex [2];
               //needs to know the dataset and the word number, the summing and stuff has to get handled by grendel
               MPI_Recv(vocabWordIndex,2,MPI_INT,0,QUERY4,MPI_COMM_WORLD,0); //this is returning the dataset queried on, and the count of the word appearances
               int returnArr [2];
               returnArr[0] = vocabWordIndex[0];//has the database to query, word num to query
               returnArr[1] = query3(masterBabyData[vocabWordIndex[0]],vocabWordIndex[1]);
               MPI_Send(returnArr,2,MPI_INT,0,QUERY4,MPI_COMM_WORLD);
            }
         }
         else if(currentTag==BABYQUIT) {
            break; //breaks from loop to quit
         }
         int sendResponse[1] = {5}; //sends response to say that it is done, 5 is arbitrary
         MPI_Send(sendResponse,1,MPI_INT,0,BABYREADY,MPI_COMM_WORLD); 
      }
      MPI_Finalize(); //finalize MPI and quit
   }
}

int query1(babyDataStore bds,int numTimes) {
   dataEntry* babyArr = bds.data; //this checks to see if a word appeared more than some times, then saves doc count
   int numDocuments = 0;
   int mostRecentDocFound = -1;
   for(int i =0; i < bds.count; i++) {
      if(babyArr[i].docNum==mostRecentDocFound) {
         continue; //makes sure to not add a doc more than once, just in case
      }
      if(babyArr[i].count>numTimes) {
         numDocuments++;
         mostRecentDocFound = babyArr[i].docNum;
      }
   }
   return numDocuments;
}

int query3(babyDataStore bds,int vocabWord) { //query 3 here, counts the num times a word appears
   dataEntry* babyArr = bds.data;
   int sumCount = 0;
   for(int i = 0; i < bds.count; i++) {
      if(babyArr[i].wordNum==vocabWord) {
         sumCount = sumCount + babyArr[i].count;
      }
   }
   return sumCount; //pretty straightforward, counts num times word appears and returns it
}

int* query2(babyDataStore bds,int numTimes,int arrSize) { //start query 2
   //what words in X are used more than ___ times in any document?
   dataEntry* babyArr = bds.data;
   int* returnArr;
   returnArr = new int[arrSize];
   int indexVar = 1;
   for(int i = 0;i<bds.count; i++) { //for the length of the baby data, run through
      if(babyArr[i].count>numTimes) {
         if(indexVar>=arrSize) { //if it is too small, expand the array
            returnArr = expandIntArray(returnArr,arrSize);
            int newArrSize = arrSize/4;
            arrSize = arrSize + newArrSize;
         }
         //check for duplicate values before adding, ugh
         bool alreadyExists = false;
         for(int k = 0; k < indexVar;k++) {
            if(returnArr[k]==babyArr[i].wordNum) {
               alreadyExists = true;
               break;
            }
         }
         if(!alreadyExists) {
            returnArr[indexVar] = babyArr[i].wordNum;
            indexVar++;
         }
      }
   }
   returnArr[0] = indexVar; //stores array length at index 0
   return returnArr; //returns the info
}

int* getDocumentPayloadArr(int rank, int numProcess, allDatabases dbNumber,int* docStartLocations) {
   //start doc number, end doc number, starting doc byte, dataset label, starting size of arr (non zero counts / num babies)
   int* returnArr = new int[5];
   string inputDB = documentFileNames[dbNumber];
   int numBabyNodes = numProcess - 1; //removes grendel from the divison
   int numDocuments = getDocumentCount(dbNumber); 
   int constantNumDocs = numDocuments/numBabyNodes; //for determining the num to give to each baby
   int totalEntryCount = getEntriesLength(dbNumber);
   if(rank==1) { //gives baby 1 from doc 1
      returnArr[0] = 1;
      returnArr[1] = constantNumDocs;
   }
   else if (rank == numBabyNodes) { //last baby has to get the extra scraps
      returnArr[0] = (constantNumDocs * (rank - 1))+1;
      returnArr[1] = numDocuments;
   }
   else { //rank is in the middle
      returnArr[0] = (constantNumDocs * (rank - 1))+1;
      returnArr[1] = constantNumDocs * rank;
   }
   returnArr[2] = docStartLocations[returnArr[0]];
   returnArr[3] = dbNumber;
   returnArr[4] = (totalEntryCount/numBabyNodes);
   return returnArr; //returns the array with all info for the babies
}

int getVocabularyLength(allDatabases dbNumber) { //gets the length of the vocabulary
   string fileName = documentFileNames[dbNumber];
   string fullFileName = "docword."+fileName+".txt";
   string currentFile = mainDir + fullFileName;
   ifstream docwordF(currentFile.c_str());
   string line;
   getline(docwordF, line);//docs in set
   getline(docwordF, line);//words in set
   int wordsInSet = atoi(line.c_str());
   docwordF.close();
   return wordsInSet; //returns vocab len
}

int getEntriesLength(allDatabases dbNumber) { //gets and returns num entries in a dataset
   string fileName = documentFileNames[dbNumber];
   string fullFileName = "docword."+fileName+".txt";
   string currentFile = mainDir + fullFileName;
   ifstream docwordF(currentFile.c_str());
   string line;
   getline(docwordF, line);//docs in set
   getline(docwordF, line);//words in set
   getline(docwordF, line);//entries in set
   int wordsInSet = atoi(line.c_str());
   docwordF.close();
   return wordsInSet; //returns num of entries
}

int getDocumentCount(allDatabases dbNumber) { //gets and returns num documents in set
   string fileName = documentFileNames[dbNumber];
   string fullFileName = "docword."+fileName+".txt";
   string currentFile = mainDir + fullFileName;
   ifstream docwordF(currentFile.c_str());
   string line;
   getline(docwordF, line);//docs in set
   int wordsInSet = atoi(line.c_str()); //returns the int of docs in set
   docwordF.close();
   return wordsInSet;
}

string* getVocabArray(allDatabases dbNumber) {
   string inputDB = documentFileNames[dbNumber];
   string vocabFile = "vocab."+inputDB+".txt";
   string* vocabulary = new string[getVocabularyLength(dbNumber)+1]; //creates array for holding vocab
   string currentFile = mainDir + vocabFile;
   ifstream vocabF(currentFile.c_str());
   int j = 1;
   string vocabWord;
   while (getline(vocabF, vocabWord)) { //for reading each word in the file and storing it in an array
         vocabulary[j] = vocabWord;
         j++;
   }
   vocabF.close();
   return vocabulary;
}

babyDataStore readEntries(int dbNumber,int startDoc, int endDoc, int seekByte, int arrSize) {
   //reads the actual entries from the docword file and stores in a struct
   babyDataStore bds; //creates struct to hold all the entries
   string inputDB = documentFileNames[dbNumber];
   string docFile = "docword."+inputDB+".txt";
   string currentFile = mainDir + docFile;
   ifstream docwordF(currentFile.c_str());
   docwordF.seekg(seekByte);//seeks to location quicky, optimizes execution
   int i = 0;
   string line;
   dataEntry * returnArr = new dataEntry[arrSize];
   while (getline(docwordF, line)) {
         if(i >= arrSize) {
            returnArr = expandArray(returnArr,arrSize); //expands array
            int newArrSize = arrSize / 4; //controls the size of array value
            arrSize = arrSize + newArrSize;
         }
         istringstream iss(line);
         string a, b, c;
         iss >> a >> b >> c;
         int currentDocNum = atoi(a.c_str());
         if(currentDocNum>endDoc) {
            break;
         }
         returnArr[i].docNum = currentDocNum; //reads the actual entry and adds to a entry struct
         returnArr[i].wordNum = atoi(b.c_str());
         returnArr[i].count = atoi(c.c_str());
         i++;
   }
   bds.data = returnArr;
   bds.count = i;
   docwordF.close();
   return bds;
}

dataEntry* expandArray(dataEntry* inputArr, int arrSize) {
   int newArrSize = arrSize / 4;
   newArrSize = arrSize + newArrSize;
   dataEntry* returnArr = new dataEntry[newArrSize];
   for(int i =0; i < arrSize; i++) {
      returnArr[i] = inputArr[i];
   }
   delete [] inputArr; //deletes old array from memory
   return returnArr; //returns an expanded data entry array
}

int* expandIntArray(int* inputArr, int arrSize) { //expands the size of int array
   int newArrSize = arrSize / 4;
   newArrSize = arrSize + newArrSize;
   int* returnArr = new int[newArrSize];
   for(int i =0; i < arrSize; i++) {
      returnArr[i] = inputArr[i];
   }
   delete [] inputArr; //deletes old array from memory
   return returnArr; //returns new one
}

int* readDocStart(allDatabases dbNumber){ //reads the doc start file based on DB
   string fileName = documentFileNames[dbNumber];
   string docFile = "docstart."+fileName+".txt"; //gets the file
   string currentFile = mainDir + docFile;
   ifstream docwordF(currentFile.c_str());
   string line;
   //for reading the docword file
   int * returnArr = new int[getDocumentCount(dbNumber)+1]; //makes sure array is big enough
   while (getline(docwordF, line)) {
         istringstream iss(line);
         string a, b;
         iss >> a >> b;
         int docNum, byteNum;
         docNum = atoi(a.c_str()); //breaks out and stores information
         byteNum = atoi(b.c_str());
         returnArr[docNum] = byteNum;
   }
   docwordF.close();
   return returnArr; //returns an array with docnum and start byte
}




	

