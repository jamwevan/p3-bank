// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98

// These are the libraries that are used by the code.
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;

// This class encapsulates each transactions' data and provides methods to access and modify transaction details.
class Transaction
{
  public:
    Transaction(uint64_t time, const string &sender, const string &recepient, uint64_t amount, uint64_t exec, const string &execdate, const string &feePayer, size_t transID)
    :placementTime(time), sender(sender), recepient(recepient), amount(amount), exectime(exec),  execdate(execdate), feePayer(feePayer), transID(transID) {
      fee = 0;
    }
    uint64_t getPlacementTime() const{
      return placementTime;
    }
    string getSender() {
      return sender;
    }
    string getRecepient() {
      return recepient;
    }
    uint64_t getAmount() const{
      return amount;
    }
    uint64_t getExecTime() const{
      return exectime;
    }
    string getExecDate() const{
      return execdate;
    }
    string getFeePayer() const{
      return feePayer;
    }
    size_t getTransID() const{
      return transID;
    }
    uint64_t getFee() const{
      return fee;
    }
    void setFee(uint64_t bankFee){
      fee = bankFee;
    }
  private:
    uint64_t placementTime;
    string sender;
    string recepient;
    uint64_t amount;
    uint64_t exectime;
    string execdate;
    string feePayer;
    size_t transID;
    uint64_t fee;
};

// This class defines the criteria for ordering transactions in the PQ. This is the comparator the PQ using. This is a functor.
class TransactionCompare
{
  public:
  bool operator() (Transaction const &trans1, Transaction const &trans2) {
    // The statement "If L < R then return false" implies that this is a min heap.
    if (trans1.getExecTime() < trans2.getExecTime()) {
        return false;
    }
    // Second condition to break ties.
    else if (trans1.getExecTime() == trans2.getExecTime()) {
        // This is a min heap.
        if (trans1.getTransID() < trans2.getTransID()) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        // Stand alone else branch here
        return true;
    }
  }
};

// This class manages information for each user of the 281 bank.
class User {
public:
    /*
     The variable ts is time stamp of when the user registered.
     This constructor is used when loading user registration data from the account file.
     Remember the account file has lines in this format: REG_TIMESTAMP|USER_ID|PIN|STARTING_BALANCE.
     */
    User(uint64_t ts, string uID, string PIN, uint64_t balance)
    : timestamp(ts), userID(uID), myPin(PIN), balance(balance) {}
    // Default Constructor
    User(){
        timestamp = 0;
        userID = "none";
        myPin = "12345";
        balance = 0;
    }
    uint64_t getStartTime() const{
        return timestamp;
    }
    string getUserID() const{
        return userID;
    }
    string getPin() const{
        return myPin;
    }
    uint64_t getBalance() const{
        return balance;
    }
    // Returns the IP address of the active session.
    string getActiveSess() const{
        return activeUserSess;
    }
    /*
     Sets activeUserSess which causes the session to be marked as active.
     When a user logs in sucessfully, this method updates their active session.
     By having an IP in activeUserSess, it indicates that the user has an ongoing (active) session
     */
    void setActiveSess(const string &IP){
        activeUserSess = IP;
    }
    /*
     Adds new IP to IpAddresses, which is an unordered set.
     This data structure is crucial for verifying transactions quickly since only known IP addresses are valid
     */
    void addIP(const string &IPAddy){
        IPAddresses.insert(IPAddy);
    }
    // This function is used when the user logs out to ensure only logged-in users with valid IPs can transact.
    void removeIP(const string &IPAddy){
        IPAddresses.erase(IPAddy);
        activeUserSess = "";
    }
    // This function is used to confirm that a transaction request is coming from a recognized IP; helping to prevent fraudulent transactions.
    bool validIP(const string &IPAddy) const{
        if (IPAddresses.find(IPAddy) == IPAddresses.end()) {
            return false;
        }
        return true;
    }
    /*
     Returns true if the user has at least one logged IP address, indicating an active session.
     This ensures users can only place transactions while logged in.
     */
    bool isLoggedIn(){
        if(IPAddresses.size() >= 1) {
            return true;
        }
        else {
            return false;
        }
    }
    void removeMoney(uint64_t amount){
        balance = balance - amount;
    }
    
    void addMoney(uint64_t amount){
        balance = balance + amount;
    }
    // Adds a transaction to the outgoing vector, which stores a record of transactions the user has sent.
    void addOutgoing(Transaction trans){
        outgoing.push_back(trans);
    }
    // Adds a transaction to the incoming vector, which stores a record of transactions the user has received.
    void addIncoming(Transaction trans){
        incoming.push_back(trans);
    }
    vector<Transaction> getOutgoing(){
        return outgoing;
    }
    vector<Transaction> getIncoming(){
        return incoming;
    }
private:
    uint64_t timestamp;
    string userID;
    string myPin;
    uint64_t balance;
    string activeUserSess;
    unordered_set<string> IPAddresses;
    vector<Transaction> outgoing;
    vector<Transaction> incoming;
};

class Bank {
    public:
        // Bank constructor
        Bank(bool verbose)
          :isVerbose(verbose){
            numUsers = 0;
            numTransactions = 0;
        }
        size_t getNumUsers(){
          return numUsers;
        }
        void addUser(User newUser){
          /*
           The variable myUsers is an unordered map where the key is user id and the object is user.
           Both unordered_set and unordered_map are implemented as hash tables in C++.
           They both use hash functions to organize and quickly retrieve elements, but unordered map stores key value pairs.
          */
          myUsers[newUser.getUserID()] = newUser;
          numUsers++;
        }
        User* getUser(const string &uID){
          // Returning an address here.
          return &myUsers[uID];
        }
        bool login(const string &uID, const string &potPin, string IP){
          User* tempUser = getUser(uID);
          if(potPin == tempUser->getPin()){
            tempUser->setActiveSess(IP);
            tempUser->addIP(IP);
            return true;
          }
          else {
            // Returns false if failure.
            return false;
          }
        }
        bool logout(const string &uID, string IP){
          User* tempUser = getUser(uID);
          if(tempUser->validIP(IP)){
            tempUser->removeIP(IP);
            return true;
          }
          else {
              return false;
          }
        }
        void checkBalance(const string &userID, const string &IP) {
            // Checking if the user exists.
            auto it = myUsers.find(userID);
            if (it == myUsers.end()) {
                if (isVerbose) {
                    cout << "Account " << userID << " does not exist." << endl;
                }
                    return;
            }
            // Accessing second element in key-value pair.
            User* user = &it->second;
            // Check if the user is logged in
            if (!user->isLoggedIn()) {
                if (isVerbose) {
                    cout << "Account " << userID << " is not logged in." << endl;
                }
                return;
            }
            // Checking for fraudulent IP.
            if (!user->validIP(IP)) {
                if (isVerbose) {
                    cout << "Fraudulent transaction detected, aborting request." << endl;
                }
                    return;
            }
            // Determining the timestamp to use: mostRecentTimestamp or registration timestamp.
            uint64_t displayTimestamp = (mostRecentTimestamp != 0) ? mostRecentTimestamp : user->getStartTime();
            // Displaying balance if all checks passed.
            cout << "As of " << displayTimestamp << ", " << userID << " has a balance of $" << user->getBalance() << "." << endl;
        }
        bool placeTransaction(string &timestamp, string &IP, string &amount, string &exec_date, const string &feePayer, const string &sName, const string &rName){
          // Establishing a limit of 3 days to ensure that the exec_date is not too far in the future.
          uint64_t threedays = 3000000;
          // Converts exec_date and timestamp into c style strings that are stored in a char pointer because of array decay.
          const char* exec = exec_date.c_str();
          const char* time = timestamp.c_str();
          // Converting the c style strings into unit64_t integers.
          uint64_t execnum = strtoull(exec, NULL, 10);
          uint64_t timenum = strtoull(time, NULL, 10);
          // Setting mostRecentTimestamp to time of most recent place command.
          mostRecentTimestamp = timenum;
          uint64_t difference = execnum - timenum;
          if (sName == rName) {
              if (isVerbose) {
                  cout << "Self transactions are not allowed." << "\n";
              }
              else {
                  return false;
              }
          }
          if(difference > threedays) {
              if(isVerbose) {
                  cout << "Select a time up to three days in the future." << "\n";
              }
              return false;
          }
          // Ensuring that the sender exists.
          if(myUsers.find(sName) == myUsers.end()) {
              if(isVerbose) {
                  cout << "Sender " << sName << " does not exist." << "\n";
              }
              return false;
          }
          // Ensuring that the recipient exists.
          if(myUsers.find(rName) == myUsers.end()) {
              if(isVerbose) {
                  cout << "Recipient " << rName << " does not exist." << "\n";
              }
              return false;
          }
          // getUser returns a user object address
          User* sender = getUser(sName);
          User* recepient = getUser(rName);
          // The arrow operator is used to access member variables or member functions of a pointer.
          string senderName = sender->getUserID();
          string recepientName = recepient->getUserID();
          // Checking if the sender has registered.
          if(execnum < sender->getStartTime()) {
              if(isVerbose) {
                  cout << "At the time of execution, sender and/or recipient have not registered." << "\n";
              }
              return false;
          }
          // Checking if the recepient has registered.
          if(execnum < recepient->getStartTime()) {
              if(isVerbose) {
                  cout << "At the time of execution, sender and/or recipient have not registered." << "\n";
              }
              return false;
          }
          if(!sender->isLoggedIn()) {
              if(isVerbose) {
                  cout << "Sender " << senderName << " is not logged in." << "\n";
              }
              return false;
          }
          if(!sender->validIP(IP)) {
              if(isVerbose) {
                  cout << "Fraudulent transaction detected, aborting request." << "\n";
              }
              return false;
          }
          /*
           Calling executeTransaction to process any pending transactions before adding a new one; this place order arrived in a new point in time.
           Because we moved foward in time, we have to check if any pending transactions are now due to execute.
           Timestamp is read in from spec-commands because place orders come with a timestamp.
          */
          executeTransaction(timestamp);
          const char* amt = amount.c_str();
          uint64_t amtnum = strtoull(amt, NULL, 10);
          numTransactions++;
          Transaction trans = Transaction(timenum, senderName, recepientName, amtnum, execnum, exec_date, feePayer, numTransactions);
          // myTransactions a PQ.
          myTransactions.push(trans);
          if(isVerbose) {
              cout << "Transaction " << (trans.getTransID() - 1) << " placed at " << timenum << ": $" << amount << " from " << sender->getUserID() << " to " << recepient->getUserID() << " at " << execnum << "." << "\n";
          }
          return true;
        }
        bool hasTransactions(){
            if(myTransactions.size() > 0) {
                return true;
            }
            else {
                return false;
            }
        }
        // The function executeTransaction processes transactions in the priority queue up to the specified timestamp.
        void executeTransaction(string &timestamp){
          while(!myTransactions.empty()){
            const char* ctime = timestamp.c_str();
            uint64_t currentTime = strtoull(ctime, NULL, 10);
            // The top() method retrieves the transaction with the highest priority (earliest execution time) from the priority queue.
            Transaction temp = myTransactions.top();
            /*
             The bool valid variable is used to track whether a transaction is eligible to be executed based on a series of checks (such as sufficient funds for both the
             sender and recipient). If any of these checks fail, valid is set to false, preventing the transaction from being executed.
            */
            bool valid = true;
              //  If the transaction’s execution time is greater than currentTime, it’s not yet ready to be processed, so the function returns early and does nothing.
            if (temp.getExecTime() > currentTime) {
                return;
            }
            //o := sender, s := shared equally
            // The transaction fee is calculated as 1% of the transaction amount, with a minimum of $10 and a maximum of $450.
            uint64_t fee = (temp.getAmount() * 1) / 100;
            if (fee < 10) {
                fee = 10;
            }
            else if (fee > 450) {
                fee = 450;
            }
            // The variable temp is a Transaction object.
            uint64_t exectime = temp.getExecTime();
            User* sender = getUser(temp.getSender());
            User* recepient = getUser(temp.getRecepient());
            // If the sender has been registered for more than 5 years, they receive a 25% discount on the fee.
            // 50000000000) := 5 years
            if ((exectime - sender->getStartTime()) >= 50000000000) {
                fee = (fee * 3) / 4;
            }
            uint64_t senderFee = 0;
            uint64_t recepFee = 0;
            if (temp.getFeePayer() == "o") {
              senderFee = fee;
              recepFee = 0;
            }
            else if (temp.getFeePayer() == "s") {//shared fee
              recepFee = fee / 2;
              senderFee = fee / 2;
              if (fee%2 != 0) {//odd means sender pays the extra cent
                senderFee++;
              }
            }
            // The sender must have enough for the transaction amount plus their share of the fee.
            if (sender->getBalance() < (senderFee + temp.getAmount())) {
                if (isVerbose) {
                    cout << "Insufficient funds to process transaction " << (temp.getTransID() - 1) << "." << "\n";
                }
                // If either party lacks sufficient funds, the transaction is marked not valid and removed from the queue without executing.
              myTransactions.pop();
              valid = false;
            }
            // The recipient must have enough for their share of the fee.
            else if (recepient->getBalance() < recepFee) {
                if (isVerbose) {
                    cout << "Insufficient funds to process transaction " << (temp.getTransID() - 1) << "." << "\n";
                }
                // If either party lacks sufficient funds, the transaction is marked not valid and removed from the queue without executing.
              myTransactions.pop();
              valid = false;
            }
            if (valid) {
              sender->removeMoney(temp.getAmount() + senderFee);
              recepient->removeMoney(recepFee);
              recepient->addMoney(temp.getAmount());
              if (isVerbose) {
                  cout << "Transaction " << (temp.getTransID() - 1) << " executed at " << temp.getExecTime() << ": $" << temp.getAmount() << " from " << sender->getUserID() << " to " << recepient->getUserID() << "." << "\n";
              }

              myTransactions.pop();
              temp.setFee(fee);
              /*
               queryList is a vector of Transactions that keeps a history of all executed transactions. Adding temp to queryList ensures that this transaction can be accessed
               later for queries such as listing transactions within a specific time range or calculating bank revenue.
              */
              queryList.push_back(temp);
              /*
               The addOutgoing function records the transaction temp in the sender’s outgoing vector (a part of the User class). This allows the bank to retrieve a history
               of all transactions sent by the user, which is useful for generating transaction summaries or account histories.
              */
              sender->addOutgoing(temp);
              /*
               The addIncoming function records temp in the recipient’s incoming vector (also part of the User class). This allows the recipient’s account to show a record of
               all funds received, useful for query functions that generate account histories.
              */
              recepient->addIncoming(temp);
            }
          }
        }
        /*
         The ListTransactions function in the Bank class is designed to display a list of transactions that occurred within a specified time range.
         The function takes two string references, startTime and endTime, which represent the time range for the transactions to be listed.
         Each timestamp is in the format yy:mm:dd:hh:mm:ss
        */
        void ListTransactions(string &startTime, string &endTime){
          // Ignoring all colons.
          string temptime1 = startTime.substr(0,2) + startTime.substr(3,2) + startTime.substr(6,2) + startTime.substr(9,2) + startTime.substr(12,2) + startTime.substr(15,2);
          const char* ctime1 = temptime1.c_str();
          uint64_t start = strtoull(ctime1, NULL, 10);
          string temptime2 = endTime.substr(0,2) + endTime.substr(3,2) + endTime.substr(6,2) + endTime.substr(9,2) + endTime.substr(12,2) + endTime.substr(15,2);
          const char* ctime2 = temptime2.c_str();
          uint64_t end = strtoull(ctime2, NULL, 10);
          // The variable count keeps track of how many transactions fall within the specified range.
          int count = 0;
          // The loop iterates over each transaction in queryList, which is a vector of Transactions that stores all executed transactions.
          for(size_t i = 0; i < queryList.size(); ++i){
            Transaction* temp = &queryList[i];
            uint64_t time = temp->getExecTime();
            if (start <= time && time < end) {
              string d = "dollar";
              if (temp->getAmount() > 1 || temp->getAmount() == 0) {
                  // Pluralizing dollar when it is appropriate to do so.
                  d += 's';
              }
              // we use numTransactions as transID it is indexed by 1 and we are formatting output to index 0
            cout << (temp->getTransID() - 1) << ": " << temp->getSender() << " sent " << temp->getAmount() << " " << d << " to " << temp->getRecepient() << " at " << temp->getExecTime() << "." << '\n';
            count++;
            }
          }
          string t = "transaction";
          if (count > 1 || count == 0) {
            // Pluralizing transaction when it is appropriate to do so.
            t += 's';
            cout << "There were " << count <<  " " << t << " that were placed between time " << start << " to " << end << "." << '\n';
          }
          else {
            cout << "There was " << count <<  " " << t << " that was placed between time " << start << " to " << end << "." << '\n';
          }
        }
        /*
         The calcRevenue function in the Bank class calculates the bank’s revenue from transaction fees over a specified time range
         It iterates through the bank’s list of executed transactions (queryList) and sums up the fees for all transactions that occurred within the specified time window
        */
        uint64_t calcRevenue(uint64_t start, uint64_t end, bool isExec){
          uint64_t revenue = 0;
          for(size_t i = 0; i < queryList.size(); ++i){
            Transaction* temp = &queryList[i];
            uint64_t time = 0;
            /*
             IsExex is a boolean indicating whether to use the transaction’s execution time or placement time for comparison.
             This choice gives additional flexibility in the revenue calculation.
             It lets the function calculate revenue based on when transactions were placed or when they were executed.
            */
            if(isExec)
              time = temp->getExecTime();
            else
              time = temp->getPlacementTime();
            if(start <= time && time < end){
              revenue += temp->getFee();
            }
          }
          return revenue;
        }
        void BankRevenue(string &startTime, string &endTime){
          string temptime1 = startTime.substr(0,2) + startTime.substr(3,2) + startTime.substr(6,2) + startTime.substr(9,2) + startTime.substr(12,2) + startTime.substr(15,2);
          const char* ctime1 = temptime1.c_str();
          uint64_t start = strtoull(ctime1, NULL, 10);
          string temptime2 = endTime.substr(0,2) + endTime.substr(3,2) + endTime.substr(6,2) + endTime.substr(9,2) + endTime.substr(12,2) + endTime.substr(15,2);
          const char* ctime2 = temptime2.c_str();
          uint64_t end = strtoull(ctime2, NULL, 10);
          // Here isExec is set to true
          uint64_t revenue = calcRevenue(start, end, true);
          uint64_t time = end - start;
          string output = "";
          vector<string> times = {"second", "minute", "hour", "day", "month", "year"};
          size_t i = 0;
          while (time > 0) {
            // The loop extracts each component of time (e.g., seconds, minutes, etc.) by taking the last two digits and appending the corresponding unit
            uint64_t num = time % 100;
            if (num > 1) {
                output = to_string(num) + " " + times[i] + "s " + output;
            }
            else if (num == 1) {
                output = to_string(num) + " " + times[i] + " " + output;
            }
            // This line removes the last two digits from the time variable by use of integer division.
            time /= 100;
            ++i;
          }
          // Removing the trailing space.
        if (start != end) {
            output.pop_back();
        }
        cout << "281Bank has collected " << revenue << " dollars in fees over " << output << "." << '\n';
      }
        /*
         The CustomerHistory function in the Bank class displays a summary of a specific user’s account history, including their balance, total number of transactions, and
         recent incoming and outgoing transactions
        */
        void CustomerHistory(string &user){
          // If user does not exist then find returns an iterator equal to myUsers.end(), which is a special iterator representing “one past the end” of the container.
          if (myUsers.find(user) == myUsers.end()) {
            cout << "User " << user << " does not exist." << '\n';
            return;
          }
          User* thisUser = getUser(user);
          // The member function getIncoming() retrieves a vector of all transactions in which this user is the recipient.
          vector<Transaction> tempin = thisUser->getIncoming();
          // The member function getOutgoing() retrieves a vector of all transactions where this user is the sender.
          vector<Transaction> tempout = thisUser->getOutgoing();
          cout << "Customer " << user << " account summary:" << '\n';
          cout << "Balance: $" << thisUser->getBalance() << '\n';
          cout << "Total # of transactions: " << (tempin.size() + tempout.size()) << '\n';
          // The expression temp.size() is used multiple times so create a variable for it.
          size_t insize = tempin.size();
          cout << "Incoming " << insize << ":" << '\n';
          size_t start = 0;
          // Displaying the ten most recent incoming transactions.
          if (insize > 10) {
              start = insize - 10;
          }
          while (start < insize) {
          // Vectors support random access!
            Transaction* temp = &tempin[start];
            string d = "dollar";
            if (temp->getAmount() > 1 || temp->getAmount() == 0) {
                d += "s";
            }
            cout << (temp->getTransID() - 1) << ": " << temp->getSender() << " sent " << temp->getAmount() << " " << d << " to " << user << " at " << temp->getExecTime() << "." << '\n';
            start++;
          }
          size_t outsize = tempout.size();
          cout << "Outgoing " << outsize << ":" << '\n';
          start = 0;
          if (outsize > 10) {
              start = outsize - 10;
          }
          while(start < outsize){
            Transaction* temp = &tempout[start];
            string d = "dollar";
            if(temp->getAmount() > 1 || temp->getAmount() == 0)
              d += "s";
            cout << (temp->getTransID() - 1) << ": " << user << " sent " << temp->getAmount() << " " << d << " to " << temp->getRecepient() << " at " << temp->getExecTime() << "." << '\n';
            start++;
          }
        }
        // The SummarizeDay function in the Bank class provides a summary of all transactions that occurred within a specific day.
        void SummarizeDay(string timestamp){
          timestamp = timestamp.substr(0,2) + timestamp.substr(3,2) + timestamp.substr(6,2) + timestamp.substr(9,2) + timestamp.substr(12,2) + timestamp.substr(15,2);
          const char* ctime = timestamp.c_str();
          uint64_t time = strtoull(ctime, NULL, 10);
          /*
           The start of the day is calculated by setting the hour, minute, and second components to zero. This is done by subtracting the remainder when time is divided by
           1000000.
          */
          uint64_t start = time - (time % 1000000);
          // The end of the day is calculated by adding 1000000 to start, which adds 24 hours and represents the beginning of the following day.
          uint64_t end = time - (time % 1000000) + 1000000;
          cout << "Summary of [" << start << ", " << end << "):" << '\n';
          int count = 0;
          // The variable queryList contains all of the executed transactions.
          for (size_t i = 0; i < queryList.size(); ++i) {
            Transaction* temp = &queryList[i];
            uint64_t time = temp->getExecTime();
            if (start <= time && time < end) {
              string d = "dollar";
              if (temp->getAmount() > 1 || temp->getAmount() == 0) {
                  d += 's';
              }
              cout << (temp->getTransID() - 1) << ": " << temp->getSender() << " sent " << temp->getAmount() << " " << d << " to " << temp->getRecepient() << " at " << temp->getExecTime() << "." << '\n';
              count++;
            }
          }
          string t = "";
          if (count > 1 || count == 0) {
            t += "There were a total of " + to_string(count) + " transactions, ";
          }
          else {
            t += "There was a total of " + to_string(count) + " transaction, ";
          }
          uint64_t revenue = calcRevenue(start, end, true);
          t += "281Bank has collected " + to_string(revenue) + " dollars in fees.";
          cout << t << '\n';
        }
    private:
        // The data structure unordered_map stores a key-value pair where the key is the user id and the object is the user.
        unordered_map<string, User> myUsers;// key is user id, object is user
        size_t numUsers;
        bool isVerbose;
        size_t numTransactions;
        priority_queue<Transaction, vector<Transaction>, TransactionCompare> myTransactions;
        vector<Transaction> queryList;
        uint64_t mostRecentTimestamp;
};

void getMode(int argc, char * argv[], bool &isVerbose, string &filename) {
  //  This line tells getopt_long not to automatically print error messages for unrecognized options, allowing the program to handle error messages manually.
  opterr = false;
  // The variable choice is used to store the result of each parsed option from getopt_long.
  int choice;
  // This is a dummy variable because its only use is a special case where commands have the same name and you need an index to differentiate between them.
  int dummy = 0;
  // long_options is an array of option structures that are defined in the following lines
  option long_options[] = {
    { "help",    no_argument,       nullptr, 'h'  },
    { "file",    required_argument, nullptr, 'f'  },
    { "verbose", no_argument,       nullptr, 'v'  },
    // This is terminator for long_options.
    { nullptr,   0,                 nullptr, '\0' }
  };
  /*
   If getopt_long successfully identifies an option, it returns the option’s corresponding character h, f, or v.
   The character is implicitly converted to its integer representation when initializing choice.
   If there are no more options to process, getopt_long returns -1.
   Each time the loop iterates, getopt_long processes the next option.
   The getopt_long function is able to return a different character for each iteration because it maintains an internal state between calls with optind.
   Optind is a global variable declared in the getopt.h file.
   The function getopt_long checks argv[optind] when called.
  */
  while ((choice = getopt_long(argc, argv, "hf:v", long_options, &dummy)) != -1) {
      // Based on the value of choice, the function handles each option with the use of the switch statement.
    switch (choice) {
      case 'h':
        cout << "This program simulates EECS281 bank.\n";
        cout << "It takes in a registration file, follows commands, then outputs.\n";
        exit(0);
      case 'f':{
        /*
         The variable optarg is a global variable provided by getopt_long that holds the argument value for options that require one.
         It is overwritten when ecountering another option that requires an argument.
         Within the STL, string has a constructor that accepts a const char* parameter.
         This constructor creates a new std::string object from the contents of the C-style string.
         The syntax string arg{optarg}; uses brace initialization to call this constructor, creating arg as a std::string with the same content as optarg.
        */
        string arg{optarg};
        // Making sure that only txt files can be accepted.
        if (arg[arg.length() - 1] == 't') {
            filename = arg;
        }
        break;
      }
      case 'v':
        isVerbose = true;
        break;
      default:
        cerr << "Error: invalid option" << endl;
        exit(1);
      }
  }
}

int main(int argc, char* argv[]) {
    // IO optimization being utilized here.
    ios_base::sync_with_stdio(false);
    bool isVerbose = false;
    string fileName;
    getMode(argc, argv, isVerbose, fileName);
    // the filename was passed by reference
    if (fileName.empty()) {
        cerr << "filename has not been specified" << endl;
        exit(1);
    }
    Bank myBank = Bank(isVerbose);
    // Here we are using the ifstream constructor and specifying the file we are using and the fact we are reading
    ifstream regfile(fileName, ifstream::in);
    if(regfile.good()){
        while(regfile){
            string temptime;
            uint64_t time;
            string name;
            string pin;
            string tempnum;
            uint64_t balance;
            // getline(regfile, temptime, '|'); reads a portion of the line up to the first | character and stores it in temptime.
            getline(regfile, temptime, '|');//time
            if (temptime.empty()) {
                break;
            }
            temptime = temptime.substr(0,2) + temptime.substr(3,2) + temptime.substr(6,2) + temptime.substr(9,2) + temptime.substr(12,2) + temptime.substr(15,2);
            const char* ctime = temptime.c_str();
            time = strtoull(ctime, NULL, 10);
            getline(regfile, name, '|');
            getline(regfile, pin, '|');
            getline(regfile, tempnum);
            const char* tempstring = tempnum.c_str();
            balance = strtoull(tempstring, NULL, 10);
            User tempUser = User(time, name, pin, balance);
            myBank.addUser(tempUser);
        }
    }
    else {
        cerr << "Registration file failed to open." << endl;
        exit(1);
    }
    regfile.close();
    if (cin.fail()) {
        cerr << "Error: Reading from cin has failed" << endl;
    exit(1);
    }
    uint64_t prevPlaceTime = 0;
    int placed = 0;
    string currTime;
    /*
     We are using two different files. One is registration file and the other is a command file.
     When running the program from the command line, you can redirect cin to read from a file by using < operator.
     */
    if(!cin.fail()){
        string junk;
        string temp;
        cin >> temp;
        // In every given file the operations section ends with $$$, and is followed by queries.
        while(temp != "$$$"){
            switch(temp[0]){
                case '#':{
                    getline(cin, junk);
                    break;
                }
                case 'l':{
                    string uID;
                    string pin;
                    string IP;
                    cin >> uID;
                    cin >> pin;
                    cin >> IP;
                    bool success = myBank.login(uID, pin, IP);
                    if (success) {
                        if (isVerbose) {
                            cout << "User " << uID << " logged in." << "\n";
                        }
                    }
                    else {
                        if (isVerbose) {
                            cout << "Login failed for " << uID << "." << "\n";
                        }
                    }
                    break;
                }
                case 'o':{
                    string uID;
                    string IP;
                    cin >> uID;
                    cin >> IP;
                    bool success = myBank.logout(uID, IP);
                    if (success) {
                        if (isVerbose) {
                            cout << "User " << uID << " logged out." << "\n";
                        }
                    }
                    else {
                        if (isVerbose) {
                            cout << "Logout failed for " << uID << "." << "\n";
                        }
                    }
                    break;
                }
                // This is the case for the balance command.
                case 'b': {
                    string userID, IP;
                    cin >> userID >> IP;
                    myBank.checkBalance(userID, IP);
                    break;
                }
                // This is the case for the place command.
                case 'p':{
                    string timestamp;
                    string IP;
                    string sender;
                    string recepient;
                    string amount;
                    string exec_date;
                    string feePayer;
                    cin >> timestamp;
                    cin >> IP;
                    cin >> sender;
                    cin >> recepient;
                    cin >> amount;
                    cin >> exec_date;
                    cin >> feePayer;
                    timestamp = timestamp.substr(0,2) + timestamp.substr(3,2) + timestamp.substr(6,2) + timestamp.substr(9,2) + timestamp.substr(12,2) + timestamp.substr(15,2);
                    exec_date = exec_date.substr(0,2) + exec_date.substr(3,2) + exec_date.substr(6,2) + exec_date.substr(9,2) + exec_date.substr(12,2) + exec_date.substr(15,2);
                    const char* exec = exec_date.c_str();
                    const char* time = timestamp.c_str();
                    uint64_t execnum = strtoull(exec, NULL, 10);
                    uint64_t timenum = strtoull(time, NULL, 10);
                    if (prevPlaceTime > timenum && placed != 0) {
                        cerr << "Invalid decreasing timestamp in 'place' command." << endl;
                        exit(1);
                    }
                    if (execnum < timenum) {
                        cerr << "You cannot have an execution date before the current timestamp." << endl;
                        exit(1);
                    }
                    bool validT = myBank.placeTransaction(timestamp, IP, amount, exec_date, feePayer, sender, recepient);
                    if (validT) {
                        currTime = timestamp;
                        prevPlaceTime = timenum;
                        placed++;
                    }
                    break;
                }
            }
            cin >> temp;
        }
        // Setting a high time lets the function executeTransaction to process all remaining pending transactions.
        while (myBank.hasTransactions()) {
            string maxTime = "999999999999";
            myBank.executeTransaction(maxTime);
        }
        // Extracting $$$ from the command file.
        cin >> temp;
        // Now we are handling the queries.
        while(!cin.fail()){
            switch(temp[0]){
                case 'l':{
                    string starttime;
                    string endtime;
                    cin >> starttime;
                    cin >> endtime;
                    myBank.ListTransactions(starttime, endtime);
                    break;
                }
                case 'r':{
                    string starttime;
                    string endtime;
                    cin >> starttime;
                    cin >> endtime;
                    myBank.BankRevenue(starttime, endtime);
                    break;
                }
                case 'h':{
                    string user;
                    cin >> user;
                    myBank.CustomerHistory(user);
                    break;
                }
                case 's':{
                    string day;
                    cin >> day;
                    myBank.SummarizeDay(day);
                    break;
                }
            }
            cin >> temp;
        }
    }
        return 0;
}
