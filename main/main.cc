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
    RowTuple() {}
    RowTuple(unordered_map<string, string> input_map) : row_data(std::move(input_map)) {}

    bool is_empty() {
      return row_data.empty();
    }

    std::string get_value(const string& key) {
      auto it = row_data.find(key);
      if (it == row_data.end()) {
        cout << "Could not find key " << key << endl;
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

    // Note this is case sensitive right now
    // TODO Maybe make case-insensitive
    bool operator==(const RowTuple& other) {
      if (row_data.size() != other.row_data.size()) {
        return false;
      }
      
      for (const auto& it : row_data) {
        string key = it.first;
        string value = it.second;
        auto other_it = other.row_data.find(key);
        if ((other_it == other.row_data.end()) || (other_it->second != value)) {
          return false;
        }
      }
      return true;
    }

    bool operator!=(const RowTuple& other) {
      return !(*this == other); 
    }

  private:
    std::unordered_map<string, string> row_data;
};

/**
 * Note, all the init method of an iterator must be called before it is used
 * Behavior is undefined if you call an iterator class without calling init first
 */
class Iterator {
  public:

    virtual ~Iterator() = default;

    virtual void init() {
      cout << "Init in Iterator Base Class" << endl;
      for (std::unique_ptr<Iterator>& input : inputs) {
        input->init();
      }
    }

    virtual void close() {
      cout << "Inside Iterator Close Base Class" << endl;
      // close all inputs
      // TODO make sure there won't be a scenario where inputs are closed multiple times
      for (std::unique_ptr<Iterator>& input : inputs) {
        input->close();
      }
    }

    virtual std::unique_ptr<RowTuple> get_next_ptr() = 0;

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
      cout << "File scan Init method" << endl;
      // Init inputs first
      Iterator::init();

      // Handle myself
      record_ptrs.clear();
      iterator_position = 0;
      read_dummy_data(); // comment out once you've implemented the read_csv function
      //read_csv_data(); // uncomment when ready to read in CSV
    }

    void close() {
      cout << "File Scan Closed Called" << endl;
      // Close inputs
      Iterator::close();

      // Close class-specific resources
      record_ptrs.clear();
      iterator_position = 0;
      // TODO free up any resources related to reading in csv file
    }

    std::unique_ptr<RowTuple> get_next_ptr() {
      if (iterator_position >= record_ptrs.size()) {
        return nullptr;
      }

      auto row_tuple = std::move(record_ptrs[iterator_position]); 
      iterator_position++;
      return row_tuple;
    }

  private:
    std::vector<std::unique_ptr<RowTuple>> record_ptrs;
    unsigned int iterator_position = 0;
    string file_name = "";

    void read_dummy_data() {
      if (!record_ptrs.empty()) { record_ptrs.clear(); }

      auto record1 = std::unique_ptr<RowTuple>(
          new RowTuple({{"student", "Jimmy Cricket"}, {"Sport", "Tennis"}, {"id", "2"}}));
      auto record2 = std::unique_ptr<RowTuple>(
          new RowTuple({{"student", "Foo Bar"}, {"Sport", "Rocket League"}, {"id", "3"}}));
      auto record3 = std::unique_ptr<RowTuple>(
          new RowTuple({{"student", "Arry Potter"}, {"Sport", "Quidditch"}, {"id", "2"}}));
      auto record4 = std::unique_ptr<RowTuple>(
          new RowTuple({{"student", "Arry Potter"}, {"Sport", "Quidditch"}, {"id", "2"}}));
      auto record5 = std::unique_ptr<RowTuple>(
          new RowTuple({{"student", "Harry Potter"}, {"Sport", "Foo"}, {"id", "5"}}));

      record_ptrs.push_back(std::move(record1));
      record_ptrs.push_back(std::move(record2));
      record_ptrs.push_back(std::move(record3));
      record_ptrs.push_back(std::move(record4));
      record_ptrs.push_back(std::move(record5));
    }

    void read_csv_data() {
      //TODO Read in csv data
      // For fist attempt, read everything into memory (since you can)o
      // Later, read in a section of csv file and maintain
      // necessary info so you can resume reading at subsequent call
    }
};

class Select : public Iterator {
  public:

    void init() {
      cout << "Select Node Inited" << endl;
      // Init inputs
      Iterator::init();
      // No internal state to set up at moment
    }

    void close() {
      cout <<  "Select Node closed" << endl;
      Iterator::close();
      // No internal state to clean up
    }

    void set_predicate(bool (*predicate) (const std::unique_ptr<RowTuple>&)) {
      this->predicate = predicate;
    }
    
    std::unique_ptr<RowTuple> get_next_ptr() {
      if (inputs.empty() || predicate == nullptr) {
        return nullptr;
      }
      std::unique_ptr<Iterator>& input = inputs[0];
      std::unique_ptr<RowTuple> curr_tuple;
      while((curr_tuple = input->get_next_ptr()) != nullptr) {
        if (predicate(curr_tuple)) {
          return curr_tuple;
        }
      }
      return nullptr;

    }
    
  private:
    bool (*predicate) (const std::unique_ptr<RowTuple>&);
};

class Count : public Iterator {
  public:

    Count() {}

    Count(string alias) : result_alias(alias) {}

    void init() {
      cout << "Initializing Count Node" << endl;
      Iterator::init();
      num_records = 0;
    }

    void close() {
      cout << "Closing Count Node" << endl;
      Iterator::close();
    }

    void set_result_alias(const string& alias) {
      this->result_alias = alias;
    }

    std::unique_ptr<RowTuple> get_next_ptr() {
      if (inputs.empty()) {
        return std::unique_ptr<RowTuple>(new RowTuple());
      }
      
      unique_ptr<Iterator>& input = inputs[0];
      while((input->get_next_ptr()) != nullptr) {
        num_records++;
      }

      auto total_count = std::unique_ptr<RowTuple>(
          new RowTuple(std::unordered_map<string, string>{{result_alias, std::to_string(num_records)}}));

      return total_count;
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

    void init() {
      cout <<  "Initing Average Iterator" << endl;
      Iterator::init();
      total_count = 0;
      running_sum = 0;
    }
    void close() {
      cout << "Closing Average Iterator" << endl;
      Iterator::close();
    }

    void set_result_alias(const string& alias) {
      result_alias = alias;
    }

    void set_col_to_avg(const string& col_name) {
      this->column_to_avg = col_name;
    }

    /* Pointer representation */
    unique_ptr<RowTuple> get_next_ptr() {
      if (inputs.empty() || column_to_avg == "") {
        cout << "Average: Either inputs or target col name not set" << endl;
        return nullptr;
      }

      unique_ptr<Iterator>& input = inputs[0];
      unique_ptr<RowTuple> curr_tuple;

      while((curr_tuple = input->get_next_ptr()) != nullptr) {
        string val = curr_tuple->get_value(this->column_to_avg);
        if (val == "") {
          // Assuming that all or none of the tuples contain the column to sort on 
          // Revisit this logic if necessary
          cout << "Average: No value in row_tuple matching key: " << this->column_to_avg << endl;
          return nullptr;
        }
        long converted_val = atol(val.c_str());
        if (!converted_val) {
          cout << "Avg: Conversion to long failed" << endl;
          return nullptr;
        }
        running_sum += converted_val;
        total_count++;
      }
      double avg = total_count == 0 ? 0.0 : ((running_sum)/((double) total_count));

      return std::unique_ptr<RowTuple>(
          new RowTuple(std::unordered_map<string, string>{{result_alias, std::to_string(avg)}}));
    }

  private:
    string result_alias = "Average";
    string column_to_avg = "";
    long total_count;
    long running_sum; // TODO guard against overflow
};

class Distinct : public Iterator {
  // Note, input needs to be in sorted order for this operator to work
  public:
    Distinct() {}

    void init() {
      cout << "Initing Distinct Node" << endl;
      Iterator::init();
    }

    void close() {
      cout << "Closing Distinct Node" << endl;
      Iterator::close();
    }

    std::unique_ptr<RowTuple> get_next_ptr() {
      if (inputs.empty()) {
        return nullptr;
      }
      std::unique_ptr<Iterator>& input = inputs[0];
      std::unique_ptr<RowTuple> curr_tuple;

      while((curr_tuple = input->get_next_ptr()) != nullptr) {
        if (curr_reference == nullptr) {
          curr_reference = std::move(curr_tuple);
        }
        else {
          if (*curr_reference != *curr_tuple) {
            auto return_val = std::move(curr_reference);
            curr_reference = std::move(curr_tuple);
            return return_val;
          }
        }

      }
      
      if (curr_reference != nullptr) {
        return std::move(curr_reference);
      }
      return nullptr;
    }

  private:
    std::unique_ptr<RowTuple> curr_reference = nullptr;
};

// Note right now sort criteria is being passed in
class Sort : public Iterator {
  public:
    Sort() {}

    void init() {
      // Note the init method should read in and sort all records
      // get_next will then return one sorted record at a time
      // so init() will be slow, get_next() = fast
      Iterator::init();
    }

    void close() {
      Iterator::close();
    }

    std::unique_ptr<RowTuple> get_next_ptr() {
      return nullptr;
      // Call next from input until nothing left, store everything in vector, sort vector
      // Keep returning elements from vector until noting left
    }
};

class Projection : public Iterator {
};

// Basic Count Test
void test_one() {
  // Based off of above query
  /*
  FileScan s;
  std::unique_ptr<RowTuple> foo = s.test_1(); 
  std::unique_ptr<RowTuple> bar = std::move(foo);
  */
  /*
  FileScan s;
  s.init();
  auto next = s.get_next_ptr();
  cout << "About to print contents from query 1" << endl;
  next->print_contents();
  */

  Count count;
  auto future_input = unique_ptr<Iterator>(new FileScan());
  count.append_input(std::move(future_input));
  count.init();
  auto tot_count = count.get_next_ptr();
  cout << "Printing Count Contents" << endl;
  tot_count->print_contents();
  count.close();
}

// Basic average test
void test_average_basic() {
  Average avg_node;
  auto file_scan = unique_ptr<Iterator>(new FileScan());
  avg_node.append_input(std::move(file_scan));
  avg_node.init();
  avg_node.set_col_to_avg("id");
  avg_node.get_next_ptr()->print_contents();
  
}

void test_two() {
  cout << "Starting Test 2" << endl;
  auto s = std::unique_ptr<Iterator>(new FileScan());

  Average avg("avg");
  avg.set_col_to_avg("foo");
  avg.append_input(std::move(s));
  cout << "About to print average" << endl;
  avg.init();
  cout << (avg.get_next_ptr() == nullptr) << endl;

  auto p = std::unique_ptr<Iterator>(new FileScan());
  Average avg2("id");
  avg2.set_col_to_avg("id");
  avg2.append_input(std::move(p));
  avg2.init();
  cout << "About to print second average" << endl;
  avg2.get_next_ptr()->print_contents();
  
}

// Tests row_tuple equality
void test_three() {
  cout << "Starting RowTuple equality tests" << endl;
  RowTuple t1({{"student", "jimmy cricket"}, {"id", "2"}});
  RowTuple t2({{"student", "jimmy cricket"}, {"id", "2"}});
  cout << "RowTuple equality test 1\t" <<  "Expected: 1 " << "Actual: " << (t1 == t2) << endl;

  cout << "Second RowTuple Test: Same keys but diff value" << endl;
  RowTuple t3({{"student", "jimmy cricket"}, {"id", "2"}});
  RowTuple t4({{"student", "jimmy cricket"}, {"id", "1"}});
  cout << "RowTuple equality test 1\t" <<  "Expected: 0 " << "Actual: " << (t3 == t4) << endl;

  cout << "RowTuple Equality Test: Diff keys, same value" << endl;
  RowTuple t5({{"student", "jimmy cricket"}, {"ids", "2"}});
  RowTuple t6({{"student", "jimmy cricket"}, {"id", "2"}});
  cout << "RowTuple equality test 1\t" <<  "Expected: 0 " << "Actual: " << (t5 == t6) << endl;

  cout << "RowTuple Equality Test: Diff size bucket" << endl;
  RowTuple t7({{"student", "jimmy cricket"}, {"id", "2"}, {"fav_novel", "IT"}});
  RowTuple t8({{"student", "jimmy cricket"}, {"id", "2"}});
  cout << "RowTuple equality test 1\t" <<  "Expected: 0 " << "Actual: " << (t7 == t8) << endl;

  cout << "RowTuple Equality: Check not equals operator" << endl;
  RowTuple t9({{"student", "james cricket"}, {"id", "2"}});
  RowTuple t10({{"student", "jimmy cricket"}, {"id", "2"}});
  cout << "RowTuple equality test 1\t" <<  "Expected: 1 " << "Actual: " << (t9 != t10) << endl;
  
}

// Modify later to add Sort Node as input
void test_distinct_node_basic() {
  auto scan = std::unique_ptr<Iterator>(new FileScan());
  Distinct d;
  d.append_input(std::move(scan));
  d.init();

  unique_ptr<RowTuple> tuple;
  unsigned int counter = 0;

  while((tuple = d.get_next_ptr()) != nullptr) {
    tuple->print_contents();
    counter++;
  }
  cout << "Number of records seen testing distinct = " << counter << endl;
}

int main() {
  cout << "Starting Main Function" << endl;
  //test_one();
  //test_average_basic();
  test_two();
  //test_distinct_node_basic();
  //FileScan scan;
  //scan.init();
  //query_one();
  //test_two();
  //test_three();
  //test_distinct_node();
  /*
  FileScan scan;
  scan.init();
  RowTuple row_tuple;
  while ( !(row_tuple = scan.get_next()).is_empty()) {
    row_tuple.print_contents();
  }
  */

  /*
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
  */
  



  return (0);
}




