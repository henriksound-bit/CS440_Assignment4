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

    // Create the index
    //cout << sizeof(Record) << endl;

    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");

    string userInput;
    bool quit = true;

    // Loop to lookup IDs until user is ready to quit

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


    return 0;
}
