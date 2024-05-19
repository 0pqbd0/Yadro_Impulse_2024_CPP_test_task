#include <iostream>
#include <fstream>
#include <regex>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <iomanip>

class Client {
public:
    std::string name;
    int tableNumber;
    bool isWaiting;

    Client(const std::string &name)
        : name(name), tableNumber(0), isWaiting(false) {
    }
};

class Table {
public:
    int number;
    std::string takenSince;
    int cash;
    bool isTaken;
    int howLongTaken_m;

    Table(int number) : number(number), cash(0), isTaken(false), howLongTaken_m(0) {
    }
};

class Event {
public:
    std::string time;
    int id;
    std::string name;
    int details;

    Event(const std::string &time, int id, const std::string &name, int details = 0)
        : time(time), id(id), name(name), details(details) {
    }
};

class Club {
private:
    const int tableCount;
    std::string openingTime;
    std::string closingTime;
    const int hourlyRate;
    std::map<std::string, Client> clients;
    std::vector<Table> tables;
    std::queue<std::string> waitingQueue;
    std::vector<std::string> report;

    bool isOpen(const std::string &time);

    void processEvent(const Event &event);

    void processClientArrival(const Event &event);

    void processClientSit(const Event &event);

    void processClientWait(const Event &event);

    void processClientLeave(const Event &event);

    void processClubIsClosing();

    void generateReport();

public:
    Club(int tableCount, const std::string &openingTime, const std::string &closingTime, int hourlyRate);

    int freeTables;

    void closingClub();

    void addEvent(std::string &eventLine);

    void printReport();
};

Club::Club(int tableCount, const std::string &openingTime, const std::string &closingTime, int hourlyRate)
    : tableCount(tableCount), openingTime(openingTime), closingTime(closingTime), hourlyRate(hourlyRate),
      freeTables(tableCount) {
    for (int i = 1; i <= tableCount; ++i) {
        tables.emplace_back(i);
    }
}

void Club::addEvent(std::string &eventLine) {
    report.push_back(eventLine);
    std::istringstream iss(eventLine);
    std::vector<std::string> fields;

    std::string field;
    while (iss >> field) {
        fields.push_back(field);
    }
    std::string time = fields[0];
    int id = std::stoi(fields[1]);
    std::string name = fields[2];
    if (id == 2) {
        int details = std::stoi(fields[3]);
        Event event(time, id, name, details);
        processEvent(event);
    } else {
        Event event(time, id, name);
        processEvent(event);
    }
}

bool Club::isOpen(const std::string &time) {

    int eventHour = std::stoi(time.substr(0, 2));
    int eventMinutes = std::stoi(time.substr(3, 2));
    int eventTime = eventHour * 60 + eventMinutes;

    int closingHour = std::stoi(closingTime.substr(0, 2));
    int closingMinutes = std::stoi(closingTime.substr(3, 2));
    int closingTime = closingHour * 60 + closingMinutes;

    int openingHour = std::stoi(openingTime.substr(0, 2));
    int openingMinutes = std::stoi(openingTime.substr(3, 2));
    int openingTime = openingHour * 60 + openingMinutes;


    return eventTime >= openingTime && eventTime <= closingTime;
}

void Club::processClubIsClosing() {
    if (!clients.empty()) {
        std::vector<std::string> clientsToErase;
        for (const auto &elem: clients) {
            if (elem.second.tableNumber != 0) {
                auto &table = tables[elem.second.tableNumber - 1];
                table.isTaken = false;

                int closingtime = std::stoi(closingTime.substr(0, 2)) * 60 + std::stoi(closingTime.substr(3, 2));

                int clienttime = std::stoi(tables[elem.second.tableNumber - 1].takenSince.substr(0, 2)) * 60 +
                                 std::stoi(tables[elem.second.tableNumber - 1].takenSince.substr(3, 2));


                int timeSpent = closingtime - clienttime;
                table.howLongTaken_m += timeSpent;
                table.cash += hourlyRate * ((timeSpent + 59) / 60);
            }
            report.push_back(closingTime + " 11 " + elem.first);
            clientsToErase.push_back(elem.first);
        }

        for (const auto &key: clientsToErase) {
            clients.erase(key);
        }

        freeTables = tableCount;
    }
}

void Club::closingClub() {
    processClubIsClosing();
}


void Club::processEvent(const Event &event) {
    switch (event.id) {
        case 1: processClientArrival(event);
            break;
        case 2: processClientSit(event);
            break;
        case 3: processClientWait(event);
            break;
        case 4: processClientLeave(event);
            break;
    }
}

void Club::processClientArrival(const Event &event) {
    if (clients.find(event.name) != clients.end()) {
        report.push_back(event.time + " 13 YouShallNotPass");
        return;
    }
    if (!isOpen(event.time)) {
        report.push_back(event.time + " 13 NotOpenYet");
        return;
    }

    clients.emplace(event.name, Client(event.name));
}

void Club::processClientSit(const Event &event) {
    if (!isOpen(event.time)) {
        processClubIsClosing();
        report.push_back(event.time + " 13 NotOpenYet");
        return;
    }

    if (clients.find(event.name) == clients.end()) {
        report.push_back(event.time + " 13 ClientUnknown");
        return;
    }

    auto &client = clients.at(event.name);
    if (client.tableNumber != 0) {
        if (client.tableNumber == event.details) {
            report.push_back(event.time + " 13 PlaceIsBusy");
            return;
        }
    }

    if (tables[event.details - 1].isTaken) {
        report.push_back(event.time + " 13 PlaceIsBusy");
        return;
    }

    if (client.tableNumber != 0) {
        tables[client.tableNumber - 1].isTaken = false;
        client.tableNumber = event.details;
        tables[event.details - 1].isTaken = true;
        tables[event.details - 1].takenSince = event.time;
    }

    if (client.tableNumber == 0) {
        client.tableNumber = event.details;
        tables[event.details - 1].isTaken = true;
        tables[event.details - 1].takenSince = event.time;
        --freeTables;
    }
}

void Club::processClientWait(const Event &event) {
    if (!isOpen(event.time)) {
        processClubIsClosing();
        report.push_back(event.time + " 13 NotOpenYet");
        return;
    }

    if (clients.find(event.name) == clients.end()) {
        report.push_back(event.time + " 13 ClientUnknown");
        return;
    }

    if (clients.find(event.name) != clients.end() && freeTables > 0) {
        report.push_back(event.time + " 13 ICanWaitNoLonger!");
        return;
    }

    if (!waitingQueue.empty() && waitingQueue.size() >= tableCount) {
        report.push_back(event.time + " 11 " + event.name);
        clients.erase(event.name);
        return;
    }

    waitingQueue.push(event.name);
    clients.at(event.name).isWaiting = true;
}

void Club::processClientLeave(const Event &event) {
    if (!isOpen(event.time)) {
        processClubIsClosing();
        report.push_back(event.time + " 13 NotOpenYet");
        return;
    }

    if (clients.find(event.name) == clients.end()) {
        report.push_back(event.time + " 13 ClientUnknown");
        return;
    }

    auto &client = clients.at(event.name);
    if (client.tableNumber != 0) {
        auto &table = tables[client.tableNumber - 1];
        table.isTaken = false;

        int eventTime = std::stoi(event.time.substr(0, 2)) * 60 + std::stoi(event.time.substr(3, 2));

        int clientTime = std::stoi(tables[client.tableNumber - 1].takenSince.substr(0, 2)) * 60 +
                         std::stoi(tables[client.tableNumber - 1].takenSince.substr(3, 2));


        int timeSpent = eventTime - clientTime;
        table.howLongTaken_m += timeSpent;
        table.cash += hourlyRate * ((timeSpent + 59) / 60);
        client.tableNumber = 0;


        if (!waitingQueue.empty()) {
            auto &clientNew = clients.at(waitingQueue.front());
            waitingQueue.pop();
            clientNew.tableNumber = table.number;
            /*clientNew.startTime = event.time;*/
            clientNew.isWaiting = false;
            table.isTaken = true;
            table.takenSince = event.time;
            report.push_back(event.time + " 12 " + clientNew.name + " " + std::to_string(table.number));
        } else {
            ++freeTables;
        }
        clients.erase(event.name);
    }
}

void Club::generateReport() {
    std::cout << openingTime << std::endl;

    for (const auto &event: report) {
        std::cout << event << std::endl;
    }

    std::cout << closingTime << std::endl;

    for (const auto &table: tables) {
        int hours = table.howLongTaken_m / 60;
        int mins = table.howLongTaken_m % 60;
        std::string hoursStr = std::to_string(hours);
        std::string minsStr = std::to_string(mins);
        std::string howLongTakenStr = (hours < 10 ? "0" + hoursStr : hoursStr) + ":" + (
                                          mins < 10 ? "0" + minsStr : minsStr);
        std::cout << table.number << " " << table.cash << " " << howLongTakenStr << std::endl;
    }
}

void Club::printReport() {
    generateReport();
}

bool ValidEvent(std::string &line) {
    std::regex numberPattern(R"(^[1-9]\d*$)");
    std::regex timePattern(R"((0[0-9]|1[0-9]|2[0-3]):([0-5][0-9]))");
    std::regex clientNamePattern(R"((^[a-z0-9_-]+$))");

    std::istringstream iss(line);
    std::vector<std::string> words;

    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }

    if (((words[1] == "2") && (words.size() < 4)) || ((words[1] != "2") && (words.size() >= 4))) {
        return false;
    }
    if (!std::regex_match(words[0], timePattern)) {
        return false;
    }
    if (words[1] != "1" && words[1] != "2" && words[1] != "3" && words[1] != "4") {
        return false;
    }
    if (!std::regex_match(words[2], clientNamePattern)) {
        return false;
    }

    if ((words[1] == "2") && (!std::regex_match(words[3], numberPattern))) {
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "No arguments with file name" << std::endl;
        return 1;
    }

    std::string myFile = argv[1];
    std::ifstream inputFile(myFile);

    if (!inputFile.is_open()) {
        std::cout << "No such file" << std::endl;
        return 1;
    }

    std::string fileLine;

    std::getline(inputFile, fileLine);
    std::regex numberPattern(R"(^[1-9]\d*$)");
    if (!std::regex_match(fileLine, numberPattern)) {
        std::cout << fileLine << std::endl;
        return 1;
    }
    int numberOfTables = std::stoi(fileLine);


    std::getline(inputFile, fileLine);
    std::regex timePattern2(R"((0[0-9]|1[0-9]|2[0-3]):([0-5][0-9]) (0[0-9]|1[0-9]|2[0-3]):([0-5][0-9]))");
    if (!std::regex_match(fileLine, timePattern2)) {
        std::cout << fileLine << std::endl;
        return 1;
    }

    std::string openingTime = fileLine.substr(0, 5);
    std::string closingTime = fileLine.substr(6, 5);


    std::getline(inputFile, fileLine);
    if (!std::regex_match(fileLine, numberPattern)) {
        std::cout << fileLine << std::endl;
        return 1;
    }
    int costHour = std::stoi(fileLine);

    Club club(numberOfTables, openingTime, closingTime, costHour);


    while (std::getline(inputFile, fileLine)) {
        if (!ValidEvent(fileLine)) {
            std::cout << fileLine;
            return 1;
        }
        club.addEvent(fileLine);
    }

    club.closingClub();

    club.printReport();

    inputFile.close();
    return 0;
}
