/**************************************
 * Assignment 2 CS 440
 * Names: Ishanshi Bhardwaj (bhardwai) and Johnny li (liJohnny)
 * Date: Tuesday 6th 2024
**************************************/

#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>
#include <cstring>
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

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }

    // Returns size of record for later calculations
    int findSize() {
        // Fields converted back to strings for serialized calcs
        string field_mid = to_string(manager_id);
        string field_id = to_string(id);

        return (field_id.size() + field_mid.size() + bio.size() + name.size() + 3);
    }
};


Record createRecord(string s) {
    vector <Record> temp;

    int id, manager_id;
        string name, bio;

    const char* stringToChar = s.c_str();

    char* substr = strdup(stringToChar);

    // Find first '$' where Name starts
    char* dollar1 = strchr(substr, '$');

    // Find second '$' where Bio start
    char* dollar2 = strchr(dollar1 + 1, '$');

    // Calculate the lengths of the name and bio
    size_t nameLength = dollar2 - (dollar1 + 1);
    size_t bioLength = strchr(dollar2 + 1, '$') - (dollar2 + 1);

    
    // Extracting fields between dollar signs
    name.assign(dollar1 + 1, nameLength);
    bio.assign(dollar2 + 1, bioLength);

    //initial goal to read all in one go, but sscanf can't do whitespace well
    sscanf(substr, "%d$%*[^$]$%*[^$]$%d", &id, &manager_id); 

    Record tempRecord(vector<string>{to_string(id), name, bio, to_string(manager_id)});
    // temp.push_back(tempRecord);

    return tempRecord;
}

class StorageBufferManager {
private:
    // Removed 4 bytes to allocate space for record amount in page
    const int BLOCK_SIZE = 4092; // initialize the block size allowed in main memory according to the question 
    FILE* datFile;
    int pageNum = 0;
    // You may declare variables based on your need 
    const int MAX_BUFFER_SIZE = 3 * BLOCK_SIZE;

    // Insert new record
    void insertRecord(vector<Record> record, int Record_count) {
        int num_rec = record.size();
        int recordOffset = 0;
        char buffer[4096] = {' '};
        size_t slotPos = sizeof(buffer) - sizeof(int);
        memcpy(buffer + slotPos, &num_rec, sizeof(int));

        fseek(datFile, 0, SEEK_END);

        size_t pos = 0;

        for (int i = 0; i < num_rec; i++) {

            // Concat structure of each record into a string [id$name$bio$mid]
            string recordString = to_string(record[i].id) + "$" +
                                  record[i].name + "$" +
                                  record[i].bio + "$" +
                                  to_string(record[i].manager_id);

            // Write the record to memory
            memcpy(buffer + pos, recordString.c_str(), recordString.length());
            pos += recordString.length();

            // Store size of current record
            int recordSize = record[i].findSize();

            //Store offset, size at end of page for current record (loaded forward)
            slotPos -= sizeof(int);
            memcpy(buffer + slotPos, &recordSize, sizeof(int));

            slotPos -= sizeof(int);
            memcpy(buffer + slotPos, &recordOffset, sizeof(int));

            // Update pos of offset for proceeding record
            recordOffset += record[i].findSize();
        }

        // Write memory to .dat file
        fwrite(buffer, sizeof(char), 4096, datFile);

        pageNum += 1;
    }

public:
    StorageBufferManager(string NewFileName) {
        int pageNum = 0;

        datFile = fopen(NewFileName.c_str(), "wb+");
        if (!datFile) {
            cout << "Error opening " << NewFileName << endl;
            exit(EXIT_FAILURE);
        }
    }

    ~StorageBufferManager() {
        if (datFile) {
            fclose(datFile);
        }
    }

    // Read csv file (Employee.csv) and add records to the (EmployeeRelation)
    void createFromFile(const string& csvFileName) {
    // Open input file stream
    ifstream inputFileStream(csvFileName);
    string currentLine;

    // Check if the file is successfully opened
    if (!inputFileStream) {
        cerr << "Error opening file: " << csvFileName << endl;
        exit(EXIT_FAILURE);
    }

    // Keep track of parsed records and memory limits
    vector<Record> recordBuffer;
    int currentMemoryUsage = 0;
    int recordCount = 0;

    while (getline(inputFileStream, currentLine)) {
        // Parse each line from the CSV file
        istringstream lineStream(currentLine);
        string fieldElement;
        vector<string> fieldValues;

        // Split the line into individual fields
        while (getline(lineStream, fieldElement, ',')) {
            fieldValues.push_back(fieldElement);
        }

        // Convert fields to a record
        Record currentRecord(fieldValues);

        // Calculate the size of the record with overhead
        const int recordSizeWithOverhead = currentRecord.findSize() + 8;

        // Check if adding the current record exceeds the page limit
        if (currentMemoryUsage + recordSizeWithOverhead >= BLOCK_SIZE) {
            // Insert records into storage and reset counters
            insertRecord(recordBuffer, recordCount);
            currentMemoryUsage = 0;
            recordBuffer.clear();
            recordCount = 0;
        }

        // Add the current record to the buffer
        recordBuffer.push_back(currentRecord);
        currentMemoryUsage += recordSizeWithOverhead;
        ++recordCount;
    }

    // Insert any remaining records into storage
    if (recordCount) {
        insertRecord(recordBuffer, recordCount);
        currentMemoryUsage = 0;
        recordBuffer.clear();
        recordCount = 0;
    }

    // Close the input file stream
    inputFileStream.close();
}

    // Given an ID, find the relevant record and print it
    void findRecordById(int id) {
        bool found = false;

        for (size_t pages = 0; pages < pageNum; ++pages) {
        // Read a block
        vector<Record> records = parsePage(pages);
            // Iterate through all records in the block
            for (size_t i = 0; i < records.size(); ++i) {

                // Check if the record's id matches the target id
                if (records[i].id == id) {
                    // Print the matching record :D
                    records[i].print();
                    found = true;
                    return; 
                }
            }
        }

        if (!found) {
            cout << "Record with ID " << id << " not found.\n" << endl;
        }
    }



    vector<Record> parsePage(size_t pages) {
        // Set the file pointer to the beginning of the specified page
        fseek(datFile, 4096 * pages, SEEK_SET);
        const int SIZE = 4097;
        // Read the content of the page into a buffer
        char buffer[SIZE] = {' '}; // Initialize buffer with a space
        size_t bytesRead = fread(buffer, 1, SIZE-1, datFile);

        // Get total number of records at end of the page
        int totalRecords;
        totalRecords = 0;
        const int SECOND_SIZE = 4092;

        memcpy(&totalRecords, buffer + SECOND_SIZE, sizeof(int));

        // Add null to buffer for str operations
        buffer[4097] = '\0';

        vector<Record> temp;

        // Variables to track record size, offset, and current record index
        int recordSize = 0;
        int recordOffset = 0;
        int recIndex = 1;

        // Loop through each record in the page
        for (int i = 0; i < totalRecords; i++) {
            // Extract record size and offset from the buffer
            memcpy(&recordSize, buffer + (SECOND_SIZE - (recIndex * 4)), sizeof(int));
            recIndex++;
            memcpy(&recordOffset, buffer + (SECOND_SIZE - (recIndex * 4)), sizeof(int));
            recIndex++;

            // Extract the record data from the buffer based on size and offset
            string s(buffer + recordOffset, recordSize);

            // Create a Record object from the extracted data and add it to the vector
            Record record = createRecord(s);
            temp.push_back(record);
        }

        // Return the vector of parsed records
        return temp;
    }
};
