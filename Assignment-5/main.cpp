/**************************************
 * Assignment 3 CS 440
 * Names: Ishanshi Bhardwaj (bhardwai) and Johnny li (liJohnny)
 * Date: March 5th 2024
**************************************/


#include <bits/stdc++.h>
#include "record_class.h"
#include <fstream>
#include <sstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace std;

//defines how many blocks are available in the Main Memory 
#define buffer_size 22

Records buffers[buffer_size]; //use this class object of size 22 as your main memory

// Temp buffer struct to hold joining record
struct joining {
    
    // Dept record content
    int did;
    string dname;
    double budget;
    int managerid;

    // Emp record content
    // Removing eid since redundant due to managerid
    string ename;
    int age;
    double salary;
};

/***You can change return type and arguments as you want.***/

//Sorting the buffers in main_memory and storing the sorted records into a temporary file (runs) 
void Sort_Buffer(int records, int runs, const string& name) {
    //Remember: You can use only [AT MOST] 22 blocks for sorting the records / tuples and create the runs

    // Creating temp file name using input filename and current run num
    string tempStr = name + "Run" + to_string(runs) + ".txt";

    // Opening new file made from tempStr filename
    ofstream temp(tempStr);

    // Sorting entire buffer array of records 
    sort(buffers, buffers + records, [&](const Records &x, const Records &y) {

        // Separate sorting rules based on the filename

        // Sorts department records
        if (name == "d") {
            return x.dept_record.managerid < y.dept_record.managerid;

        } 
        // Sorts employee records
        else if (name == "e") {
            return x.emp_record.eid < y.emp_record.eid;
        }
    });

    // Iterates through all records in the array to be stored in temp run file
    for (int i = 0; i < records; ++i) {
        
        // Specifically writes sorted department records to run file
        if (name == "d") {

            temp    << buffers[i].dept_record.did << ","
                    << buffers[i].dept_record.dname << ","
                    << fixed << setprecision(0)
                    << buffers[i].dept_record.budget << ","
                    << buffers[i].dept_record.managerid << "\n";

        } 

        // Writes sorted employee records to run file
        else if (name == "e") {

            temp << buffers[i].emp_record.eid << ","
                 << buffers[i].emp_record.ename << ","
                 << buffers[i].emp_record.age << ","
                 << fixed << setprecision(0)
                 << buffers[i].emp_record.salary << "\n";
                     
        }
    }
    temp.close();
}

//Prints out the attributes from empRecord and deptRecord when a join condition is met 
//and puts it in file Join.csv
void PrintJoin(ofstream& output, joining record) {

    // Printing joining records 
    // cout    << record.did << ","
    //         << record.dname << ","
    //         << fixed << setprecision(0)
    //         << record.budget << ","
    //         << record.managerid << ","
    //         << record.eid << ","
    //         << record.ename << ","
    //         << record.age << ","
    //         << fixed << setprecision(0)
    //         << record.salary << "\n";

    // Writing joining records to join.csv

    // Writing department record data
    output  << record.did << ","
            << record.dname << ","
            << fixed << setprecision(0)
            << record.budget << ","
            << record.managerid << ","

            // Writing employee record data
            << record.ename << ","
            << record.age << ","
            << fixed << setprecision(0)
            << record.salary << "\n";
}

//Use main memory to Merge and Join tuples 
//which are already sorted in 'runs' of the relations Dept and Emp 
void Merge_Join_Runs(int eRuns, int dRuns) {
    // Open output file for writing the joined records
    ofstream output("Join.csv");

    // Temporary buffer holding the joined record
    joining record;

    // Iterates through every department
    for (int j = dRuns - 1; j >= 0; --j) {

        // Iterates through every employee ID
        for (int i = eRuns - 1; i >= 0; --i) {
            // Grabbing file names to open
            string eFileStr = "eRun" + to_string(i) + ".txt";
            string dFileStr = "dRun" + to_string(j) + ".txt";
            
            // Opening files to read in the records to join
            fstream eFile(eFileStr, ios::in);
            fstream dFile(dFileStr, ios::in);


            // Temporary buffer holding single record being checked
            Records dRec, eRec;

            // Reading the first line of records from both department and employee files
            dRec = Grab_Dept_Record(dFile);
            eRec = Grab_Emp_Record(eFile);

            // Continues to checks for end of file within department and employee
            while (dRec.no_values != -1 && eRec.no_values != -1) {

                // Checks if the current department managerID matches current employee ID
                if (dRec.dept_record.managerid == eRec.emp_record.eid) {

                    // Stores matching department record data
                    record.did = dRec.dept_record.did;
                    record.dname = dRec.dept_record.dname;
                    record.budget = dRec.dept_record.budget;
                    record.managerid = dRec.dept_record.managerid;

                    // Stores matching employee record data
                    record.ename = eRec.emp_record.ename;
                    record.age = eRec.emp_record.age;
                    record.salary = eRec.emp_record.salary;

                    // Calls PrintJoin to write the joining record to join.csv
                    PrintJoin(output, record);

                    // Grabs next department for the current employee to continue checking
                    dRec = Grab_Dept_Record(dFile);
                } 

                // Continues to loop to check for other matching departments for current employee
                else if (eRec.emp_record.eid < dRec.dept_record.managerid) {
                    // Moves to next employee if current employee ID is larger than current manager ID
                    eRec = Grab_Emp_Record(eFile);
                } 

                else if (eRec.emp_record.eid > dRec.dept_record.managerid){
                    // Moves to next department if current manager ID is larger than current employee iD
                    dRec = Grab_Dept_Record(dFile);
                }
            }
            eFile.close();
            dFile.close();
        }
    }
    output.close();
}

int main() {

    //Open file streams to read and write
    //Opening out two relations Emp.csv and Dept.csv which we want to join
    fstream emp("Emp.csv", ios::in);
    fstream dept("Dept.csv", ios::in);

    // Designating flags to indicate end of files for department and employee files
    bool dEnd = false;
    bool eEnd = false;

    // Counters used to indicate number of runs generated from each file
    int eRuns = 0;
    int dRuns = 0;

    // Counter to keep track of current amount of records being read in
    int totalRec = 0;

    //1. Create runs for Dept and Emp which are sorted using Sort_Buffer()
    
    // Creating runs for department
    while (!dEnd) {

        //Reading department records into buffer
        for (int i = 0; i < buffer_size; i++) {
            buffers[i] = Grab_Dept_Record(dept);

            // Checking for end of department file
            if (buffers[i].no_values == -1) {
                
                // Indicate hit end of file, then break out of current loop
                dEnd = true;
                break;
            }
            totalRec++;
        }

        // Checking department records are read then create run
        if (totalRec > 0) {

            // Sort current buffer into a single run
            Sort_Buffer(totalRec, dRuns, "d");

            // Reset counter after sort
            totalRec = 0; 

            // Update run counter for file naming scheme
            dRuns++;
        }
    }

    // Creating runs for employee
    while (!eEnd) {

        //Reading employee records into buffer
        for (int i = 0; i < buffer_size; i++) {
            buffers[i] = Grab_Emp_Record(emp);
            if (buffers[i].no_values == -1) {
                eEnd = true;
                break;
            }
            totalRec++;
        }

        // Checking employee records are read then create run
        if (totalRec > 0) {
            // Same logic as above, except it's employee
            Sort_Buffer(totalRec, eRuns, "e");
            totalRec = 0; //reset counter after sort
            eRuns++;
            
        }
    }

    //2. Use Merge_Join_Runs() to Join the runs of Dept and Emp relations
    Merge_Join_Runs(eRuns,dRuns);

    emp.close();
    dept.close();

    //Please delete the temporary files (runs) after you've joined both Emp.csv and Dept.csv
    
    // Deleting department runs
    for (int i = 0; i < dRuns; i++) {
        remove(("dRun" + to_string(i) + ".txt").c_str()); 
    }

    // Deleting employee runs
    for (int i = 0; i < eRuns; i++) {
        remove(("eRun" + to_string(i) + ".txt").c_str()); 
    }

    return 0;
}
