#pragma once

#include <memory>
#include <vector>

#include "food.h"

using namespace std;

class DietManager {
private:
    vector<unique_ptr<Food>> foods;

public:
    void addFood(unique_ptr<Food> food) {
        if (food) {
            foods.push_back(move(food));
        }
    }

    bool removeFood(int index) {
        if (index < 0 || index >= static_cast<int>(foods.size())) {
            return false;
        }

        foods.erase(foods.begin() + index);
        return true;
    }

    void clear() {
        foods.clear();
    }

    int size() const {
        return static_cast<int>(foods.size());
    }

    bool empty() const {
        return foods.empty();
    }

    const vector<unique_ptr<Food>>& getFoods() const {
        return foods;
    }

    NutrientData getTotalNutrition() const {
        NutrientData total;

        for (const auto& food : foods) {
            total = total + food->getActualIntake();
        }

        return total;
    }
};
