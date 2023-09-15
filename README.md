# Linear Hash Indexing Assignment for CS 440 Database Management Systems at Oregon State University

This application demonstrates the concept of Linear Hash Indexing. It reads data from a CSV file, creates an index, and allows the user to look up records by ID.
### Overview

The application is written in C++ and consists of several classes:

- `Record`: Represents a record in the database. Each record has an ID, name, bio, and manager ID.
- `Block`: Represents a block in the index. Each block contains a vector of records, the size of the block, an overflow pointer index, the number of records, and the block index.
- `LinearHashIndex`: Represents the linear hash index. It contains methods for creating the index from a CSV file, inserting records, and finding records by ID.
How it Works

The application starts by creating an instance of LinearHashIndex and populating it with data from a CSV file:
```c++
    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");
```
The user is then prompted to enter an ID to look up a record. The application continues to prompt the user until they enter "quit":
```c++
    while(quit)
    {
        cout << "\n\nLook employee up by ID (quit to exit):";
        cin >> userInput;

        while(cin.fail()) {
            cout << "Invalid ID Error\n";
            cin.clear();
            cin.ignore(256,'\n');
            cin >> userInput;
        }
        if (userInput == "quit")
        {
            cout << "Have nice day\n";
            return 0;
        }
        else
        {

            cout << "Looking up ID: " << stoi(userInput) << endl;
            Record target = emp_index.findRecordById(stoi(userInput));
            target.print();
        }

    }
```
The `LinearHashIndex` class uses a hash function to map IDs to buckets in the index:
```c++
    int hash_function(int id) {
        return (id % (int) pow(2, 8));
    }
```
When a record is inserted, the application checks if the block has enough capacity. If not, it creates an overflow block:
```c++

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
```
When a record is looked up by ID, the application calculates the bucket index, reads the block from the index, and loops through the block to find the record:
```c++
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
```
### Limitations
The application currently only supports looking up records by ID. It does not support deleting records or updating existing records. It also does not handle collisions in the hash function.
