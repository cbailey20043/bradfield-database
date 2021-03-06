#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <stdlib.h>

using std::vector;
using std::cout;
using std::string;
using std::endl;
using std::unordered_map;
using std::unique_ptr;

extern "C" {
  #include "thirdparty/csv_parser/csv.h"
}

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

    void add_pair_to_record(string key, string value) {
      std::pair<std::string, std::string> new_pair(key, value);
      this->row_data.insert(new_pair);
    }

    void print_contents() {
      if (row_data.empty()) {
        cout << "Column: None" << endl;
        cout << "Value: None" << endl;
        return;
      }

      for (const auto& it : row_data) {
        cout << "<" << it.first << ", ";
        cout <<  it.second << "> ";
      }
      cout << endl;
    }

    const std::unordered_map<std::string, std::string>& get_row_data() {
      return row_data;
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
      for (std::unique_ptr<Iterator>& input : inputs) {
        input->init();
      }
    }

    virtual void close() {
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

class FileScan : public Iterator {
  public:

    FileScan() {}

    FileScan(string file_path) : file_path(file_path) {}

    void init() {
      cout << "File scan Init method" << endl;
      Iterator::init();

      record_ptrs.clear();
      iterator_position = 0;
      //read_dummy_data(); // comment out once you've implemented the read_csv function
      read_csv_data(); // uncomment when ready to read in CSV
    }

    void close() {
      cout << "File Scan Closed Called" << endl;
      Iterator::close();

      record_ptrs.clear();
      csv_headers.clear();
      iterator_position = 0;
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
    std::vector<std::string> csv_headers;
    unsigned int iterator_position = 0;
    const unsigned int max_csv_line_size = 100000;
    string file_path = ""; // should be absolute path

    void read_csv_data() {
      // Reading everything into memory for now
      FILE *fp = std::fopen(this->file_path.c_str(), "r");
      if (fp == nullptr) {
        throw std::runtime_error("Failed to open csv file at path: " + this->file_path);
      }
      process_csv_headers(fp);
      process_csv_data(fp);
      std::fclose(fp);
      //print_stored_records();
    }

    void print_stored_records() {
      // WARNING: I wouldn't call this if full csv file is read in
      cout << "Now Printing Stored Records from File Scan" << endl;
      for (auto &record : record_ptrs) {
        record->print_contents();
      }
    }

    void process_csv_data(FILE* fp) {
      int done = 0;
      int err = 0;
      char* csv_line;

      while (true) {
        csv_line = fread_csv_line(fp, max_csv_line_size, &done, &err);
        if (done) {
          return;
        }
        if (err || (csv_line == nullptr)) {
          throw std::runtime_error("Failed to process csv data: " + this->file_path);
        }
        char **parsed = parse_csv(csv_line);
        free(csv_line);

        auto new_tuple = unique_ptr<RowTuple>(new RowTuple());
        unsigned int counter = 0;
        char **curr_field = parsed;
        for(; *curr_field != nullptr; curr_field++) {
          string header = csv_headers[counter];
          string value = string(*curr_field);
          new_tuple->add_pair_to_record(header, value);
          counter++;
        }
        record_ptrs.push_back(std::move(new_tuple));
        free_csv_line(parsed);
      }
    }

    void process_csv_headers(FILE* fp) {
      int done = 0;
      int err = 0;
      char* csv_line = fread_csv_line(fp, max_csv_line_size, &done, &err);

      if (done) {
        throw std::runtime_error("CSV has no data at path: " + this->file_path);
      }
      if (err) {
        throw std::runtime_error("CSV reading error at path: " + this->file_path);
      }
      char **parsed = parse_csv(csv_line);
      free(csv_line);
      if (parsed == nullptr) {
        throw std::runtime_error("Error parsing csv file: " + this->file_path);
      }

      char **curr_field = parsed;
      for(; *curr_field != nullptr; curr_field++) {
        csv_headers.push_back(std::string(*curr_field));
      }
      free_csv_line(parsed);
    }

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

};

class Select : public Iterator {
  public:

    void init() {
      cout << "Select Node Inited" << endl;
      Iterator::init();
    }

    void close() {
      cout <<  "Select Node closed" << endl;
      Iterator::close();
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
      running_sum = 0.0;
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
        double converted_val = std::stod(val.c_str());
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
    double running_sum; // TODO guard against overflow
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
    Sort(string sort_col) : sort_column(sort_col) {}

    void init() {
      Iterator::init();
      iterator_position = 0;
      get_unsorted_input();
      sort_input();
    }

    void close() {
      Iterator::close();
      iterator_position = 0;
      sorted_list.clear();
    }

    void set_sort_column(string col_to_sort) {
      this->sort_column = col_to_sort;
    }

    std::unique_ptr<RowTuple> get_next_ptr() {
      if (iterator_position >= sorted_list.size()) {
        return nullptr;
      }
      auto next = std::move(sorted_list[iterator_position++]);
      return next;
    }

  private:
    std::vector<std::unique_ptr<RowTuple>> sorted_list;
    std::string sort_column = "";
    unsigned int iterator_position;

    void get_unsorted_input() {
      std::unique_ptr<Iterator>& input = inputs[0];
      std::unique_ptr<RowTuple> curr_tuple;

      while ((curr_tuple = input->get_next_ptr()) != nullptr) {
        sorted_list.push_back(std::move(curr_tuple));
      }
    }
    
    // NOTE, assuming that all RowTuples have same columns
    void sort_input() {
      // TODO overload comparison operator for RowTuples and put some of this logic there
      auto col_to_sort = this->sort_column;
      std::sort(sorted_list.begin(), sorted_list.end(), 
          [&col_to_sort](unique_ptr<RowTuple> &a, unique_ptr<RowTuple> &b) {
          if (col_to_sort != "") {
            //TODO What if value not found?
            //TODO Make sure you sort numeric data numerically!!!
            string a_val = a->get_value(col_to_sort);
            string b_val = b->get_value(col_to_sort);
            //double a_val = atof(a->get_value(col_to_sort).c_str());
            //double b_val = atof(b->get_value(col_to_sort).c_str());
            return  a_val < b_val;
          }
          
          std::vector<std::string> a_keys;
          for(const auto& it : a->get_row_data()) {
            a_keys.push_back(it.first);
          }
          std::sort(a_keys.begin(), a_keys.end()); // necessary for predictable sorting
          for (const auto& key : a_keys) {
            string a_val = a->get_value(key);
            string b_val = b->get_value(key);
            if ((b_val == "") || (a_val < b_val)) {
              return true;
            }
            else if (a_val > b_val) {
              return false;
            }
          }
          // defaulting to false if the rows are equal
          return false;
      });
    }
};

class Projection : public Iterator {
};

class NestedJoin : public Iterator {
  public:

    NestedJoin() {}

    void init() {
      check_for_required_inputs();
      Iterator::init();
    }

    void close() {
      Iterator::close();
    }

    void set_predicate(bool (*theta) (const std::unique_ptr<RowTuple>&, const std::unique_ptr<RowTuple>&)) {
      this->theta = theta;
    }

    unique_ptr<RowTuple> get_next_ptr() {
      unique_ptr<Iterator>& R = inputs[0];
      unique_ptr<Iterator>& S = inputs[1];
      if (current_r == nullptr) {
        this->current_r = R->get_next_ptr();
      }
      while (this->current_r != nullptr) {
        unique_ptr<RowTuple>& r = this->current_r;
        unique_ptr<RowTuple> s;
        while ((s = S->get_next_ptr()) != nullptr) {
          if (theta(r,s)) {
            return merge_row_tuples(r, s);
          }
        }
        S->close();
        S->init();
        this->current_r = R->get_next_ptr();
      }
      return nullptr;
    }

  private:
    unique_ptr<RowTuple> current_r;

    bool (*theta) (const std::unique_ptr<RowTuple>&, const std::unique_ptr<RowTuple>&);

    void check_for_required_inputs() {
      if (theta == nullptr) {
        throw std::runtime_error("Nested Join requires a predicate function");
      }
      if (inputs.size() != 2) {
        throw std::runtime_error("Nested join requires two inputs");
      }
    }

    unique_ptr<RowTuple> merge_row_tuples(const unique_ptr<RowTuple>& r, const unique_ptr<RowTuple>& s) {
      return nullptr;
    }
};


// Basic Count Test
void test_count_basic(const string& file_path) {
  Count count;
  auto future_input = unique_ptr<Iterator>(new FileScan(file_path));
  count.append_input(std::move(future_input));
  count.init();
  auto tot_count = count.get_next_ptr();
  cout << "Printing Count Contents" << endl;
  tot_count->print_contents();
  count.close();
}

// Basic average test
void test_average_basic(const string& file_path) {
  Average avg_node;
  auto file_scan = unique_ptr<Iterator>(new FileScan(file_path));
  avg_node.append_input(std::move(file_scan));
  avg_node.init();
  avg_node.set_col_to_avg("rating");
  avg_node.get_next_ptr()->print_contents();
  avg_node.close();
  
}

// Basic sort test
void test_sort_basic(const string& file_path) {
  auto file_scan = unique_ptr<Iterator>(new FileScan(file_path));
  Sort sort;
  sort.set_sort_column("rating");
  sort.append_input(std::move(file_scan));
  sort.init();
  unique_ptr<RowTuple> t;

  while ((t = sort.get_next_ptr()) != nullptr) {
    t->print_contents();
  }
}

void test_row_tuple_equality() {
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
void test_distinct_node_basic(const string& file_path) {
  auto scan = std::unique_ptr<Iterator>(new FileScan(file_path));
  auto sort = std::unique_ptr<Iterator>(new Sort("movieId")); //timestamp
  Distinct d;
  sort->append_input(std::move(scan));
  d.append_input(std::move(sort));
  d.init();

  unique_ptr<RowTuple> tuple;
  unsigned int counter = 0;

  while((tuple = d.get_next_ptr()) != nullptr) {
    tuple->print_contents();
    counter++;
  }
  cout << "Number of records seen testing distinct = " << counter << endl;
}

void test_csv_read() {
  FileScan scan("/Users/cameron/database_class/resources/datasets/ratings_10.csv");
  scan.init();
  scan.close();

  FileScan scan2("/Users/cameron/database_class/resources/datasets/ratings_10.csv");
  scan2.init();
  scan2.close();
}

int main() {
  cout << "Starting Main Function" << endl;
  //test_count_basic();
  //test_average_basic();
  //test_sort_basic();
  const string test_file_path = "/Users/cameron/database_class/resources/datasets/ratings_10.csv";
  //test_count_basic(test_file_path);
  test_distinct_node_basic(test_file_path);
  //test_one();
  //test_two();
  //test_csv_read();
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




