#pragma once

#include <memory>
#include <string>

#include "ai_parser.h"
#include "food.h"

using namespace std;

class FoodFactory {
public:
    static unique_ptr<Food> createFood(const AiFoodInput& input,
                                       const NutrientData& nutrient) {
        return createFood(input.type, input.name, input.weight, nutrient);
    }

    static unique_ptr<Food> createFood(const string& type,
                                       const string& name,
                                       double weight,
                                       const NutrientData& nutrient) {
        if (type == "主食") {
            return unique_ptr<Food>(new StapleFood(name, weight, nutrient));
        }
        if (type == "肉类") {
            return unique_ptr<Food>(new MeatFood(name, weight, nutrient));
        }
        if (type == "蔬菜") {
            return unique_ptr<Food>(new VegetableFood(name, weight, nutrient));
        }
        if (type == "水果") {
            return unique_ptr<Food>(new FruitFood(name, weight, nutrient));
        }

        return nullptr;
    }
};
