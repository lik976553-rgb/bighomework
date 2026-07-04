#pragma once

#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "food.h"

using namespace std;

class RecordManager {
private:
    static string escapeJson(const string& text) {
        string result;
        for (char ch : text) {
            if (ch == '"' || ch == '\\') {
                result += '\\';
            }
            result += ch;
        }
        return result;
    }

    static string safeUsername(const string& username) {
        string result;
        for (char ch : username) {
            if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' ||
                ch == '?' || ch == '"' || ch == '<' || ch == '>' ||
                ch == '|') {
                result += '_';
            } else {
                result += ch;
            }
        }
        return result;
    }

    static string currentDateTime() {
        time_t now = time(nullptr);
        tm* local = localtime(&now);

        stringstream ss;
        ss << put_time(local, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    static string nutrientToJson(const NutrientData& data) {
        stringstream ss;
        ss << fixed << setprecision(1);
        ss << "{"
           << "\"calorie\":" << data.calorie << ","
           << "\"protein\":" << data.protein << ","
           << "\"fat\":" << data.fat << ","
           << "\"carb\":" << data.carb << ","
           << "\"fiber\":" << data.fiber << ","
           << "\"sugar\":" << data.sugar << ","
           << "\"cholesterol\":" << data.cholesterol << ","
           << "\"vitaminC\":" << data.vitaminC << ","
           << "\"sodium\":" << data.sodium
           << "}";
        return ss.str();
    }

public:
    static string filename(const string& username) {
        return "diet_record_" + safeUsername(username) + ".jsonl";
    }

    static bool saveRecord(const string& username,
                           const vector<unique_ptr<Food>>& foods,
                           const NutrientData& total) {
        if (username.empty() || foods.empty()) {
            return false;
        }

        ofstream file(filename(username), ios::app);
        if (!file) return false;

        file << fixed << setprecision(1);
        file << "{\"time\":\"" << currentDateTime() << "\",";
        file << "\"foods\":[";

        for (size_t i = 0; i < foods.size(); i++) {
            const auto& food = foods[i];
            if (i > 0) file << ",";

            file << "{"
                 << "\"type\":\"" << escapeJson(food->getType()) << "\","
                 << "\"name\":\"" << escapeJson(food->getName()) << "\","
                 << "\"weight\":" << food->getWeight()
                 << "}";
        }

        file << "],";
        file << "\"total\":" << nutrientToJson(total);
        file << "}\n";

        return true;
    }
};
