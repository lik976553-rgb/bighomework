#pragma once

#include <string>

using namespace std;

// Nutrition values per 100g or actual intake.
struct NutrientData {
    double calorie = 0.0;      // kcal
    double protein = 0.0;      // g
    double fat = 0.0;          // g
    double carb = 0.0;         // g
    double fiber = 0.0;        // g
    double sugar = 0.0;        // g
    double cholesterol = 0.0;  // mg
    double vitaminC = 0.0;     // mg
    double sodium = 0.0;       // mg

    NutrientData operator+(const NutrientData& other) const {
        NutrientData result;
        result.calorie = calorie + other.calorie;
        result.protein = protein + other.protein;
        result.fat = fat + other.fat;
        result.carb = carb + other.carb;
        result.fiber = fiber + other.fiber;
        result.sugar = sugar + other.sugar;
        result.cholesterol = cholesterol + other.cholesterol;
        result.vitaminC = vitaminC + other.vitaminC;
        result.sodium = sodium + other.sodium;
        return result;
    }
};

class Food {
protected:
    string name;
    double weight;          // g
    NutrientData per100g;

public:
    Food(const string& foodName, double foodWeight, const NutrientData& nutrient)
        : name(foodName), weight(foodWeight), per100g(nutrient) {}

    virtual ~Food() = default;

    virtual string getType() const = 0;

    string getName() const {
        return name;
    }

    double getWeight() const {
        return weight;
    }

    NutrientData getPer100g() const {
        return per100g;
    }

    NutrientData getActualIntake() const {
        double ratio = weight / 100.0;
        NutrientData actual;
        actual.calorie = per100g.calorie * ratio;
        actual.protein = per100g.protein * ratio;
        actual.fat = per100g.fat * ratio;
        actual.carb = per100g.carb * ratio;
        actual.fiber = per100g.fiber * ratio;
        actual.sugar = per100g.sugar * ratio;
        actual.cholesterol = per100g.cholesterol * ratio;
        actual.vitaminC = per100g.vitaminC * ratio;
        actual.sodium = per100g.sodium * ratio;
        return actual;
    }
};

class StapleFood : public Food {
public:
    StapleFood(const string& name, double weight, const NutrientData& nutrient)
        : Food(name, weight, nutrient) {}

    string getType() const override {
        return "主食";
    }
};

class MeatFood : public Food {
public:
    MeatFood(const string& name, double weight, const NutrientData& nutrient)
        : Food(name, weight, nutrient) {}

    string getType() const override {
        return "肉类";
    }
};

class VegetableFood : public Food {
public:
    VegetableFood(const string& name, double weight, const NutrientData& nutrient)
        : Food(name, weight, nutrient) {}

    string getType() const override {
        return "蔬菜";
    }
};

class FruitFood : public Food {
public:
    FruitFood(const string& name, double weight, const NutrientData& nutrient)
        : Food(name, weight, nutrient) {}

    string getType() const override {
        return "水果";
    }
};
