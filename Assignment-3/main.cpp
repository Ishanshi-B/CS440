/*
Skeleton code for linear hash indexing
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;


int main(int argc, char* const argv[]) {

    // Error message guide for cmd line input
    if (argc < 2) {
        cout << "Missing id in command line\n\n";
        cerr << "Please use the correct format: " << argv[0] << " ID1 \n";
        cerr << "For additional IDs: " << argv[0] << " ID1 ID2 ID3...\n\n";
        return 1;
    }

    // Create the index
    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");
    emp_index.writeToFile();

    // Iterate through the command line arguments for each ID
    for (int i = 1; i < argc; ++i) {

        // Converting id to searchable format
        int id = stoi(string(argv[i]));
        Record found = emp_index.findRecordById(id);

        // Uses blank record id to check if found
        if (found.id != -1) {
                cout << "Employee " << i << " Found: \n";
                found.print(); // Print the found record
            } 
            else {
                cout << "Employee " << i << " with ID " << id << " could not be found.\n\n";
            }
    }
    
    // Loop to lookup IDs until user is ready to quit
    

    return 0;
}