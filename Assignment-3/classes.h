
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <bitset>
#include <cmath>
using namespace std;

class Record {
public:
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    void print(){
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
        cout << "\n"; 
    }
};

struct Page {
    static const int PAGE_SIZE = 4096;
    vector<Record> records;
};

struct Bucket {
    static const int BLOCK_SIZE = 4096; // Defining the block size
    vector<Record> records;
    vector<Record> OFRecords;

    Bucket() {
        records.reserve(BLOCK_SIZE);
    }
};

class LinearHashIndex {
private:
    vector<Bucket> buckets;
    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used
    int i; // The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    string fName; // Name of index file
    ofstream dataFile;

    int hashFunction(int id){
        return id % 216;
    }

    void flushToFile(const Page& page, ofstream& outFile) {
        for (size_t i = 0; i < page.records.size(); ++i) {
            // Serializing to file
            outFile << page.records[i].id << "," 
                    << page.records[i].name << "," 
                    << page.records[i].bio << "," 
                    << page.records[i].manager_id << "\n";
        }

        // Calculate remaining space after serializing and fill with zeros
        int remainingSpace = Page::PAGE_SIZE - (page.records.size() * sizeof(Record));
        if (remainingSpace > 0) {
            std::vector<char> emptySpace(remainingSpace, 0);  // Creates vector of zeros
            outFile.write(emptySpace.data(), remainingSpace); // Write zeros to the file
        }
        outFile.flush();
    }

public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Start with 4 buckets in index
        fName = indexFileName;
        
        // Create your EmployeeIndex file and write out the initial 4 buckets
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately

        dataFile.open(fName, ios::binary | ios::app);

        if (!dataFile.is_open()) {
            cerr << "Error opening idx file: " << fName << "\n";
            exit(EXIT_FAILURE);
        }

        // Writing out initial 4 buckets
        buckets.resize(n);
        blockDirectory.resize(n);

        // Incrementing nextFreeBlock
        for (int j = 0; j < n; ++j) {
            blockDirectory[j] = j;
        }
    }
    
    ~LinearHashIndex() {
        if (dataFile.is_open()) {
            dataFile.close();
        }
    }
    
    void insertRecord(Record record) {
        int idx = hashFunction(record.id) & ((1 << i) - 1); // Get bucket index
        Bucket& bucket = buckets[idx];

        // Check for duplicate entries prior to insertion, reused logic from findByRecordID

        // Concatenate the original records and overflow records
        vector<Record> allRec;
        allRec.reserve(bucket.records.size() + bucket.OFRecords.size());
        allRec.insert(allRec.end(), bucket.records.begin(), bucket.records.end());
        allRec.insert(allRec.end(), bucket.OFRecords.begin(), bucket.OFRecords.end());

        // Search for the record in the concatenated vector
        auto found = find_if(allRec.begin(), allRec.end(), [&](const Record& r) {
            return r.id == record.id;
        });

        if (found != allRec.end()) {
            cout << "Duplicate entry found for Employee ID: " << record.id << ", insertion will be skipped.\n";
            return; 
        }

        // Inserts record and checks for overflow
        handleOverflow(bucket, record);

        // Check the rehashing 
        if (n > pow(2, i)) {
            rehash(i, n, buckets, blockDirectory, idx, record);
        }
    }

    // Function to handle overflow scenario
    void handleOverflow(Bucket& bucket, const Record& record) {
        // Get total num of rec in both buckets
        const int allRec = bucket.records.size() + bucket.OFRecords.size();

        // Checking all rec vs block threshold for distribution
        if (allRec >= Bucket::BLOCK_SIZE) {
            redistributeRecords(bucket);

            // Add the current record to overflow
            bucket.OFRecords.push_back(record);
        } 
        else {
            // Adds record to main bucket
            bucket.records.push_back(record);
        }
    }

    void redistributeRecords(Bucket& bucket) {
        // Get current bucket sizes
        const int bucketSize = bucket.records.size();
        const int OFSize = bucket.OFRecords.size();

        // Checking for redistribution
        if (bucketSize < OFSize) {
            
            // Moving half of overflow 
            const int recToMove = OFSize / 2; 

            // Set iterators to section out the recs to be moved into main bucket
            vector<Record>::iterator start = bucket.OFRecords.begin() + (OFSize - recToMove);
            vector<Record>::iterator end = bucket.OFRecords.end();

            // Moved OF section into bucket
            bucket.records.insert(bucket.records.end(), start, end);

            // Deleted the stuff we moved from OF bucket
            bucket.OFRecords.erase(start, end);
        }
    }


    // Function to perform rehashing
    void rehash(int& i, int n, vector<Bucket>& buckets, vector<int>& blockDirectory, int idx, const Record& record) {
        int prevIdx;
        i++;
        buckets.resize(n);

        // Temporary record vector to hold all bucket's content
        vector<Record> temp;

        // Move records from each bucket to temp
        for (size_t idx = 0; idx < buckets.size(); ++idx) {
            Bucket& bucket = buckets[idx];

            // Moving all buckets into temp mega bucket
            std::move(bucket.records.begin(), bucket.records.end(), std::back_inserter(temp));
            std::move(bucket.OFRecords.begin(), bucket.OFRecords.end(), std::back_inserter(temp));

            // Dumping buckets for later redistribution
            bucket.records.clear();
            bucket.OFRecords.clear();
        }

        // Updating dirc for where new i points
        blockDirectory.clear();
        for (int j = 0; j < n; ++j) {
            blockDirectory.push_back(j % (1 << i));
        }

        // Putting the buckets back together with the content
        for (size_t idx = 0; idx < temp.size(); ++idx) {
            const Record& tempRec = temp[idx];

            // Get new bucket index and updated i
            int newIdx = hashFunction(tempRec.id) & ((1 << i) - 1);

            // Filling record back into bucket
            buckets[newIdx].records.push_back(tempRec);
        }

        // Cleaning out bucket's old data
        for (int j = 0; j < n; ++j) {
            if (j != idx) {
                Bucket& prevBucket = buckets[j];

                 // Erase records from the previous bucket that match the conditions
                prevBucket.records.erase(
                    std::remove_if(prevBucket.records.begin(), prevBucket.records.end(), [&](const Record& r) {
                        // Get old bucket index
                        prevIdx = hashFunction(r.id) & ((1 << (i - 1)) - 1);

                        // Checking if id in prev bucket matches triggering bucket
                        return prevIdx == j && r.id == record.id;
                    }),
                    prevBucket.records.end()
                );
            }
        }
    }

    void writeToFile() {
        // Create temp to hold block directory
        Page temp;

        // Iterate through the buckets and the overflow 
        for (int blockIdx = 0; blockIdx < n; ++blockIdx) {
            
            // Get bucket index for current block and its bucket
            int idx = blockDirectory[blockIdx];
            Bucket& bucket = buckets[idx];

            // Pour records from bucket to temp page
            for (size_t idx = 0; idx < bucket.records.size(); ++idx) {
        
                temp.records.push_back(bucket.records[idx]);

                // Write bucket to file when temp page full
                if (temp.records.size() * sizeof(Record) >= Page::PAGE_SIZE) {
                    
                    flushToFile(temp, dataFile);
                    temp.records.clear(); 
                }
            }

            // Pouring records from OF bucket to temp page
            for (size_t idx = 0; idx < bucket.OFRecords.size(); ++idx) {

                temp.records.push_back(bucket.OFRecords[idx]);

                // Write OF bucket to file when temp page full
                if (temp.records.size() * sizeof(Record) >= Page::PAGE_SIZE) {

                    flushToFile(temp, dataFile);
                    temp.records.clear(); 
                }
            }
        }

        // Dump extra records into temp to file
        if (!temp.records.empty()) {
            flushToFile(temp, dataFile);
            temp.records.clear();
        }
    }

    void createFromFile(string csvFileName) {
        ifstream inputFileStream(csvFileName);
        string currentLine;

        if (!inputFileStream) {
            cerr << "Error opening file: " << csvFileName << "\n";
            exit(EXIT_FAILURE);
        }

        while(getline(inputFileStream, currentLine)) {
            stringstream lineStream(currentLine);
            string fieldElement;
            vector<string> fieldValues;

            while(getline(lineStream, fieldElement, ',')){
                fieldValues.push_back(fieldElement);
            }
            if(fieldValues.size() != 4){
                cerr << "Invalid record in the file\n";
                continue;
            }
            Record record(fieldValues);
            insertRecord(record);
        }
        inputFileStream.close();
    }

    Record findRecordById(int id) {
        int idx = hashFunction(id) & ((1 << i) - 1);

        Bucket& bucket = buckets[idx];

        // Concatenate the original records and overflow records
        vector<Record> allRec;
        allRec.reserve(bucket.records.size() + bucket.OFRecords.size());
        allRec.insert(allRec.end(), bucket.records.begin(), bucket.records.end());
        allRec.insert(allRec.end(), bucket.OFRecords.begin(), bucket.OFRecords.end());

        // Search for the record in the concatenated vector
        auto found = find_if(allRec.begin(), allRec.end(), [&](const Record& r) {
            return r.id == id;
        });

        if (found != allRec.end()) {
            return *found;
        }

        // Returns blank record
        return Record({"-1", "N/A", "N/A", "-1"});
    }

};
