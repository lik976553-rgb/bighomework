#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct User {
    string username;
    string password;
    int age = 0;
    string gender;
    double heightCm = 0.0;
    double weightKg = 0.0;
    string activityLevel;
    string healthGoal;
};

class UserManager {
private:
    string filename;
    vector<User> users;

    User* findUser(const string& username) {
        for (auto& user : users) {
            if (user.username == username) {
                return &user;
            }
        }
        return nullptr;
    }

    const User* findUser(const string& username) const {
        for (const auto& user : users) {
            if (user.username == username) {
                return &user;
            }
        }
        return nullptr;
    }

    bool saveAll() const {
        ofstream file(filename);
        if (!file) return false;

        for (const auto& user : users) {
            file << user.username << ","
                 << user.password << ","
                 << user.age << ","
                 << user.gender << ","
                 << user.heightCm << ","
                 << user.weightKg << ","
                 << user.activityLevel << ","
                 << user.healthGoal << "\n";
        }

        return true;
    }

public:
    explicit UserManager(const string& userFile = "users.dat")
        : filename(userFile) {
        load();
    }

    bool load() {
        users.clear();

        ifstream file(filename);
        if (!file) return false;

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            User user;

            getline(ss, user.username, ',');
            getline(ss, user.password, ',');

            string ageText;
            getline(ss, ageText, ',');
            getline(ss, user.gender, ',');

            string heightText;
            string weightText;
            getline(ss, heightText, ',');
            getline(ss, weightText, ',');
            getline(ss, user.activityLevel, ',');
            getline(ss, user.healthGoal, ',');

            if (!ageText.empty()) {
                try {
                    user.age = stoi(ageText);
                } catch (...) {
                    user.age = 0;
                }
            }
            if (!heightText.empty()) {
                try {
                    user.heightCm = stod(heightText);
                } catch (...) {
                    user.heightCm = 0.0;
                }
            }
            if (!weightText.empty()) {
                try {
                    user.weightKg = stod(weightText);
                } catch (...) {
                    user.weightKg = 0.0;
                }
            }

            if (!user.username.empty()) {
                users.push_back(user);
            }
        }

        return true;
    }

    bool registerUser(const string& username,
                      const string& password,
                      int age,
                      const string& gender,
                      double heightCm,
                      double weightKg,
                      const string& activityLevel,
                      const string& healthGoal,
                      string* message = nullptr) {
        if (username.empty() || password.empty()) {
            if (message) *message = "用户名和密码不能为空";
            return false;
        }

        if (username.find(',') != string::npos ||
            password.find(',') != string::npos ||
            gender.find(',') != string::npos ||
            activityLevel.find(',') != string::npos ||
            healthGoal.find(',') != string::npos) {
            if (message) *message = "用户名、密码、性别、活动水平和健康目标不能包含逗号";
            return false;
        }

        if (age <= 0 || age > 120) {
            if (message) *message = "年龄需要在 1 到 120 之间";
            return false;
        }

        if (gender.empty()) {
            if (message) *message = "请选择性别";
            return false;
        }

        if (heightCm <= 0.0 || heightCm > 250.0) {
            if (message) *message = "身高需要在 1 到 250 cm 之间";
            return false;
        }

        if (weightKg <= 0.0 || weightKg > 300.0) {
            if (message) *message = "体重需要在 1 到 300 kg 之间";
            return false;
        }

        if (activityLevel.empty()) {
            if (message) *message = "请选择活动水平";
            return false;
        }

        if (healthGoal.empty()) {
            if (message) *message = "请选择健康目标";
            return false;
        }

        if (findUser(username)) {
            if (message) *message = "用户已存在";
            return false;
        }

        users.push_back({username, password, age, gender, heightCm, weightKg, activityLevel, healthGoal});

        ofstream file(filename, ios::app);
        if (!file) {
            if (message) *message = "无法写入用户文件";
            return false;
        }

        file << username << "," << password << "," << age << "," << gender
             << "," << heightCm << "," << weightKg
             << "," << activityLevel << "," << healthGoal << "\n";
        if (message) *message = "注册成功";
        return true;
    }

    bool registerUser(const string& username,
                      const string& password,
                      string* message = nullptr) {
        return registerUser(username, password, 25, "未说明", 170.0, 60.0, "轻度运动", "均衡饮食", message);
    }

    bool login(const string& username,
               const string& password,
               string* message = nullptr) {
        User* user = findUser(username);
        if (!user) {
            if (message) *message = "用户不存在";
            return false;
        }

        if (user->password != password) {
            if (message) *message = "密码错误";
            return false;
        }

        if (message) *message = "登录成功";
        return true;
    }

    bool updateProfile(const string& username,
                       int age,
                       const string& gender,
                       double heightCm,
                       double weightKg,
                       const string& activityLevel,
                       const string& healthGoal,
                       string* message = nullptr) {
        User* user = findUser(username);
        if (!user) {
            if (message) *message = "用户不存在";
            return false;
        }

        if (age <= 0 || age > 120) {
            if (message) *message = "年龄需要在 1 到 120 之间";
            return false;
        }

        if (gender.empty() || gender.find(',') != string::npos) {
            if (message) *message = "性别不能为空且不能包含逗号";
            return false;
        }

        if (heightCm <= 0.0 || heightCm > 250.0) {
            if (message) *message = "身高需要在 1 到 250 cm 之间";
            return false;
        }

        if (weightKg <= 0.0 || weightKg > 300.0) {
            if (message) *message = "体重需要在 1 到 300 kg 之间";
            return false;
        }

        if (activityLevel.empty() || activityLevel.find(',') != string::npos) {
            if (message) *message = "活动水平不能为空且不能包含逗号";
            return false;
        }

        if (healthGoal.empty() || healthGoal.find(',') != string::npos) {
            if (message) *message = "健康目标不能为空且不能包含逗号";
            return false;
        }

        user->age = age;
        user->gender = gender;
        user->heightCm = heightCm;
        user->weightKg = weightKg;
        user->activityLevel = activityLevel;
        user->healthGoal = healthGoal;

        if (!saveAll()) {
            if (message) *message = "无法写入用户文件";
            return false;
        }

        if (message) *message = "资料更新成功";
        return true;
    }

    int getAge(const string& username) const {
        const User* user = findUser(username);
        return user ? user->age : 0;
    }

    string getGender(const string& username) const {
        const User* user = findUser(username);
        return user ? user->gender : "";
    }

    double getHeightCm(const string& username) const {
        const User* user = findUser(username);
        return user ? user->heightCm : 0.0;
    }

    double getWeightKg(const string& username) const {
        const User* user = findUser(username);
        return user ? user->weightKg : 0.0;
    }

    string getActivityLevel(const string& username) const {
        const User* user = findUser(username);
        return user ? user->activityLevel : "";
    }

    string getHealthGoal(const string& username) const {
        const User* user = findUser(username);
        return user ? user->healthGoal : "";
    }
};
