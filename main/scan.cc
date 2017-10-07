#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>

using std::vector;
using std::cout;
using std::string;
using std::endl;
using std::unordered_map;
using std::unique_ptr;

class RowTuple {
  public:
    RowTuple() {};
    RowTuple(unordered_map<string, string> input_map) : row_data(std::move(input_map)) {}

    bool is_empty() {
      return row_data.empty();
    }

    void print_contents() {
      if (row_data.empty()) {
        cout << "Column: None" << endl;
        cout << "Value: None" << endl;
        return;
      }

      for (const auto& it : row_data) {
        cout << "Column: " << it.first << endl;
        cout << "Value: " << it.second << endl;
      }
    }

  private:
    unordered_map<string, string> row_data;
};

class Iterator {
  public:

    virtual void init() = 0;
    virtual void close() = 0;
    virtual RowTuple get_next() = 0;

    void set_inputs(vector<unique_ptr<Iterator>> inputs) {
      this->inputs = std::move(inputs);
    }

    void append_input(unique_ptr<Iterator> new_input) {
      inputs.push_back(std::move(new_input));
    }

  protected:
    vector<unique_ptr<Iterator>> inputs; //inputs = some other vector -> assignment op
};

// Note I believe Scan is an input / parent for Select in this example
// FLOW: SELECT calls get_next() of scan, which is an input, applies a predicate and keeps
// calling next on scan until select's predicate returns true
class FileScan : public Iterator {
  public:

    FileScan() {}

    FileScan(string file_name) {
      this->file_name = file_name;
    }

    void init() {
      cout << "Scan Node Initiated" << endl;
      records.clear();
      iterator_position = 0;
      read_data();
    }

    void close() {
      cout << "Scan node closed" << endl;
    }

    RowTuple get_next() {
      if (iterator_position >= records.size()) {
        RowTuple empty_tuple;
        return empty_tuple;
      }
      // Calls copy constructor
      RowTuple row_tuple = records[iterator_position]; // same as RowTuple row_tuple(records[iterator_pos])
      iterator_position++;
      return row_tuple;
    }

  private:
    vector<RowTuple> records;
    unsigned int iterator_position = 0;
    string file_name = "";


    void read_data() {
      cout << "Reading in data from scan node" << endl;
      if (!records.empty()) { records.clear(); }

      cout << "Inserting fake records " << endl;
      records.push_back(RowTuple({{"student", "Jimmy Cricket"}, {"Sport", "Tennis"}, {"id", "2"}}));
      cout << "Records read in" << endl;
    }
};

class Select : public Iterator {
  public:

    void init() {}

    void close() {}

    void set_input() {}

    void set_predicate(bool (*predicate) (RowTuple)) {}

    RowTuple get_next();
  private:
    bool (*predicate) (RowTuple);
};

/*** Implementations */

/*
void Select::set_input(Iterator *it) {
  input = it;
}

void Select::set_predicate(bool (*predicate) (RowTuple)) {
  this->predicate = predicate;
}

RowTuple Select::get_next() {
  RowTuple tuple;
  // Make sure you check that predicate and input are initialized
  // Not worrying about it right now
  while (!(tuple = input->get_next()).is_empty()) {
    if (predicate(tuple)) {
      return tuple;
    }
  }
  // Nothing in input satisfies predicate :(
  // Make sure this is doing what you want
  return RowTuple();
}
*/

int main() {
  cout << "Starting Main Function" << endl;
  FileScan scan;
  scan.init();
  RowTuple row_tuple;
  while ( !(row_tuple = scan.get_next()).is_empty()) {
    row_tuple.print_contents();
  }
  return (0);
}
