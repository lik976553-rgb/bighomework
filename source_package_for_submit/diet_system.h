#pragma once

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ai_parser.h"
#include "diet_manager.h"
#include "food_factory.h"
#include "nutrition_db.h"
#include "record_manager.h"
#include "user_manager.h"

using namespace std;

class DietSystem {
private:
    UserManager userManager;
    NutritionDatabase database;
    DietManager manager;
    bool databaseLoaded = false;
    string currentUsername;

public:
    bool init(const string& nutritionDbFile = "nutrition_db.csv") {
        userManager.load();
        databaseLoaded = database.loadFromCsv(nutritionDbFile);
        return databaseLoaded;
    }

    string registerUser(const string& username, const string& password) {
        string message;
        userManager.registerUser(username, password, &message);
        return message;
    }

    string registerUser(const string& username,
                        const string& password,
                        int age,
                        const string& gender,
                        double heightCm,
                        double weightKg,
                        const string& activityLevel,
                        const string& healthGoal) {
        string message;
        userManager.registerUser(username, password, age, gender, heightCm, weightKg, activityLevel, healthGoal, &message);
        return message;
    }

    string login(const string& username, const string& password) {
        string message;
        if (userManager.login(username, password, &message)) {
            currentUsername = username;
            manager.clear();
        }
        return message;
    }

    string logout() {
        if (!isLoggedIn()) {
            return "尚未登录";
        }

        string oldUsername = currentUsername;
        currentUsername.clear();
        manager.clear();
        return oldUsername + " 已退出登录";
    }

    bool isLoggedIn() const {
        return !currentUsername.empty();
    }

    string getCurrentUsername() const {
        return currentUsername;
    }

    int getCurrentUserAge() const {
        return isLoggedIn() ? userManager.getAge(currentUsername) : 0;
    }

    string getCurrentUserGender() const {
        return isLoggedIn() ? userManager.getGender(currentUsername) : "";
    }

    double getCurrentUserHeightCm() const {
        return isLoggedIn() ? userManager.getHeightCm(currentUsername) : 0.0;
    }

    double getCurrentUserWeightKg() const {
        return isLoggedIn() ? userManager.getWeightKg(currentUsername) : 0.0;
    }

    string getCurrentUserActivityLevel() const {
        return isLoggedIn() ? userManager.getActivityLevel(currentUsername) : "";
    }

    string getCurrentUserHealthGoal() const {
        return isLoggedIn() ? userManager.getHealthGoal(currentUsername) : "";
    }

    string updateCurrentUserProfile(int age,
                                    const string& gender,
                                    double heightCm,
                                    double weightKg,
                                    const string& activityLevel,
                                    const string& healthGoal) {
        if (!isLoggedIn()) {
            return "请先登录";
        }

        string message;
        userManager.updateProfile(currentUsername, age, gender, heightCm, weightKg, activityLevel, healthGoal, &message);
        return message;
    }

    string addFoodsFromAiJson(const string& aiJson) {
        if (!isLoggedIn()) {
            return "请先登录";
        }

        if (!databaseLoaded) {
            return "营养数据库尚未加载";
        }

        string error;
        vector<AiFoodInput> inputs = AiFoodParser::parse(aiJson, &error);

        if (!error.empty()) {
            return "AI JSON解析失败: " + error;
        }

        if (inputs.empty()) {
            return "AI JSON中没有有效食物";
        }

        int addedCount = 0;
        vector<string> failedFoods;

        for (const auto& input : inputs) {
            NutrientData nutrient;
            if (!database.find(input.name, nutrient)) {
                failedFoods.push_back(input.name + "(营养库未找到)");
                continue;
            }

            auto food = FoodFactory::createFood(input, nutrient);
            if (!food) {
                failedFoods.push_back(input.name + "(未知类型:" + input.type + ")");
                continue;
            }

            manager.addFood(move(food));
            addedCount++;
        }

        stringstream result;
        result << "成功添加 " << addedCount << " 种食物";

        if (!failedFoods.empty()) {
            result << "，失败 " << failedFoods.size() << " 种: ";
            for (size_t i = 0; i < failedFoods.size(); i++) {
                if (i > 0) result << "、";
                result << failedFoods[i];
            }
        }

        return result.str();
    }

    string saveCurrentRecord() const {
        if (!isLoggedIn()) {
            return "请先登录";
        }

        if (manager.empty()) {
            return "当前没有可保存的饮食记录";
        }

        bool ok = RecordManager::saveRecord(
            currentUsername,
            manager.getFoods(),
            manager.getTotalNutrition()
        );

        return ok ? "记录保存成功" : "记录保存失败";
    }

    NutrientData getTotalNutrition() const {
        return manager.getTotalNutrition();
    }

    const vector<unique_ptr<Food>>& getFoods() const {
        return manager.getFoods();
    }

    int foodCount() const {
        return manager.size();
    }

    void clearFoods() {
        manager.clear();
    }

    bool isDatabaseLoaded() const {
        return databaseLoaded;
    }
};
