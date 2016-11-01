// Test program that can be called by JSON test suite at
// https://github.com/nst/JSONTestSuite.

#include <fstream>
#include <iostream>
#include <string>
#include "univalue.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

using namespace std;

string ReadFile(const char* filename) {
    ifstream file(filename, ios::binary);
    if (!file) throw runtime_error("ifstream open failed");
    file.seekg(0, ios::end);
    ifstream::pos_type length = file.tellg();
    string str;
    str.resize(length);
    file.seekg(0, ios::beg);
    file.read(&str[0], length);
    return str;
}

int main (int argc, char *argv[])
{
    if (argc != 2) {
        cerr << "Usage: test_json filename" << endl;
        return 2;
    }

    try {
      UniValue val;
      string str = ReadFile(argv[1]);
      if (val.read(str)) {
        cout << val.write(1 /* prettyIndent */, 4 /* indentLevel */) << endl;
        return 0;
      } else {
        cerr << "JSON Parse Error." << endl;
        return 1;
      }

    } catch (const std::runtime_error& e) {
        cerr << "Error" << e.what() << endl;
        return 3;
    }
}
