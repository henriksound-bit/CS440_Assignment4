#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>
#include <cmath>
#include <cstdio>
#include <cstdint>

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

    Record()
    {
        id = -1;
        name = "";
        bio = "";
        manager_id = -1;
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }

    int calculate_record_size() {
        return 8 + 8 + bio.length() + name.length() + 4;
    }

    void write_record(fstream &index_file) {
        // Write ID
        ::int64_t ID_pad = (std::int64_t)id;
        ::int64_t manager_id_pad = (std::int64_t)manager_id_pad;

        index_file.write(reinterpret_cast<char *>(&ID_pad), sizeof(ID_pad));
        index_file << "~";
        index_file << name << "~";
        index_file << bio << "~";
        index_file.write(reinterpret_cast<char *>(&manager_id_pad), sizeof(manager_id_pad));
        index_file << "~";
    }
};

class Block {
private:
    const int BLOCK_SIZE = 4096;

public:
    vector<Record> records; // Records in the block
    int block_size; // Size of the block in bytes
    int overflow_ptr_idx; // Index of the overflow block in the block directory
    int num_records; // Number of records in the block
    int block_idx; // Offset index to block location in index file

    Block() {
        block_size = 0;
        block_idx = -1;
    }

    Block(int physical_idx) {
        block_size = 0;
        block_idx = physical_idx;
    }
    
    // Reead in a record from from the index file
    void read_record(fstream &input_file) {

        // Get 8 byte ints to store 8 byte ints in file to eventually convert to 4 byte ints in Record
        ::int64_t ID_pad;
        ::int64_t manager_id_pad;
        string name, bio;

        input_file.read(reinterpret_cast<char *>(&ID_pad), sizeof(ID_pad));
        // Ignore delimiter
        input_file.ignore(1, '~');
        getline(input_file, name, '~');
        getline(input_file, bio, '~');
        input_file.read(reinterpret_cast<char *>(&manager_id_pad), sizeof(manager_id_pad));
        // Ignore delimiter after binary int
        input_file.ignore(1, '~');
        
        cout << "Read In: " << ID_pad << " " << name << " " << bio << " " << manager_id_pad << endl;
        
        int regular_id = (int)ID_pad;
        int regular_manager_id = (int)manager_id_pad;
        vector<string> fields = {to_string(regular_id), name, bio, to_string(regular_manager_id)};
    }
    
    void read_block(fstream &input_file) {
        // Get first physical block
        input_file.seekg(block_idx * BLOCK_SIZE);
        
        // Read in all blocks intitialized with overflow pointer and num_records
        input_file.read(reinterpret_cast<char *>(&overflow_ptr_idx), sizeof(overflow_ptr_idx));
        input_file.read(reinterpret_cast<char *>(&num_records), sizeof(num_records));
        
        cout << "Read Block: " << endl;
        cout << "Overflow Index: " << overflow_ptr_idx << endl;
        cout << "Num Records: " << num_records << endl;
        
        // Add (4+4=8) to block size for idx and num_records
        block_size += 8;
        
        // Read in all records
        for (int i = 0; i < num_records; i++) {
            read_record(input_file);
            // Add 8 bytes for each record
            block_size += records[i].calculate_record_size();
            cout << "Block Size: " << block_size << endl;
        }
        cout << "* * * *\n" << endl;
    }
};

class LinearHashIndex {

private:
    const int BLOCK_SIZE = 4096;

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
    // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
    // can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used (number of blocks)
    int i;    // The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords;    // Records currently in index. Used to test whether to increase n (splitpoint?)
    int nextFreeBlock; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;      // Name of index file
    // Additional vars
    int num_overflow_blocks;
    int curr_size_total;
    int numBuckets; // Bucket point to blocks, block holds records

    int hash_function(int id) {
        return (id % (int) pow(2, 8));
    }

    int get_last_i_bits(int hash_val, int i) {
        return hash_val & ((1 << i) - 1); // Get last i'th bits of hash value
    }

    // Get record from input file
    Record Grab_Record(fstream &empin) {
        string line, word;
        vector<string> fields;
        // grab entire line
        if (getline(empin, line, '\n')) {
            // turn line into a stream
            stringstream s(line);
            // gets everything in stream up to comma
            getline(s, word,',');
            fields.push_back(word);
            getline(s, word, ',');
            fields.push_back(word);
            getline(s, word, ',');
            fields.push_back(word);
            getline(s, word, ',');
            fields.push_back(word);

            return Record(fields); // This should store everything into appropriate fields?

        }
        else {
            fields.push_back("-1");
            fields.push_back("-1");
            fields.push_back("-1");
            fields.push_back("-1");
            return Record(fields);
        }
    }

    int init_bucket(fstream &index_file) {

        int overflow_idx = -1;
        int default_num_records = 0;

        // Write overflow pointer and num_records to blocks at the buckets
        index_file.write(reinterpret_cast<char *>(&overflow_idx), sizeof(overflow_idx));
        index_file.write(reinterpret_cast<char *>(&default_num_records), sizeof(default_num_records));

        blockDirectory.push_back(nextFreeBlock++); // Update number of buckets as they are created. Maintain the index
        numRecords++; // based on number of blocks so we allocate more blocks outside of this function, so that we can
        numBuckets++; // get the last free block index for the bucket.
        // Update current total size
        curr_size_total += sizeof(overflow_idx) + sizeof(default_num_records);

        return blockDirectory.size() - 1;

    }

    int init_overflow_block(int parent_block_idx, fstream &index_file) {

        int overflow_idx = -1;
        int default_num_records = 0;

        int curr_block_idx = nextFreeBlock++; // Index of overflow block
        // Write initial values to initial overflow block
        index_file.seekp(curr_block_idx * BLOCK_SIZE);

        // Write superceding block overflow index to point here by overwriting first 4 bytes with current block index
        index_file.write(reinterpret_cast<char *>(&overflow_idx), sizeof(overflow_idx));
        index_file.write(reinterpret_cast<char *>(&default_num_records), sizeof(default_num_records));

        index_file.seekg(parent_block_idx * BLOCK_SIZE);
        index_file.write(reinterpret_cast<char *>(&nextFreeBlock), sizeof(nextFreeBlock));
        // Update number of blocks
        numRecords++;
        num_overflow_blocks++;

        curr_size_total += sizeof(overflow_idx) + sizeof(default_num_records);
        // Return index of overflow block
        return curr_block_idx;

    }

    int init_empty_block(fstream &index_file) {
        int overflow_idx = -1;
        int default_num_records = 0;

        // Get index for current block
        int curr_block_idx = nextFreeBlock++;

        index_file.seekp(curr_block_idx * BLOCK_SIZE);

        // Write overflow pointer and num_records to blocks at the buckets
        index_file.write(reinterpret_cast<char *>(&overflow_idx), sizeof(overflow_idx));
        index_file.write(reinterpret_cast<char *>(&default_num_records), sizeof(default_num_records));

        // Update number of blocks
        numRecords++;
        // Update current total size
        curr_size_total += sizeof(overflow_idx) + sizeof(default_num_records);
        return curr_block_idx;
    }

    void write_record_to_idx(Record record, int bblock_idx, fstream &index_file) {

        bool written_record = false;

        while (!written_record) {
            // Read block that was indexed
            Block current_block(bblock_idx);
            current_block.read_block(index_file);

            // Check if block has capacity for current reecord, otherwise look for overflow, if no overflow create one
            if (current_block.block_size + record.calculate_record_size() <= BLOCK_SIZE) {

                // Write the record into the block
                index_file.seekp((bblock_idx * BLOCK_SIZE) + current_block.block_size);
                record.write_record(index_file);

                int temp_num_record = current_block.num_records + 1;
                // Go to block start and overwritea index to write at number of records field
                index_file.seekp(bblock_idx * BLOCK_SIZE + sizeof(int));
                index_file.write(reinterpret_cast<char *>(&temp_num_record), sizeof(temp_num_record));

                // Update total size
                curr_size_total += record.calculate_record_size();
                written_record = true;
            }
            else if (current_block.overflow_ptr_idx != -1) {
                // Current block is full, but there is an overflow block, so go to that block
                // Set base block index to overflow block index
                bblock_idx = current_block.overflow_ptr_idx;
            }
            else {
                // Create overflow block, initialize it, and return offset index
                int overflow_block_idx = init_overflow_block(bblock_idx, index_file);
                // Go to overflow block and write record after overflow index and num_records
                index_file.seekp((overflow_block_idx * BLOCK_SIZE) + sizeof(int) + sizeof(int));
                record.write_record(index_file);
                // Update block's num_records
                int new_num_records = 1;
                // Got to block start and overwrite index to write at number of records field
                index_file.seekp(overflow_block_idx * BLOCK_SIZE + sizeof(int));
                index_file.write(reinterpret_cast<char *>(&new_num_records), sizeof(new_num_records));
                // Update total size
                curr_size_total += record.calculate_record_size();
                written_record = true;
            }
        }
    }

    void PrintIDs(ifstream &empin) {
        string line, word;
        std::vector<string>  fields(4, "");
        // grab entire line
        while (getline(empin, line, '\n')) {
            // turn line into a stream
            stringstream s(line);
            // gets everything in stream up to comma
            getline(s, word,',');
            cout << word << endl;
        }
        return;
    }

    void printRecord(Record temp)
    {
        cout << "Employee: " << temp.name << " | ID number: " << temp.id << " | Manager ID: " << temp.manager_id << "\nbio: " << temp.bio << "\n\n";
    }
    void writeRecord(Record temp, ofstream file)
    {
        file << temp.id << temp.name << temp.bio  << temp.manager_id << endl;
    }

    void insertRecord(Record record, fstream &index_file) {

        // No records in index, so create first block
        if (numRecords == 0) {
            // Initialize first block
            for (int i = 0; i < 2; i++) {
                init_bucket(index_file);
            }
            i = 1;// Single bit to address 2 blocks

        }

        // Add record to index in appropriate block
        int bucket_idx = get_last_i_bits(hash_function(record.id), i);

        // If last i bits >= n set MSB from 1  to 0
        if (bucket_idx >= numBuckets) {
            bucket_idx &= ~(1 << (i-1));
        }
        
        // Insert in index file at bucket index
        int block_idx = blockDirectory[bucket_idx];
        cout << "Offset of index of bucket index" << bucket_idx << " is " << block_idx << endl;

        write_record_to_idx(record, block_idx, index_file);
        // Increase num records
        numRecords++;
        
        double avg_bucket_capacity = (double)curr_size_total / (double)numRecords;
        
        if (avg_bucket_capacity > 0.7 * (double)BLOCK_SIZE) {
            
            int new_bucket_idx = init_bucket(index_file);
            // Calculate number of digits to address new bucket
            int new_bucket_address = (int)ceil(log2(numBuckets));
            // Rehash search keys into new bucket
            int move_records_bucket = new_bucket_idx;
            move_records_bucket &= ~(1 << (new_bucket_address-1));
            
            // Ghost bucket is now real
            int move_records_bucket_idx = blockDirectory[move_records_bucket];
            
            int old_bucket_idx = init_empty_block(index_file);
            
            while (move_records_bucket_idx != -1) {
                // Read block that was indexed
                Block old_block(move_records_bucket_idx);
                old_block.read_block(index_file);

                // Parsed entire block
                index_file.seekp(old_block.block_idx * BLOCK_SIZE);
                index_file << string(BLOCK_SIZE, '*');

                // Decerement blocks and number of overflow blocks
                num_overflow_blocks--;
                n--;
                // Update size and number of records
                curr_size_total -= old_block.block_size;
                numRecords -= old_block.num_records;
                // Check each record in old block and determine which goes to which
                for (int i = 0; i < old_block.num_records; i++) {

                    if (get_last_i_bits(hash_function(old_block.records[i].id), new_bucket_address) == move_records_bucket) {
                        int new_bucket_block_idx = blockDirectory[new_bucket_idx];
                        // Write record to new bucket
                        write_record_to_idx(old_block.records[i], new_bucket_idx, index_file);
                    }
                    else {
                        int temp_old_block_idx = old_bucket_idx;
                        // Write record to old bucket
                        write_record_to_idx(old_block.records[i], temp_old_block_idx, index_file);
                    }
                    numRecords++;
                }
                // Move to overflow index
                move_records_bucket_idx = old_block.overflow_ptr_idx;
            }
            num_overflow_blocks++;
            blockDirectory[move_records_bucket] = old_bucket_idx;
            i = new_bucket_address;
        }
    }



public:
    LinearHashIndex(string indexFileName) {
        n = 0; // Start with 4 buckets in index
        i = 0; // Need 2 bits to address 4 buckets
        numRecords = 0;
        numBuckets = 0;
        fName = indexFileName;
        num_overflow_blocks = 0;
        curr_size_total = 0;
        nextFreeBlock = 0;

    }



    // Read csv file and add records to the index
    // Read csv file and add records to the index
    void createFromFile(string csvFName) {

        fstream index_file(fName, ios::in | ios::out | ios::trunc | ios::binary);
        fstream input_file(csvFName, ios::in);

        if (input_file.is_open()){
            cout << "File is open" << endl;
        }

        // Loop through input file and get records

        bool records_left = true;

        while (records_left) {

            Record temp = Grab_Record(input_file);
            // Check if more records to grab
            if (temp.id == -1) {
                records_left = false;
            }
            else {
                insertRecord(temp, index_file);
            }

        }
        // Close fstreams
        index_file.close();
        input_file.close();
    }


    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {

        fstream index_file(fName, ios::in);
        // Calculate bucket index, get last i bits, insert before search so assum i s correct address
        int bucket_idx = get_last_i_bits(hash_function(id), i);
        // If record is going to real or fake bucket
        if (bucket_idx >= numBuckets) {
            bucket_idx &= ~(1 << (i-1));
        }
        // Go through block and find record
        int block_idx = blockDirectory[bucket_idx];

        while (block_idx != -1) {
            // Read block
            Block current_block(block_idx);
            current_block.read_block(index_file);

            // Check each record in block
            for (int i = 0; i < current_block.num_records; i++) {
                if (current_block.records[i].id == id) {
                    return current_block.records[i];
                }
            }
            // Go to block idx to overflow block
            block_idx = current_block.overflow_ptr_idx;
        }
        index_file.close();
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