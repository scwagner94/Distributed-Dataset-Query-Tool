//Sean Wagner
//CSC 330, Project 3
#ifndef READFILE_H
#define READFILE_H
using namespace std; 
// This is the content of the .h file, which is where the declarations go

struct dataEntry {//struct of data entry lines 
   int docNum;
   int wordNum;
   int count;
};
struct babyDataStore { //struct to hold all data entries and length
   int count;
   dataEntry* data;
};
struct queryResponse {//struct to hold a query response and its len
	int count;
	int* data;
};
enum allDatabases { KOS = 0, NYTIMES,ENRON,NIPS };//enumerated datasets
enum allTags { READDATA = 0,DATADONE,QUERYREQUEST,QUERYRESPONSE,QUERY3,QUERY1,QUERY2,QUERY4,BABYREADY,BABYQUIT,BABYDONE};//enumerated tags for the queries and responses

//reads all entries for the baby and gives it a struct of them
babyDataStore readEntries(int dbNumber,int startDoc, int endDoc, int seekByte, int arrSize) ;

//performs query 1, returns count of word in bds
int query1(babyDataStore bds,int numTimes);

//this gets the length of the vocabular for a dataset
int getVocabularyLength(allDatabases dbNumber);

//this gets the number of documents in a dataset
int getDocumentCount(allDatabases dbNumber);

//this gets the number of entries in a dataset
int getEntriesLength(allDatabases dbNumber);

//this reads the docstart file and returns array of the doc start bytes
int* readDocStart(allDatabases dbNumber);

//this determines the start and end doc number among other things for each of the babies
int* getDocumentPayloadArr(int rank, int numProcess, allDatabases inputDB,int* docStartLocations);

//this exapnds the array of entries storage
dataEntry* expandArray(dataEntry* inputArr, int newSize);

//this expands an int array
int* expandIntArray(int* inputArr, int arrSize);

//this gets an array of the vocabulary words for printing
string* getVocabArray(allDatabases dbNumber);

//this executes query 2, returns the array of word nums that meet criteria
int* query2(babyDataStore bds,int numTimes,int arrSize);

//this executes query 3 and 4, returns count of word in a bds
int query3(babyDataStore bds,int vocabWord);
#endif
