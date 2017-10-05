#include <iostream>
#include <vector>

using std::vector;
using std::cout;
using std::string;
using std::endl;

class RowTuple {
  public:
    RowTuple() {};
    RowTuple(vector<string> col_names, vector<string> data) : column_headers(std::move(col_names)), row_data(std::move(data)) {}
    bool is_empty();
    void print_contents();
  private:
    // optimize this
    vector<string> column_headers;
    vector<string> row_data;
};

class Iterator {
  public:
    virtual void init() = 0;
    virtual void close() = 0;
    virtual RowTuple get_next() = 0;
};

class Scan : public Iterator {
  public:
    void init();
    void close();
    RowTuple get_next();
    void read_data();
  private:
    vector<RowTuple> records;
    int iterator_position = 0;
};

class SelectionNode : public Iterator {
  public:
    void init();
    void close();
    RowTuple get_next();
};

void RowTuple::print_contents() {
  if (column_headers.empty()) {
    cout << "Column Names: None" << endl;
    cout << "Data Fields: None" << endl;
    return;
  }
  vector<string>::iterator col_headers_it = column_headers.begin();
  vector<string>::iterator data_it = row_data.begin();

  while (col_headers_it != column_headers.end() && data_it != row_data.end()) {
    cout << "Column: " << *col_headers_it << ", " << "Value: " << *data_it << endl;
    col_headers_it++;
    data_it++;
  }

}

bool RowTuple::is_empty() {
  return row_data.empty();
}

void Scan::init() {
  cout << "Scan Node Inited" << "\n";
  records.clear();
}

void Scan::close() {
  cout << "Scan Node closed" << "\n";
}

void Scan::read_data() {
  if (!records.empty()) {records.clear();}

  records.push_back(RowTuple({"student", "gpa"}, {"Samer", "4.0"}));
  records.push_back(RowTuple({"student", "gpa"}, {"Vibe", "-2.0"}));
  records.push_back(RowTuple({"student", "gpa"}, {"Cameron", "4.0009"}));

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
  cout << "Hello";
  Scan scan;
  scan.init();
  scan.read_data();
  RowTuple row_tuple;
  
  while (!(row_tuple = scan.get_next()).is_empty()) {
    row_tuple.print_contents();
  }

  return (0);
}
