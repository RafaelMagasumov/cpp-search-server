#include "read_input_functions.h"
#include <iostream>



int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}