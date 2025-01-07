#include "record_class.h"
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

//defines how many blocks are available in the Main Memory 
const int buffer_size = 22;
const int MAX_RECORDS_PER_RUN = 19;

bool compareRecords(const Records &r1, const Records &r2) {
  return r1.emp_record.eid < r2.emp_record.eid;
}

//PASS 1
//Sorting the buffers in main_memory and storing the sorted records into a temporary file (Runs) 
void Sort_Buffer(fstream &input, vector<fstream> &runs) {
  //Remember: You can use only [AT MOST] 22 blocks for sorting the records / tuples and create the runs
  vector<Records> buffer[buffer_size];
  int recordsWritten[buffer_size] = {0};

  // Read records and distribute them among buffers
  while (true) {
    bool anyBufferNotFull = false;

    // Distribute records to buffers
    for (int i = 0; i < buffer_size; i++) {
      Records rec = Grab_Emp_Record(input);

      if (rec.no_values == -1) {
        if (buffer[i].empty()) {
          // All records are read, and the buffer is empty
          return;
        } else {
          // Some records remain in the buffer; write them to a run
          sort(buffer[i].begin(), buffer[i].end(), compareRecords);
          string fileName = "run" + to_string(i) + ".txt";
          runs[i].open(fileName, ios::out);
          for (Records r : buffer[i]) {
            runs[i] << r.emp_record.eid << ","
                    << r.emp_record.ename << ","
                    << r.emp_record.age << ","
                    << fixed << setprecision(0)
                    << r.emp_record.salary << "\n";
            recordsWritten[i]++;

            // Check if the maximum records per run is reached
            if (recordsWritten[i] == MAX_RECORDS_PER_RUN) {
              runs[i].close();
              recordsWritten[i] = 0;  // Reset the counter
            }
          }
          runs[i].close();
          buffer[i].clear();
        }
      } else {
        buffer[i].push_back(rec);
        anyBufferNotFull = true;
      }
    }

    // Check if any buffer is not full, indicating the end of records
    if (!anyBufferNotFull) {
      return;
    }
  }
}


//PASS 2
//Use main memory to Merge the Runs 
//which are already sorted in 'runs' of the relation Emp.csv 
void Merge_Runs(vector<fstream> &runs, fstream &output) {
  vector<Records> buffer[buffer_size];

  // Initialize buffer with the first record from each run
  for (int i = 0; i < buffer_size; i++) {
    if (!runs[i].is_open()) {
      runs[i].open("run" + to_string(i) + ".txt", ios::in);
    }
    buffer[i].push_back(Grab_Emp_Record(runs[i]));
  }

  // Merge runs
  while (true) {
    int minIndex = -1;
    for (int i = 0; i < buffer_size; i++) {
      if (buffer[i].size() > 0 && (minIndex == -1 ||
                                   compareRecords(buffer[i][0], buffer[minIndex][0]))) {
        minIndex = i;
      }
    }

    if (minIndex == -1)
      break;

    output << buffer[minIndex][0].emp_record.eid << ","
           << buffer[minIndex][0].emp_record.ename << ","
           << buffer[minIndex][0].emp_record.age << ","
           << fixed << setprecision(0)
           << buffer[minIndex][0].emp_record.salary << "\n";

    buffer[minIndex].erase(buffer[minIndex].begin());

    if (buffer[minIndex].size() == 0) {
      Records r = Grab_Emp_Record(runs[minIndex]);
      if (r.no_values == -1) {
        runs[minIndex].close();
      } else {
        buffer[minIndex].push_back(r);
      }
    }
  }

  // Close remaining runs
  for (fstream &run : runs) {
    if (run.is_open()) {
      run.close();
    }
  }
}

int main() {

  //Open file streams to read and write
  //Opening out the Emp.csv relation that we want to Sort
  fstream input("Emp.csv", ios::in);
  if (!input) {
    cerr << "Error opening file" << endl;
    return 1;
  }

  // Calculate the total number of records
  int totalRecords = 0;
  while (Grab_Emp_Record(input).no_values != -1) {
    totalRecords++;
  }

  // Reset the file pointer to the beginning
  input.clear();
  input.seekg(0, ios::beg);

  //1. Create runs for Emp which are sorted using Sort_Buffer()
  vector<fstream> runs(buffer_size);

  Sort_Buffer(input, runs);

  //2. Use Merge_Runs() to Sort the runs of Emp relations 
  fstream output("EmpSorted.csv", ios::out);
  Merge_Runs(runs, output);

  output.close();

  //Please delete the temporary files (runs) after you've sorted the Emp.csv
  for (int i = 0; i < buffer_size; i++) {
        remove(("run" + to_string(i) + ".txt").c_str()); 
    }

  return 0;
}