#include <iostream>
#include <vector>
#include <unordered_map>

using std::vector;
using std::cout;
using std::string;
using std::endl;
using std::unordered_map;

class RowTuple {
  public:
    RowTuple() {};
    RowTuple(unordered_map<string, string> input_map) : row_data(std::move(input_map)) {}
    bool is_empty();
    void print_contents();
  private:
    unordered_map<string, string> row_data;
};

class Iterator {
  public:
    //virtual void init() = 0;
    virtual void close() = 0;
    virtual RowTuple get_next() = 0;
};

// Note I believe Scan is an input / parent for Select in this example
// FLOW: SELECT calls get_next() of scan, which is an input, applies a predicate and keeps
// calling next on scan until select's predicate returns true
class Scan : public Iterator {
  public:
    void init();
    void close();
    RowTuple get_next();
  private:
    vector<RowTuple> records;
    int iterator_position = 0;
    void read_data();
};

class Select : public Iterator {
  public:
    void init();
    void close();
    void set_input(Iterator *);
    void set_predicate(bool (*predicate) (RowTuple));
    RowTuple get_next();
  private:
    Iterator *input;
    bool (*predicate) (RowTuple);
};

/*** Implementations */

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
  return RowTuple();
}

void RowTuple::print_contents() {
  if (row_data.empty()) {
    cout << "Column: None" << endl; 
    cout << "Value: None" << endl;
  }

  for (const auto& it : row_data) { // very convenient
    cout << "Column: " << it.first << endl;
    cout << "Value: " << it.second << endl;
  }
}

// still works with map
bool RowTuple::is_empty() {
  return row_data.empty();
}

void Scan::init() {
  cout << "Scan Node Inited" << endl;
  records.clear();
  read_data();
}

void Scan::close() {
  cout << "Scan Node closed" << endl;
}

void Scan::read_data() {
  cout << "Reading in data from scan node" << endl;
  if (!records.empty()) {records.clear();}
  cout << "Emptied records" << endl;

  records.push_back(RowTuple({{"student", "Cameron"}, {"Sport", "Tennis"}}));
  cout << "Records Length: " << records.size() << endl;
}

RowTuple Scan::get_next() {
  if (iterator_position >= records.size()) {
    RowTuple empty_tuple;
    return empty_tuple;
  }
  RowTuple row_tuple = records[iterator_position];
  iterator_position++;
  return row_tuple;
}

int main() {
  cout << "Starting Main Function" << endl;
  Scan scan;
  scan.init();
  RowTuple row_tuple;
  while ( !(row_tuple = scan.get_next()).is_empty()) {
    row_tuple.print_contents();
  }
  return (0);
}
