/*
Skeleton code for storage and buffer management
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


int main() {
    // Create the EmployeeRelation file from Employee.csv
    StorageBufferManager manager("EmployeeRelation.dat");
    manager.createFromFile("Employee.csv");
    
    // Loop to lookup IDs until user is ready to quit
    int id;
    while (true) {
        cout << "Enter ID to look up (or enter a non-integer to quit): ";
        cin >> id;

        // If the input is not int, break which exits
        if (cin.fail()) {
            break;
        }

        manager.findRecordById(id);
    }

    return 0;
}