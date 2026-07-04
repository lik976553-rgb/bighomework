#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "food.h"

using namespace std;

class NutritionDatabase {
private:
    unordered_map<string, NutrientData> nutrients;

    static double toDouble(const string& text) {
        if (text.empty()) return 0.0;
        return stod(text);
    }

public:
    bool loadFromCsv(const string& filename) {
        ifstream file(filename);
        if (!file) return false;

        nutrients.clear();

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            string name;
            string type;
            string value;

            if (!getline(ss, name, ',')) continue;
            if (!getline(ss, type, ',')) continue;

            NutrientData data;

            if (getline(ss, value, ',')) data.calorie = toDouble(value);
            if (getline(ss, value, ',')) data.protein = toDouble(value);
            if (getline(ss, value, ',')) data.fat = toDouble(value);
            if (getline(ss, value, ',')) data.carb = toDouble(value);
            if (getline(ss, value, ',')) data.fiber = toDouble(value);
            if (getline(ss, value, ',')) data.sugar = toDouble(value);
            if (getline(ss, value, ',')) data.cholesterol = toDouble(value);
            if (getline(ss, value, ',')) data.vitaminC = toDouble(value);
            if (getline(ss, value, ',')) data.sodium = toDouble(value);

            if (!name.empty()) {
                nutrients[name] = data;
            }
        }

        return true;
    }

    bool find(const string& foodName, NutrientData& out) const {
        auto it = nutrients.find(foodName);
        if (it != nutrients.end()) {
            out = it->second;
            return true;
        }

        for (const auto& item : nutrients) {
            const string& csvName = item.first;
            if (foodName.find(csvName) != string::npos ||
                csvName.find(foodName) != string::npos) {
                out = item.second;
                return true;
            }
        }

        return false;
    }

    string findMatchedName(const string& foodName) const {
        auto it = nutrients.find(foodName);
        if (it != nutrients.end()) {
            return it->first;
        }

        for (const auto& item : nutrients) {
            const string& csvName = item.first;
            if (foodName.find(csvName) != string::npos ||
                csvName.find(foodName) != string::npos) {
                return csvName;
            }
        }

        return "";
    }

    bool contains(const string& foodName) const {
        NutrientData ignored;
        return find(foodName, ignored);
    }

    int size() const {
        return static_cast<int>(nutrients.size());
    }
};
