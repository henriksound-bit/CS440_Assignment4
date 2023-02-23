#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>

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
};


class LinearHashIndex {

private:
    const int BLOCK_SIZE = 4096;

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used
    int i;	// The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords;    // Records currently in index. Used to test whether to increase n (splitpoint?)
    int nextFreeBlock; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;      // Name of index file

    int hash_function(int id) {
        return (id % (int)pow(2,8));
    }

    // Insert new record into index
    void insertRecord(Record record) {

        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)
            n = 4;
        }

        // Add record to the index in the correct block, creating a overflow block if necessary

        // Take neccessary steps if capacity is reached:
        if (numRecords==n) {
            // Increase n
            n = n * 2;
            // Increase i if necessary
            if (n > pow(2, i)) {
                i++;
            }
            // Place records in the new bucket that may have been originally misplaced due to a bit flip
            for (int j = 0; j < n; j++) {
                int bucket = blockDirectory[j];
                if (bucket == 0) {
                    // Read the bucket from the file
                    // Mmmmm why is Record not being recognized as a type?
                    Record *records = new Record(BLOCK_SIZE / sizeof(Record));
                    ifstream file(fName, ios::binary);
                    file.seekg(j * BLOCK_SIZE, ios::beg);
                    file.read((char *)records, BLOCK_SIZE);
                    file.close();

                    // Loop through the bucket and move any records that should be in the new bucket
                    for (int k = 0; k < BLOCK_SIZE / sizeof(Record); k++) {
                        Record record = records[k];
                        if (record.id != -1) {
                            // Calculate the hash value
                            int hash = hash_function(record.id);

                            // Determine if the record should be placed in the new bucket
                            if (hash >= n / 2) {
                                // Insert the record into the new bucket
                                insertRecord(record);

                                // Delete the record from the old bucket
                                Record empty; // Again, why is Record not being recognized as a type?
                                empty.id = -1;
                                ofstream file(fName, ios::binary);
                                file.seekp(j * BLOCK_SIZE + k * sizeof(Record), ios::beg);
                                file.write((char *)&empty, sizeof(Record));
                                file.close();
                            }
                        }
                    }

                    // Free the memory
                    delete[] records;
                }
            }
            // Increase nextFreeBlock by BLOCK_SIZE
            nextFreeBlock += BLOCK_SIZE;
        }
            // (i.e., if the record's hash value is now in the range of the new bucket, move it to the new bucket
            // increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        }


public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexFileName;

        // Create your EmployeeIndex file and write out the initial 4 buckets
        ofstream file(fName, ios::binary);
        file.close();

        // Initialize blockDirectory to a size of 256
        blockDirectory = vector<int>(256);
        // Initialize all values in blockDirectory to 0
        for (int i = 0; i < 256; i++) {
            blockDirectory[i] = 0;
        }
        // Write the first 4 buckets to the file
        for (int i = 0; i < 4; i++) {
            Record *records = new Record([BLOCK_SIZE / sizeof(Record)]);
            for (int j = 0; j < BLOCK_SIZE / sizeof(Record); j++) {
                Record empty;
                empty.id = -1;
                records[j] = empty;
            }
            // Write the bucket to the file
            ofstream file(fName, ios::binary | ios::app);
            file.write((char *)records, BLOCK_SIZE);
            file.close();
            // Free the memory
            delete[] records;
        }
        // Update blockDirectory to point to the first 4 buckets
        for (int i = 0; i < 4; i++) {
            blockDirectory[i] = i;
        }
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
        nextFreeBlock += 4 * BLOCK_SIZE;

      
    }



    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        // Open the csv file
        ifstream csvFile(csvFName);
        if (!csvFile.is_open()) {
            throw runtime_error("Could not open file");
        }

        // Read the csv file line by line
        string line;
        while (getline(csvFile, line)) {
            // Parse the line
            stringstream ss(line);
            string field;
            vector<string> fields;
            while (getline(ss, field, ',')) {
                fields.push_back(field);
            }

            // Create a record from the line
            Record record(fields);

            // Add the record to the index
            // Getting a warning here
            insertRecord(record); // My C++ is failing me, not sure why this function isn't being recognized
        }
        
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        // Calculate the hash value
        int hash = hash_function(id); // Why not recognize hash_function?

        // Determine which block to read
        int block = blockDirectory[hash]; // Uhhh vector indexing?

        // Read the block from the file
        Record *records = new Record[BLOCK_SIZE / sizeof(Record)];
        ifstream file(fName, ios::binary);
        file.seekg(block * BLOCK_SIZE, ios::beg);
        file.read((char *)records, BLOCK_SIZE);
        file.close();

        // Loop through the block to find the record
        for (int i = 0; i < BLOCK_SIZE / sizeof(Record); i++) {
            Record record = records[i];
            if (record.id == id) {
                return record;
            }
        }

        // Return a record with id -1 if the record was not found
        Record notFound;
        notFound.id = -1;
        return notFound;
    }

    // Given an ID, find the relevant record and print it
    Record findRecordByName(string name) {
        // Calculate the hash value
        int hash = hash_function(stoi(name));

        // Determine which block to read
        int block = blockDirectory[hash];

        // Read the block from the file
        Record *records = new Record[BLOCK_SIZE / sizeof(Record)];
        ifstream file(fName, ios::binary);
        file.seekg(block * BLOCK_SIZE, ios::beg);
        file.read((char *)records, BLOCK_SIZE);
        file.close();

        // Loop through the block to find the record
        for (int i = 0; i < BLOCK_SIZE / sizeof(Record); i++) {
            Record record = records[i];
            if (record.name == name) {
                return record;
            }
        }

        // Return a record with id -1 if the record was not found
        Record notFound;
        notFound.id = -1;
        return notFound;
    }
};