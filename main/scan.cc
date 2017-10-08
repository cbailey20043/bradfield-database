#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdlib.h>

using std::vector;
using std::cout;
using std::string;
using std::endl;
using std::unordered_map;
using std::unique_ptr;

class RowTuple {
  public:
    RowTuple() { cout << "Default RowTuple Constructor Called" << endl;}
    RowTuple(unordered_map<string, string> input_map) : row_data(std::move(input_map)) {}

    bool is_empty() {
      return row_data.empty();
    }

    string get_value(const string& key) {
      auto it = row_data.find(key);
      if (it == row_data.end()) {
        return "";
      }
      return it->second;

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

    virtual ~Iterator() = default;
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
        return RowTuple();
      }
      // Calls copy constructor
      // maybe read from iterator instead -- future enhancement
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
      records.push_back(RowTuple({{"student", "Foo Bar"}, {"Sport", "blahness"}, {"id", "3"}}));
      cout << "Records read in" << endl;
    }
};

class Select : public Iterator {
  public:

    void init() {}

    void close() {}

    void set_predicate(bool (*predicate) (RowTuple)) {
      this->predicate = predicate;
    }

    RowTuple get_next() {
      // right now this will only be a single input
      // Also, add checks to make sure the predicate has been initialized
      //
      if (inputs.empty() || predicate == nullptr) {
        return RowTuple();
      }

      RowTuple tuple;
      while (!(tuple = inputs[0]->get_next()).is_empty()) {
        if (predicate(tuple)) {
          return tuple;
        }
      }
      return RowTuple();
    }

  private:
    bool (*predicate) (RowTuple);
};

class Count : public Iterator {
  public:

    Count() {}

    Count(string alias) : result_alias(alias) {}

    void init() {}

    void close() {}

    void set_result_alias(const string& alias) {
      this->result_alias = alias;
    }

    RowTuple get_next() {
      if (inputs.empty()) {
        return RowTuple();
      }

      unique_ptr<Iterator>& input = inputs[0];
      while (!(input->get_next()).is_empty()) {
        num_records++;
      }

      //return RowTuple({result_alias, std::to_string(num_records)});
      return RowTuple(std::unordered_map<string, string>{{result_alias, std::to_string(num_records)}});

    }

  private:
    long num_records = 0;
    string result_alias = "Count";
  
};

class Average : public Iterator {
  // Don't forget... need to take in column name
  // Also, avg will only work work on integers at the moment
  public:
    Average() {}
    Average(const string& alias) : result_alias(alias) {}

    void init() {}
    void close() {}

    void set_result_alias(const string& alias) {
      result_alias = alias;
    }

    void set_col_to_avg(const string& col_name) {
      this->column_to_avg = col_name;
    }

    RowTuple get_next() {
      if (inputs.empty() || column_to_avg == "") {
        cout << "Averge: Either inputs or target col name not set" << endl;
        return RowTuple();
      }

      unique_ptr<Iterator>& input = inputs[0];
      RowTuple curr_tuple;

      while (!(curr_tuple = input->get_next()).is_empty()) {
        string val = curr_tuple.get_value(this->column_to_avg);
        if (val == "") {
          cout << "Average: No value in row_tuple matching key: " << this->column_to_avg << endl;
          return RowTuple();
        }
        long converted_val = atol(val.c_str());
        if (!converted_val) {
          cout << "Avg: Conversion of string to long failed." << endl;
          return RowTuple();
        }
        running_sum += converted_val;
        total_count++;
      }

      double avg = running_sum/((double) total_count);
      return RowTuple(std::unordered_map<string, string>{{result_alias, std::to_string(avg)}});

    }

  private:
    string result_alias = "Average";
    string column_to_avg = "";
    long total_count = 0;
    long running_sum = 0; // TODO guard against overflow

    long convert_str_to_long(const string& str_val) {
      long converted = atol(str_val.c_str());
      return converted;
    }
};

void query_one() {
}

int main() {
  cout << "Starting Main Function" << endl;
  /*
  FileScan scan;
  scan.init();
  RowTuple row_tuple;
  while ( !(row_tuple = scan.get_next()).is_empty()) {
    row_tuple.print_contents();
  }
  */

  cout << "Going Further" << endl;

  
  Select* select = new Select;
  select->get_next();
  delete select;
  

  unique_ptr<Iterator> file_scan(new FileScan());
  file_scan->init();
  Count count("total_count");
  
  count.append_input(std::move(file_scan));

  cout << "About to Print Context of count" << endl;
  
  count.get_next().print_contents();
  



  return (0);
}







