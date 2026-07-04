#include <QApplication>
#include <QAbstractItemView>
#include <QByteArray>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTimer>
#include <QTextStream>
#include <QUrl>
#include <QVector>

#include <string>
#include <functional>
#include <memory>

#include "deepseek_config.h"
#include "diet_system.h"

using namespace std;

static string toUtf8String(const QString& text) {
    QByteArray bytes = text.toUtf8();
    return string(bytes.constData(), static_cast<size_t>(bytes.size()));
}

static QString fromUtf8String(const string& text) {
    return QString::fromUtf8(text.data(), static_cast<int>(text.size()));
}

static QString jsonEscape(QString text) {
    text.replace("\\", "\\\\");
    text.replace("\"", "\\\"");
    return text;
}

static QString findNutritionDbFile() {
    QVector<QString> candidates;

#ifdef NUTRITION_DB_PATH
    candidates.push_back(QString::fromUtf8(NUTRITION_DB_PATH));
#endif
    candidates.push_back(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("nutrition_db.csv"));
    candidates.push_back(QDir::current().absoluteFilePath("nutrition_db.csv"));

    const QDir appDir(QCoreApplication::applicationDirPath());
    candidates.push_back(appDir.absoluteFilePath("../nutrition_db.csv"));
    candidates.push_back(appDir.absoluteFilePath("../../nutrition_db.csv"));

    for (const QString& candidate : candidates) {
        QFileInfo fileInfo(QDir::cleanPath(candidate));
        if (fileInfo.exists() && fileInfo.isFile()) {
            return fileInfo.absoluteFilePath();
        }
    }

    return QDir::current().absoluteFilePath("nutrition_db.csv");
}

static string prepareNutritionDbPath() {
    QFileInfo dbFile(findNutritionDbFile());
    if (dbFile.exists() && dbFile.isFile()) {
        QDir::setCurrent(dbFile.absolutePath());
    }

    // The backend uses std::ifstream. Opening the relative file name after
    // QDir::setCurrent avoids Windows UTF-8 path issues in Chinese folders.
    return "nutrition_db.csv";
}

struct CatalogFood {
    QString name;
    QString type;
};

class DietWindow : public QWidget {
private:
    DietSystem system;
    string nutritionDbPath;
    QNetworkAccessManager networkManager;
    bool apiBusy = false;

    QLabel* statusLabel = nullptr;
    QLabel* currentUserLabel = nullptr;
    QLineEdit* usernameEdit = nullptr;
    QLineEdit* passwordEdit = nullptr;
    QSpinBox* ageSpin = nullptr;
    QDoubleSpinBox* heightSpin = nullptr;
    QDoubleSpinBox* bodyWeightSpin = nullptr;
    QComboBox* genderCombo = nullptr;
    QComboBox* activityCombo = nullptr;
    QComboBox* goalCombo = nullptr;
    QComboBox* typeCombo = nullptr;
    QLineEdit* foodNameEdit = nullptr;
    QDoubleSpinBox* weightSpin = nullptr;
    QTextEdit* naturalEdit = nullptr;
    QTableWidget* foodTable = nullptr;
    QLabel* calorieLabel = nullptr;
    QLabel* proteinLabel = nullptr;
    QLabel* fatLabel = nullptr;
    QLabel* carbLabel = nullptr;
    QLabel* fiberLabel = nullptr;
    QLabel* sugarLabel = nullptr;
    QLabel* cholesterolLabel = nullptr;
    QLabel* vitaminCLabel = nullptr;
    QLabel* sodiumLabel = nullptr;

public:
    DietWindow() {
        setWindowTitle("每日膳食营养统计");
        resize(920, 680);
        setMinimumSize(960, 720);

        buildUi();
        applyAppStyle();

        const string dbPath = prepareNutritionDbPath();
        nutritionDbPath = dbPath;

        if (system.init(dbPath)) {
            setStatus("营养数据库加载成功");
        } else {
            setStatus("营养数据库加载失败，请检查 nutrition_db.csv");
        }

        updateView();
    }

private:
    void applyAppStyle() {
        setStyleSheet(
            "QWidget {"
            "  background: #f5f7fb;"
            "  color: #1f2937;"
            "  font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;"
            "  font-size: 13px;"
            "}"
            "QGroupBox {"
            "  background: #ffffff;"
            "  border: 1px solid #d7dde8;"
            "  border-radius: 8px;"
            "  margin-top: 14px;"
            "  padding: 12px;"
            "  font-weight: 600;"
            "  color: #27364a;"
            "}"
            "QGroupBox::title {"
            "  subcontrol-origin: margin;"
            "  left: 12px;"
            "  padding: 0 6px;"
            "  background: #f5f7fb;"
            "}"
            "QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
            "  background: #ffffff;"
            "  border: 1px solid #cbd5e1;"
            "  border-radius: 6px;"
            "  padding: 6px 8px;"
            "  selection-background-color: #b7d7ff;"
            "}"
            "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
            "  border: 1px solid #4f8df7;"
            "}"
            "QPushButton {"
            "  background: #eef4ff;"
            "  color: #1d4f91;"
            "  border: 1px solid #b8cff5;"
            "  border-radius: 6px;"
            "  padding: 7px 12px;"
            "  font-weight: 600;"
            "}"
            "QPushButton:hover {"
            "  background: #dfeaff;"
            "}"
            "QPushButton:pressed {"
            "  background: #cfe0ff;"
            "}"
            "QPushButton:disabled {"
            "  color: #94a3b8;"
            "  background: #edf1f7;"
            "  border-color: #d6deea;"
            "}"
            "QTableWidget {"
            "  background: #ffffff;"
            "  alternate-background-color: #f8fafc;"
            "  gridline-color: #e2e8f0;"
            "  border: 1px solid #d7dde8;"
            "  border-radius: 6px;"
            "}"
            "QHeaderView::section {"
            "  background: #eef3f9;"
            "  color: #334155;"
            "  border: 0;"
            "  border-right: 1px solid #d7dde8;"
            "  padding: 7px;"
            "  font-weight: 600;"
            "}"
            "QTableWidget::item {"
            "  padding: 6px;"
            "}"
            "QTableWidget::item:selected {"
            "  background: #dbeafe;"
            "  color: #1e3a8a;"
            "}"
            "QLabel#CurrentUserLabel {"
            "  background: #eaf2ff;"
            "  color: #1e3a8a;"
            "  border: 1px solid #c7dbff;"
            "  border-radius: 8px;"
            "  padding: 9px 12px;"
            "  font-weight: 600;"
            "}"
            "QLabel#StatusLabel {"
            "  background: #fff8e6;"
            "  color: #7a4d00;"
            "  border: 1px solid #f4d483;"
            "  border-radius: 8px;"
            "  padding: 8px 12px;"
            "}"
        );
    }

    void buildUi() {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(18, 18, 18, 18);
        root->setSpacing(12);

        currentUserLabel = new QLabel(this);
        currentUserLabel->setObjectName("CurrentUserLabel");
        currentUserLabel->setWordWrap(true);
        statusLabel = new QLabel(this);
        statusLabel->setObjectName("StatusLabel");
        statusLabel->setWordWrap(true);

        root->addWidget(currentUserLabel);
        root->addWidget(createUserBox());
        root->addWidget(createInputBox());
        root->addWidget(createTableBox(), 1);
        root->addWidget(createTotalBox());

        auto* actionLayout = new QHBoxLayout;
        auto* saveButton = new QPushButton("保存今日记录", this);
        auto* historyButton = new QPushButton("查看历史记录", this);
        auto* currentReportButton = new QPushButton("生成当前报告", this);
        auto* historyReportButton = new QPushButton("生成历史报告", this);
        auto* clearButton = new QPushButton("清空当前食物", this);
        actionLayout->addWidget(saveButton);
        actionLayout->addWidget(historyButton);
        actionLayout->addWidget(currentReportButton);
        actionLayout->addWidget(historyReportButton);
        actionLayout->addWidget(clearButton);
        actionLayout->addStretch();
        root->addLayout(actionLayout);
        root->addWidget(statusLabel);

        connect(saveButton, &QPushButton::clicked, this, [this]() {
            setStatus(fromUtf8String(system.saveCurrentRecord()));
        });

        connect(historyButton, &QPushButton::clicked, this, [this]() {
            showHistoryRecords();
        });

        connect(currentReportButton, &QPushButton::clicked, this, [this]() {
            generateCurrentReport();
        });

        connect(historyReportButton, &QPushButton::clicked, this, [this]() {
            generateHistoryReport();
        });

        connect(clearButton, &QPushButton::clicked, this, [this]() {
            system.clearFoods();
            setStatus("已清空当前食物");
            updateView();
        });
    }

    QGroupBox* createUserBox() {
        auto* box = new QGroupBox("用户", this);
        auto* layout = new QGridLayout(box);

        usernameEdit = new QLineEdit(box);
        passwordEdit = new QLineEdit(box);
        ageSpin = new QSpinBox(box);
        heightSpin = new QDoubleSpinBox(box);
        bodyWeightSpin = new QDoubleSpinBox(box);
        genderCombo = new QComboBox(box);
        activityCombo = new QComboBox(box);
        goalCombo = new QComboBox(box);
        passwordEdit->setEchoMode(QLineEdit::Password);
        usernameEdit->setPlaceholderText("用户名");
        passwordEdit->setPlaceholderText("密码");
        ageSpin->setRange(1, 120);
        ageSpin->setValue(25);
        heightSpin->setRange(1.0, 250.0);
        heightSpin->setDecimals(1);
        heightSpin->setValue(170.0);
        heightSpin->setSuffix(" cm");
        bodyWeightSpin->setRange(1.0, 300.0);
        bodyWeightSpin->setDecimals(1);
        bodyWeightSpin->setValue(60.0);
        bodyWeightSpin->setSuffix(" kg");
        genderCombo->addItems({"男", "女", "未说明"});
        activityCombo->addItems({"久坐", "轻度运动", "中度运动", "高强度运动"});
        goalCombo->addItems({"减脂", "维持", "增肌", "控糖", "均衡饮食"});

        auto* registerButton = new QPushButton("注册", box);
        auto* loginButton = new QPushButton("登录", box);
        auto* updateProfileButton = new QPushButton("更新资料", box);
        auto* logoutButton = new QPushButton("退出登录", box);

        layout->addWidget(new QLabel("用户名", box), 0, 0);
        layout->addWidget(usernameEdit, 0, 1);
        layout->addWidget(new QLabel("密码", box), 0, 2);
        layout->addWidget(passwordEdit, 0, 3);
        layout->addWidget(new QLabel("年龄", box), 0, 4);
        layout->addWidget(ageSpin, 0, 5);
        layout->addWidget(new QLabel("性别", box), 0, 6);
        layout->addWidget(genderCombo, 0, 7);

        layout->addWidget(new QLabel("身高", box), 1, 0);
        layout->addWidget(heightSpin, 1, 1);
        layout->addWidget(new QLabel("体重", box), 1, 2);
        layout->addWidget(bodyWeightSpin, 1, 3);
        layout->addWidget(new QLabel("活动", box), 1, 4);
        layout->addWidget(activityCombo, 1, 5);
        layout->addWidget(new QLabel("目标", box), 1, 6);
        layout->addWidget(goalCombo, 1, 7);

        auto* buttonLayout = new QHBoxLayout;
        buttonLayout->addWidget(registerButton);
        buttonLayout->addWidget(loginButton);
        buttonLayout->addWidget(updateProfileButton);
        buttonLayout->addWidget(logoutButton);
        buttonLayout->addStretch();
        layout->addLayout(buttonLayout, 2, 0, 1, 8);

        connect(registerButton, &QPushButton::clicked, this, [this]() {
            setStatus(fromUtf8String(system.registerUser(
                toUtf8String(usernameEdit->text()),
                toUtf8String(passwordEdit->text()),
                ageSpin->value(),
                toUtf8String(genderCombo->currentText()),
                heightSpin->value(),
                bodyWeightSpin->value(),
                toUtf8String(activityCombo->currentText()),
                toUtf8String(goalCombo->currentText())
            )));
            updateView();
        });

        connect(loginButton, &QPushButton::clicked, this, [this]() {
            setStatus(fromUtf8String(system.login(
                toUtf8String(usernameEdit->text()),
                toUtf8String(passwordEdit->text())
            )));
            updateView();
        });

        connect(updateProfileButton, &QPushButton::clicked, this, [this]() {
            setStatus(fromUtf8String(system.updateCurrentUserProfile(
                ageSpin->value(),
                toUtf8String(genderCombo->currentText()),
                heightSpin->value(),
                bodyWeightSpin->value(),
                toUtf8String(activityCombo->currentText()),
                toUtf8String(goalCombo->currentText())
            )));
            updateView();
        });

        connect(logoutButton, &QPushButton::clicked, this, [this]() {
            setStatus(fromUtf8String(system.logout()));
            updateView();
        });

        return box;
    }

    QGroupBox* createInputBox() {
        auto* box = new QGroupBox("添加食物", this);
        auto* layout = new QVBoxLayout(box);
        layout->setSpacing(6);

        auto* manualLayout = new QHBoxLayout;
        manualLayout->setSpacing(8);
        typeCombo = new QComboBox(box);
        typeCombo->addItems({"主食", "肉类", "蔬菜", "水果"});

        foodNameEdit = new QLineEdit(box);
        foodNameEdit->setPlaceholderText("例如：米饭、鸡胸肉、苹果");

        weightSpin = new QDoubleSpinBox(box);
        weightSpin->setRange(1.0, 5000.0);
        weightSpin->setDecimals(1);
        weightSpin->setValue(100.0);
        weightSpin->setSuffix(" g");

        auto* addOneButton = new QPushButton("添加", box);

        manualLayout->addWidget(new QLabel("类型", box));
        manualLayout->addWidget(typeCombo);
        manualLayout->addWidget(new QLabel("食物", box));
        manualLayout->addWidget(foodNameEdit, 1);
        manualLayout->addWidget(new QLabel("重量", box));
        manualLayout->addWidget(weightSpin);
        manualLayout->addWidget(addOneButton);

        naturalEdit = new QTextEdit(box);
        naturalEdit->setMinimumHeight(44);
        naturalEdit->setMaximumHeight(58);
        naturalEdit->setPlaceholderText("例如：今天午餐吃了150克米饭、120克鸡胸肉和100克西兰花");

        auto* naturalButtonLayout = new QHBoxLayout;
        naturalButtonLayout->setContentsMargins(0, 0, 0, 0);
        auto* parseNaturalButton = new QPushButton("解析语句并添加", box);
        naturalButtonLayout->addWidget(parseNaturalButton);
        naturalButtonLayout->addStretch();

        layout->addLayout(manualLayout);
        layout->addWidget(naturalEdit);
        layout->addLayout(naturalButtonLayout);

        connect(addOneButton, &QPushButton::clicked, this, [this]() {
            const QString name = foodNameEdit->text().trimmed();
            if (name.isEmpty()) {
                setStatus("请输入食物名称");
                return;
            }

            QString json = QString("[{\"type\":\"%1\",\"name\":\"%2\",\"weight\":%3}]")
                .arg(jsonEscape(typeCombo->currentText()))
                .arg(jsonEscape(name))
                .arg(weightSpin->value(), 0, 'f', 1);

            addFoodsJsonAsync(json);
        });

        connect(parseNaturalButton, &QPushButton::clicked, this, [this]() {
            addFoodsFromSentenceAsync(naturalEdit->toPlainText());
        });

        return box;
    }

    QVector<CatalogFood> loadFoodCatalog() const {
        QVector<CatalogFood> catalog;
        QFile file(fromUtf8String(nutritionDbPath.empty() ? "nutrition_db.csv" : nutritionDbPath));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return catalog;
        }

        QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec("UTF-8");
#endif

        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;

            const QStringList columns = line.split(',');
            if (columns.size() < 2) continue;

            CatalogFood food;
            food.name = columns[0].trimmed();
            food.type = columns[1].trimmed();
            if (!food.name.isEmpty() && !food.type.isEmpty()) {
                catalog.push_back(food);
            }
        }

        return catalog;
    }

    double findWeightNearFood(const QString& sentence, const QString& foodName) const {
        const QString escapedName = QRegularExpression::escape(foodName);
        const QString number = "(\\d+(?:\\.\\d+)?)";
        const QString unit = "\\s*(?:克|g|G)";

        QRegularExpression beforePattern(number + unit + "[^，。；、,;]{0,8}" + escapedName);
        QRegularExpressionMatch beforeMatch = beforePattern.match(sentence);
        if (beforeMatch.hasMatch()) {
            return beforeMatch.captured(1).toDouble();
        }

        QRegularExpression afterPattern(escapedName + "[^0-9]{0,8}" + number + unit);
        QRegularExpressionMatch afterMatch = afterPattern.match(sentence);
        if (afterMatch.hasMatch()) {
            return afterMatch.captured(1).toDouble();
        }

        return 100.0;
    }

    QString buildJsonFromSentence(const QString& sentence) const {
        const QString text = sentence.trimmed();
        if (text.isEmpty()) {
            return QString();
        }

        QStringList objects;
        const QVector<CatalogFood> catalog = loadFoodCatalog();

        for (const CatalogFood& food : catalog) {
            if (!text.contains(food.name)) continue;

            const double weight = findWeightNearFood(text, food.name);
            objects << QString("  {\"type\":\"%1\",\"name\":\"%2\",\"weight\":%3}")
                .arg(jsonEscape(food.type))
                .arg(jsonEscape(food.name))
                .arg(weight, 0, 'f', 1);
        }

        if (objects.isEmpty()) {
            return QString();
        }

        return "[\n" + objects.join(",\n") + "\n]";
    }

    QString apiKey() const {
        return QString::fromUtf8(DEEPSEEK_API_KEY).trimmed();
    }

    QString deepSeekModel() const {
        const QString model = QString::fromUtf8(DEEPSEEK_MODEL).trimmed();
        return model.isEmpty() ? "deepseek-v4-flash" : model;
    }

    void setApiBusy(bool busy, const QString& message = QString()) {
        apiBusy = busy;
        if (busy) {
            QApplication::setOverrideCursor(Qt::WaitCursor);
        } else {
            QApplication::restoreOverrideCursor();
        }
        if (!message.isEmpty()) {
            setStatus(message);
        }
    }

    void callDeepSeekAsync(const QString& systemPrompt,
                           const QString& userPrompt,
                           bool jsonMode,
                           std::function<void(const QString&, const QString&)> callback) {
        if (apiKey().isEmpty()) {
            callback(QString(), "DeepSeek API Key 为空，请在 deepseek_config.h 中填写");
            return;
        }

        QNetworkRequest request(QUrl("https://api.deepseek.com/chat/completions"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", ("Bearer " + apiKey()).toUtf8());

        QJsonArray messages;
        messages.append(QJsonObject{{"role", "system"}, {"content", systemPrompt}});
        messages.append(QJsonObject{{"role", "user"}, {"content", userPrompt}});

        QJsonObject body;
        body["model"] = deepSeekModel();
        body["messages"] = messages;
        body["stream"] = false;
        body["temperature"] = 0.2;
        if (jsonMode) {
            body["response_format"] = QJsonObject{{"type", "json_object"}};
        }

        QNetworkReply* reply = networkManager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        auto* timer = new QTimer(reply);
        timer->setSingleShot(true);

        connect(timer, &QTimer::timeout, reply, [reply]() {
            reply->setProperty("timedOut", true);
            reply->abort();
        });

        connect(reply, &QNetworkReply::finished, this, [reply, timer, callback]() {
            timer->stop();

            const QByteArray responseBytes = reply->readAll();
            const bool timedOut = reply->property("timedOut").toBool();
            const QNetworkReply::NetworkError networkError = reply->error();
            const QString networkErrorText = reply->errorString();
            reply->deleteLater();

            if (timedOut) {
                callback(QString(), "DeepSeek 请求超时");
                return;
            }

            if (networkError != QNetworkReply::NoError) {
                callback(QString(), "DeepSeek 请求失败：" + networkErrorText + "\n" + QString::fromUtf8(responseBytes));
                return;
            }

            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(responseBytes, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                callback(QString(), "DeepSeek 返回内容不是合法 JSON：" + parseError.errorString());
                return;
            }

            const QJsonArray choices = doc.object().value("choices").toArray();
            if (choices.isEmpty()) {
                callback(QString(), "DeepSeek 返回结果为空");
                return;
            }

            const QString content = choices.first().toObject()
                .value("message").toObject()
                .value("content").toString()
                .trimmed();

            if (content.isEmpty()) {
                callback(QString(), "DeepSeek 返回文本为空");
                return;
            }

            callback(content, QString());
        });

        timer->start(30000);
    }

    QString callDeepSeek(const QString& systemPrompt,
                         const QString& userPrompt,
                         bool jsonMode,
                         QString* errorMessage) {
        if (apiKey().isEmpty()) {
            if (errorMessage) *errorMessage = "DeepSeek API Key 为空，请在 deepseek_config.h 中填写";
            return QString();
        }

        QNetworkRequest request(QUrl("https://api.deepseek.com/chat/completions"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", ("Bearer " + apiKey()).toUtf8());

        QJsonArray messages;
        messages.append(QJsonObject{{"role", "system"}, {"content", systemPrompt}});
        messages.append(QJsonObject{{"role", "user"}, {"content", userPrompt}});

        QJsonObject body;
        body["model"] = deepSeekModel();
        body["messages"] = messages;
        body["stream"] = false;
        body["temperature"] = 0.2;
        if (jsonMode) {
            body["response_format"] = QJsonObject{{"type", "json_object"}};
        }

        QNetworkReply* reply = networkManager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000);
        loop.exec();

        if (!timer.isActive()) {
            reply->abort();
            reply->deleteLater();
            if (errorMessage) *errorMessage = "DeepSeek 请求超时";
            return QString();
        }
        timer.stop();

        const QByteArray responseBytes = reply->readAll();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkErrorText = reply->errorString();
        reply->deleteLater();

        if (networkError != QNetworkReply::NoError) {
            if (errorMessage) {
                *errorMessage = "DeepSeek 请求失败：" + networkErrorText + "\n" + QString::fromUtf8(responseBytes);
            }
            return QString();
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(responseBytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            if (errorMessage) *errorMessage = "DeepSeek 返回内容不是合法 JSON：" + parseError.errorString();
            return QString();
        }

        const QJsonArray choices = doc.object().value("choices").toArray();
        if (choices.isEmpty()) {
            if (errorMessage) *errorMessage = "DeepSeek 返回结果为空";
            return QString();
        }

        const QString content = choices.first().toObject()
            .value("message").toObject()
            .value("content").toString()
            .trimmed();

        if (content.isEmpty()) {
            if (errorMessage) *errorMessage = "DeepSeek 返回文本为空";
        }

        return content;
    }

    bool foodExistsInCatalog(const QString& foodName) const {
        const QVector<CatalogFood> catalog = loadFoodCatalog();
        for (const CatalogFood& food : catalog) {
            if (food.name == foodName) {
                return true;
            }
        }
        return false;
    }

    QString numberText(const QJsonObject& object, const QString& key) const {
        return QString::number(object.value(key).toDouble(), 'f', 1);
    }

    bool appendFoodToCsvByAi(const QString& foodName,
                             const QString& requestedType,
                             QString* errorMessage) {
        const QString systemPrompt =
            "你是营养数据库助手。请只返回 JSON 对象，不要解释。"
            "营养值必须是每100克可食部分的估算值。"
            "字段必须包含 name,type,calorie,protein,fat,carb,fiber,sugar,cholesterol,vitaminC,sodium。"
            "type 只能是 主食、肉类、蔬菜、水果 之一。"
            "单位：calorie 为 kcal，protein/fat/carb/fiber/sugar 为 g，cholesterol/vitaminC/sodium 为 mg。";

        const QString userPrompt = QString(
            "请查询并估算食物“%1”的营养数据。用户选择的分类是“%2”。"
            "如果用户分类明显合理，请优先使用该分类。"
        ).arg(foodName, requestedType);

        QString apiError;
        const QString content = callDeepSeek(systemPrompt, userPrompt, true, &apiError);
        if (content.isEmpty()) {
            if (errorMessage) *errorMessage = "无法查询未知食物“" + foodName + "”：" + apiError;
            return false;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            if (errorMessage) *errorMessage = "DeepSeek 返回的营养数据格式错误：" + parseError.errorString();
            return false;
        }

        QJsonObject food = doc.object();
        const QString type = food.value("type").toString(requestedType).trimmed();
        const QString typeForCsv = (type == "主食" || type == "肉类" || type == "蔬菜" || type == "水果")
            ? type
            : requestedType;

        QFile file(fromUtf8String(nutritionDbPath.empty() ? "nutrition_db.csv" : nutritionDbPath));
        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            if (errorMessage) *errorMessage = "无法写入营养数据库 CSV";
            return false;
        }

        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#endif
        out << foodName << "," << typeForCsv << ","
            << numberText(food, "calorie") << ","
            << numberText(food, "protein") << ","
            << numberText(food, "fat") << ","
            << numberText(food, "carb") << ","
            << numberText(food, "fiber") << ","
            << numberText(food, "sugar") << ","
            << numberText(food, "cholesterol") << ","
            << numberText(food, "vitaminC") << ","
            << numberText(food, "sodium") << "\n";

        return true;
    }

    QJsonArray foodsArrayFromJsonText(const QString& jsonText, QString* errorMessage) const {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            if (errorMessage) *errorMessage = "食物 JSON 格式错误：" + parseError.errorString();
            return QJsonArray();
        }

        if (doc.isArray()) {
            return doc.array();
        }

        if (doc.isObject()) {
            return doc.object().value("foods").toArray();
        }

        if (errorMessage) *errorMessage = "食物 JSON 必须是数组，或包含 foods 数组的对象";
        return QJsonArray();
    }

    bool ensureFoodsInDatabase(const QString& jsonText, QString* errorMessage) {
        QString parseError;
        const QJsonArray foods = foodsArrayFromJsonText(jsonText, &parseError);
        if (!parseError.isEmpty()) {
            if (errorMessage) *errorMessage = parseError;
            return false;
        }

        bool appended = false;
        for (const QJsonValue& value : foods) {
            const QJsonObject food = value.toObject();
            const QString name = food.value("name").toString().trimmed();
            const QString type = food.value("type").toString("主食").trimmed();
            if (name.isEmpty()) continue;

            if (!foodExistsInCatalog(name)) {
                if (!appendFoodToCsvByAi(name, type, errorMessage)) {
                    return false;
                }
                appended = true;
            }
        }

        if (appended) {
            system.init(nutritionDbPath.empty() ? "nutrition_db.csv" : nutritionDbPath);
        }

        return true;
    }

    QString buildJsonArrayFromFoodsObject(const QJsonObject& object, QString* errorMessage) const {
        const QJsonArray foods = object.value("foods").toArray();
        if (foods.isEmpty()) {
            if (errorMessage) *errorMessage = "DeepSeek 没有识别到食物";
            return QString();
        }

        QStringList objects;
        for (const QJsonValue& value : foods) {
            const QJsonObject food = value.toObject();
            const QString name = food.value("name").toString().trimmed();
            const QString type = food.value("type").toString().trimmed();
            const double weight = food.value("weight").toDouble(100.0);

            if (name.isEmpty() || type.isEmpty() || weight <= 0.0) continue;
            objects << QString("  {\"type\":\"%1\",\"name\":\"%2\",\"weight\":%3}")
                .arg(jsonEscape(type))
                .arg(jsonEscape(name))
                .arg(weight, 0, 'f', 1);
        }

        if (objects.isEmpty()) {
            if (errorMessage) *errorMessage = "DeepSeek 返回的食物字段不完整";
            return QString();
        }

        return "[\n" + objects.join(",\n") + "\n]";
    }

    QString buildJsonFromSentenceByAi(const QString& sentence, QString* errorMessage) {
        if (sentence.trimmed().isEmpty()) {
            return QString();
        }
        if (apiKey().isEmpty()) {
            return QString();
        }

        const QString systemPrompt =
            "你是饮食记录解析助手。请只返回 JSON 对象，不要解释。"
            "格式必须是 {\"foods\":[{\"type\":\"主食|肉类|蔬菜|水果\",\"name\":\"食物名\",\"weight\":克数}]}。"
            "如果用户没有说明重量，weight 填 100。"
            "type 只能是 主食、肉类、蔬菜、水果 之一。";

        QString apiError;
        const QString content = callDeepSeek(systemPrompt, sentence, true, &apiError);
        if (content.isEmpty()) {
            if (errorMessage) *errorMessage = "DeepSeek 解析语句失败：" + apiError;
            return QString();
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            if (errorMessage) *errorMessage = "DeepSeek 解析结果格式错误：" + parseError.errorString();
            return QString();
        }

        return buildJsonArrayFromFoodsObject(doc.object(), errorMessage);
    }

    bool writeAiFoodToCsv(const QString& content,
                          const QString& foodName,
                          const QString& requestedType,
                          QString* errorMessage) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            if (errorMessage) *errorMessage = "DeepSeek 返回的营养数据格式错误：" + parseError.errorString();
            return false;
        }

        QJsonObject food = doc.object();
        const QString type = food.value("type").toString(requestedType).trimmed();
        const QString typeForCsv = (type == "主食" || type == "肉类" || type == "蔬菜" || type == "水果")
            ? type
            : requestedType;

        QFile file(fromUtf8String(nutritionDbPath.empty() ? "nutrition_db.csv" : nutritionDbPath));
        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            if (errorMessage) *errorMessage = "无法写入营养数据库 CSV";
            return false;
        }

        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#endif
        out << foodName << "," << typeForCsv << ","
            << numberText(food, "calorie") << ","
            << numberText(food, "protein") << ","
            << numberText(food, "fat") << ","
            << numberText(food, "carb") << ","
            << numberText(food, "fiber") << ","
            << numberText(food, "sugar") << ","
            << numberText(food, "cholesterol") << ","
            << numberText(food, "vitaminC") << ","
            << numberText(food, "sodium") << "\n";

        return true;
    }

    void appendFoodToCsvByAiAsync(const QString& foodName,
                                  const QString& requestedType,
                                  std::function<void(bool, const QString&)> callback) {
        const QString systemPrompt =
            "你是营养数据库助手。请只返回 JSON 对象，不要解释。"
            "营养值必须是每100克可食部分的估算值。"
            "字段必须包含 name,type,calorie,protein,fat,carb,fiber,sugar,cholesterol,vitaminC,sodium。"
            "type 只能是 主食、肉类、蔬菜、水果 之一。"
            "单位：calorie 为 kcal，protein/fat/carb/fiber/sugar 为 g，cholesterol/vitaminC/sodium 为 mg。";

        const QString userPrompt = QString(
            "请查询并估算食物“%1”的营养数据。用户选择的分类是“%2”。"
            "如果用户分类明显合理，请优先使用该分类。"
        ).arg(foodName, requestedType);

        callDeepSeekAsync(systemPrompt, userPrompt, true, [this, foodName, requestedType, callback](const QString& content, const QString& error) {
            if (!error.isEmpty()) {
                callback(false, "无法查询未知食物“" + foodName + "”：" + error);
                return;
            }

            QString writeError;
            if (!writeAiFoodToCsv(content, foodName, requestedType, &writeError)) {
                callback(false, writeError);
                return;
            }

            callback(true, QString());
        });
    }

    void ensureFoodsInDatabaseAsync(const QString& jsonText,
                                    std::function<void(bool, const QString&)> callback) {
        QString parseError;
        const QJsonArray foods = foodsArrayFromJsonText(jsonText, &parseError);
        if (!parseError.isEmpty()) {
            callback(false, parseError);
            return;
        }

        QVector<CatalogFood> missingFoods;
        for (const QJsonValue& value : foods) {
            const QJsonObject food = value.toObject();
            const QString name = food.value("name").toString().trimmed();
            const QString type = food.value("type").toString("主食").trimmed();
            if (name.isEmpty()) continue;

            if (!foodExistsInCatalog(name)) {
                bool alreadyQueued = false;
                for (const CatalogFood& item : missingFoods) {
                    if (item.name == name) {
                        alreadyQueued = true;
                        break;
                    }
                }
                if (!alreadyQueued) {
                    missingFoods.push_back({name, type});
                }
            }
        }

        if (missingFoods.isEmpty()) {
            callback(true, QString());
            return;
        }

        auto index = std::make_shared<int>(0);
        auto step = std::make_shared<std::function<void()>>();

        *step = [this, missingFoods, index, step, callback]() {
            if (*index >= missingFoods.size()) {
                system.init(nutritionDbPath.empty() ? "nutrition_db.csv" : nutritionDbPath);
                callback(true, QString());
                return;
            }

            const CatalogFood food = missingFoods[*index];
            setStatus(QString("正在查询未知食物：%1").arg(food.name));

            appendFoodToCsvByAiAsync(food.name, food.type, [index, step, callback](bool ok, const QString& error) {
                if (!ok) {
                    callback(false, error);
                    return;
                }

                (*index)++;
                (*step)();
            });
        };

        (*step)();
    }

    void addFoodsJsonAsync(const QString& json) {
        if (apiBusy) {
            setStatus("DeepSeek 正在处理中，请稍候");
            return;
        }

        setApiBusy(true, "正在检查食物营养库");
        ensureFoodsInDatabaseAsync(json, [this, json](bool ok, const QString& error) {
            if (!ok) {
                setApiBusy(false, error);
                return;
            }

            setStatus(fromUtf8String(system.addFoodsFromAiJson(toUtf8String(json))));
            updateView();
            setApiBusy(false);
        });
    }

    void addFoodsFromSentenceAsync(const QString& sentence) {
        if (apiBusy) {
            setStatus("DeepSeek 正在处理中，请稍候");
            return;
        }

        if (sentence.trimmed().isEmpty()) {
            setStatus("请输入饮食语句");
            return;
        }

        if (apiKey().isEmpty()) {
            const QString json = buildJsonFromSentence(sentence);
            if (json.isEmpty()) {
                setStatus("没有从语句中识别到营养库里的食物名称");
                return;
            }
            addFoodsJsonAsync(json);
            return;
        }

        const QString systemPrompt =
            "你是饮食记录解析助手。请只返回 JSON 对象，不要解释。"
            "格式必须是 {\"foods\":[{\"type\":\"主食|肉类|蔬菜|水果\",\"name\":\"食物名\",\"weight\":克数}]}。"
            "如果用户没有说明重量，weight 填 100。"
            "type 只能是 主食、肉类、蔬菜、水果 之一。";

        setApiBusy(true, "正在用 DeepSeek 解析语句");
        callDeepSeekAsync(systemPrompt, sentence, true, [this](const QString& content, const QString& error) {
            if (!error.isEmpty()) {
                setApiBusy(false, "DeepSeek 解析语句失败：" + error);
                return;
            }

            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                setApiBusy(false, "DeepSeek 解析结果格式错误：" + parseError.errorString());
                return;
            }

            QString buildError;
            const QString json = buildJsonArrayFromFoodsObject(doc.object(), &buildError);
            if (json.isEmpty()) {
                setApiBusy(false, buildError);
                return;
            }

            setStatus("正在检查食物营养库");
            ensureFoodsInDatabaseAsync(json, [this, json](bool ok, const QString& ensureError) {
                if (!ok) {
                    setApiBusy(false, ensureError);
                    return;
                }

                setStatus(fromUtf8String(system.addFoodsFromAiJson(toUtf8String(json))));
                updateView();
                setApiBusy(false);
            });
        });
    }

    QGroupBox* createTableBox() {
        auto* box = new QGroupBox("当前食物", this);
        auto* layout = new QVBoxLayout(box);

        foodTable = new QTableWidget(box);
        foodTable->setColumnCount(7);
        foodTable->setHorizontalHeaderLabels({
            "类型", "名称", "重量(g)", "热量(kcal)", "蛋白质(g)", "脂肪(g)", "碳水(g)"
        });
        foodTable->setAlternatingRowColors(true);
        foodTable->setShowGrid(false);
        foodTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        foodTable->horizontalHeader()->setMinimumHeight(34);
        foodTable->verticalHeader()->setDefaultSectionSize(34);
        foodTable->verticalHeader()->setVisible(false);
        foodTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        foodTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        layout->addWidget(foodTable);
        return box;
    }

    QGroupBox* createTotalBox() {
        auto* box = new QGroupBox("总营养摄入", this);
        auto* layout = new QGridLayout(box);

        calorieLabel = addMetric(layout, 0, 0, "热量");
        proteinLabel = addMetric(layout, 0, 1, "蛋白质");
        fatLabel = addMetric(layout, 0, 2, "脂肪");
        carbLabel = addMetric(layout, 1, 0, "碳水");
        fiberLabel = addMetric(layout, 1, 1, "膳食纤维");
        sugarLabel = addMetric(layout, 1, 2, "糖");
        cholesterolLabel = addMetric(layout, 2, 0, "胆固醇");
        vitaminCLabel = addMetric(layout, 2, 1, "维生素C");
        sodiumLabel = addMetric(layout, 2, 2, "钠");

        return box;
    }

    QLabel* addMetric(QGridLayout* layout, int row, int column, const QString& name) {
        auto* nameLabel = new QLabel(name, this);
        auto* valueLabel = new QLabel("0.0", this);
        layout->addWidget(nameLabel, row, column * 2);
        layout->addWidget(valueLabel, row, column * 2 + 1);
        return valueLabel;
    }

    QString formatTotalNutrition(const QJsonObject& total) const {
        return QString("合计：热量 %1 kcal，蛋白质 %2 g，脂肪 %3 g，碳水 %4 g，膳食纤维 %5 g，糖 %6 g，胆固醇 %7 mg，维生素C %8 mg，钠 %9 mg")
            .arg(total.value("calorie").toDouble(), 0, 'f', 1)
            .arg(total.value("protein").toDouble(), 0, 'f', 1)
            .arg(total.value("fat").toDouble(), 0, 'f', 1)
            .arg(total.value("carb").toDouble(), 0, 'f', 1)
            .arg(total.value("fiber").toDouble(), 0, 'f', 1)
            .arg(total.value("sugar").toDouble(), 0, 'f', 1)
            .arg(total.value("cholesterol").toDouble(), 0, 'f', 1)
            .arg(total.value("vitaminC").toDouble(), 0, 'f', 1)
            .arg(total.value("sodium").toDouble(), 0, 'f', 1);
    }

    QString formatHistoryLine(const QString& line, int index) const {
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            return QString("第 %1 条记录解析失败：%2\n原始内容：%3\n")
                .arg(index)
                .arg(error.errorString())
                .arg(line);
        }

        const QJsonObject record = doc.object();
        QStringList foodTexts;
        const QJsonArray foods = record.value("foods").toArray();

        for (const QJsonValue& value : foods) {
            const QJsonObject food = value.toObject();
            foodTexts << QString("%1 %2 %3g")
                .arg(food.value("type").toString())
                .arg(food.value("name").toString())
                .arg(food.value("weight").toDouble(), 0, 'f', 1);
        }

        QString text;
        text += QString("第 %1 条\n").arg(index);
        text += "时间：" + record.value("time").toString() + "\n";
        text += "食物：" + (foodTexts.isEmpty() ? QString("无") : foodTexts.join("、")) + "\n";
        text += formatTotalNutrition(record.value("total").toObject()) + "\n";
        return text;
    }

    QString loadHistoryText() const {
        QString filename = fromUtf8String(RecordManager::filename(system.getCurrentUsername()));
        QFile file(filename);
        if (!file.exists()) {
            return QString();
        }

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return "无法打开历史记录文件：" + filename;
        }

        QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec("UTF-8");
#endif

        QString text;
        int index = 1;
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;

            if (!text.isEmpty()) text += "\n";
            text += formatHistoryLine(line, index);
            index++;
        }

        return text;
    }

    void showHistoryRecords() {
        if (!system.isLoggedIn()) {
            setStatus("请先登录");
            return;
        }

        const QString historyText = loadHistoryText();
        if (historyText.isEmpty()) {
            setStatus("当前用户还没有历史记录");
            return;
        }

        auto* dialog = new QDialog(this);
        dialog->setWindowTitle("历史饮食记录");
        dialog->resize(760, 520);

        auto* layout = new QVBoxLayout(dialog);
        auto* viewer = new QPlainTextEdit(dialog);
        viewer->setReadOnly(true);
        viewer->setPlainText(historyText);

        auto* closeButton = new QPushButton("关闭", dialog);
        auto* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);

        layout->addWidget(viewer);
        layout->addLayout(buttonLayout);

        connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
        dialog->exec();
    }

    QString currentUserProfileText() const {
        if (!system.isLoggedIn()) {
            return QString();
        }

        const int age = system.getCurrentUserAge();
        const QString gender = fromUtf8String(system.getCurrentUserGender());
        const double heightCm = system.getCurrentUserHeightCm();
        const double weightKg = system.getCurrentUserWeightKg();
        const QString activity = fromUtf8String(system.getCurrentUserActivityLevel());
        const QString goal = fromUtf8String(system.getCurrentUserHealthGoal());
        const double heightM = heightCm / 100.0;
        const double bmi = (heightM > 0.0 && weightKg > 0.0) ? weightKg / (heightM * heightM) : 0.0;

        return QString("用户资料：年龄 %1，性别 %2，身高 %3 cm，体重 %4 kg，BMI %5，活动水平 %6，健康目标 %7")
            .arg(age > 0 ? QString::number(age) : "未填写")
            .arg(gender.isEmpty() ? "未填写" : gender)
            .arg(heightCm > 0.0 ? QString::number(heightCm, 'f', 1) : "未填写")
            .arg(weightKg > 0.0 ? QString::number(weightKg, 'f', 1) : "未填写")
            .arg(bmi > 0.0 ? QString::number(bmi, 'f', 1) : "未填写")
            .arg(activity.isEmpty() ? "未填写" : activity)
            .arg(goal.isEmpty() ? "未填写" : goal);
    }

    QString buildCurrentDietText() const {
        const auto& foods = system.getFoods();
        if (foods.empty()) {
            return QString();
        }

        QStringList lines;
        lines << "当前饮食数据：";
        for (const auto& food : foods) {
            const NutrientData actual = food->getActualIntake();
            lines << QString("%1 %2 %3g：热量 %4 kcal，蛋白质 %5 g，脂肪 %6 g，碳水 %7 g")
                .arg(fromUtf8String(food->getType()))
                .arg(fromUtf8String(food->getName()))
                .arg(food->getWeight(), 0, 'f', 1)
                .arg(actual.calorie, 0, 'f', 1)
                .arg(actual.protein, 0, 'f', 1)
                .arg(actual.fat, 0, 'f', 1)
                .arg(actual.carb, 0, 'f', 1);
        }

        const NutrientData total = system.getTotalNutrition();
        lines << QString("总计：热量 %1 kcal，蛋白质 %2 g，脂肪 %3 g，碳水 %4 g，膳食纤维 %5 g，糖 %6 g，胆固醇 %7 mg，维生素C %8 mg，钠 %9 mg")
            .arg(total.calorie, 0, 'f', 1)
            .arg(total.protein, 0, 'f', 1)
            .arg(total.fat, 0, 'f', 1)
            .arg(total.carb, 0, 'f', 1)
            .arg(total.fiber, 0, 'f', 1)
            .arg(total.sugar, 0, 'f', 1)
            .arg(total.cholesterol, 0, 'f', 1)
            .arg(total.vitaminC, 0, 'f', 1)
            .arg(total.sodium, 0, 'f', 1);

        return lines.join("\n");
    }

    void showTextDialog(const QString& title, const QString& text) {
        auto* dialog = new QDialog(this);
        dialog->setWindowTitle(title);
        dialog->resize(760, 560);

        auto* layout = new QVBoxLayout(dialog);
        auto* viewer = new QPlainTextEdit(dialog);
        viewer->setReadOnly(true);
        viewer->setPlainText(text);

        auto* closeButton = new QPushButton("关闭", dialog);
        auto* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);

        layout->addWidget(viewer);
        layout->addLayout(buttonLayout);

        connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
        dialog->exec();
    }

    void generateReport(const QString& title, const QString& dietData) {
        if (apiBusy) {
            setStatus("DeepSeek 正在处理中，请稍候");
            return;
        }

        if (dietData.trimmed().isEmpty()) {
            setStatus("没有可生成报告的饮食数据");
            return;
        }

        const QString systemPrompt =
            "你是专业营养分析助手。请根据用户饮食数据生成中文报告。"
            "报告包含：总体评价、主要营养问题、可执行建议、下一餐建议。"
            "语气客观友好，不要声称可以替代医生诊断。";

        const QString userPrompt =
            "请结合用户年龄、性别、身高、体重、BMI、活动水平和健康目标，给出更合适的饮食建议。"
            "如果资料未填写，请说明建议仅按一般成年人参考。\n\n"
            + currentUserProfileText() + "\n\n"
            "请根据以下饮食数据生成膳食分析报告：\n\n" + dietData;

        setApiBusy(true, "正在生成报告");
        callDeepSeekAsync(systemPrompt, userPrompt, false, [this, title](const QString& report, const QString& error) {
            if (!error.isEmpty()) {
                setApiBusy(false, "报告生成失败：" + error);
                return;
            }

            showTextDialog(title, report);
            setApiBusy(false, "报告生成成功");
        });
    }

    void generateCurrentReport() {
        if (!system.isLoggedIn()) {
            setStatus("请先登录");
            return;
        }

        generateReport("当前饮食报告", buildCurrentDietText());
    }

    void generateHistoryReport() {
        if (!system.isLoggedIn()) {
            setStatus("请先登录");
            return;
        }

        const QString historyText = loadHistoryText();
        if (historyText.isEmpty()) {
            setStatus("当前用户还没有历史记录");
            return;
        }

        generateReport("历史饮食报告", historyText);
    }

    void updateView() {
        if (system.isLoggedIn()) {
            const int age = system.getCurrentUserAge();
            const QString gender = fromUtf8String(system.getCurrentUserGender());
            const double heightCm = system.getCurrentUserHeightCm();
            const double weightKg = system.getCurrentUserWeightKg();
            const QString activity = fromUtf8String(system.getCurrentUserActivityLevel());
            const QString goal = fromUtf8String(system.getCurrentUserHealthGoal());
            const double heightM = heightCm / 100.0;
            const double bmi = (heightM > 0.0 && weightKg > 0.0) ? weightKg / (heightM * heightM) : 0.0;
            currentUserLabel->setText(QString("当前用户：%1    年龄：%2    性别：%3    身高：%4 cm    体重：%5 kg    BMI：%6    活动：%7    目标：%8")
                .arg(fromUtf8String(system.getCurrentUsername()))
                .arg(age > 0 ? QString::number(age) : "未填写")
                .arg(gender.isEmpty() ? "未填写" : gender)
                .arg(heightCm > 0.0 ? QString::number(heightCm, 'f', 1) : "未填写")
                .arg(weightKg > 0.0 ? QString::number(weightKg, 'f', 1) : "未填写")
                .arg(bmi > 0.0 ? QString::number(bmi, 'f', 1) : "未填写")
                .arg(activity.isEmpty() ? "未填写" : activity)
                .arg(goal.isEmpty() ? "未填写" : goal));

            if (age > 0 && ageSpin) {
                ageSpin->setValue(age);
            }
            if (heightCm > 0.0 && heightSpin) {
                heightSpin->setValue(heightCm);
            }
            if (weightKg > 0.0 && bodyWeightSpin) {
                bodyWeightSpin->setValue(weightKg);
            }
            if (!gender.isEmpty() && genderCombo) {
                const int index = genderCombo->findText(gender);
                if (index >= 0) {
                    genderCombo->setCurrentIndex(index);
                }
            }
            if (!activity.isEmpty() && activityCombo) {
                const int index = activityCombo->findText(activity);
                if (index >= 0) {
                    activityCombo->setCurrentIndex(index);
                }
            }
            if (!goal.isEmpty() && goalCombo) {
                const int index = goalCombo->findText(goal);
                if (index >= 0) {
                    goalCombo->setCurrentIndex(index);
                }
            }
        } else {
            currentUserLabel->setText("当前用户：未登录");
        }

        const auto& foods = system.getFoods();
        foodTable->setRowCount(static_cast<int>(foods.size()));

        for (int row = 0; row < static_cast<int>(foods.size()); row++) {
            const auto& food = foods[static_cast<size_t>(row)];
            const NutrientData actual = food->getActualIntake();

            setTableText(row, 0, fromUtf8String(food->getType()));
            setTableText(row, 1, fromUtf8String(food->getName()));
            setTableText(row, 2, QString::number(food->getWeight(), 'f', 1));
            setTableText(row, 3, QString::number(actual.calorie, 'f', 1));
            setTableText(row, 4, QString::number(actual.protein, 'f', 1));
            setTableText(row, 5, QString::number(actual.fat, 'f', 1));
            setTableText(row, 6, QString::number(actual.carb, 'f', 1));
        }

        const NutrientData total = system.getTotalNutrition();
        calorieLabel->setText(QString::number(total.calorie, 'f', 1) + " kcal");
        proteinLabel->setText(QString::number(total.protein, 'f', 1) + " g");
        fatLabel->setText(QString::number(total.fat, 'f', 1) + " g");
        carbLabel->setText(QString::number(total.carb, 'f', 1) + " g");
        fiberLabel->setText(QString::number(total.fiber, 'f', 1) + " g");
        sugarLabel->setText(QString::number(total.sugar, 'f', 1) + " g");
        cholesterolLabel->setText(QString::number(total.cholesterol, 'f', 1) + " mg");
        vitaminCLabel->setText(QString::number(total.vitaminC, 'f', 1) + " mg");
        sodiumLabel->setText(QString::number(total.sodium, 'f', 1) + " mg");
    }

    void setTableText(int row, int column, const QString& text) {
        foodTable->setItem(row, column, new QTableWidgetItem(text));
    }

    void setStatus(const QString& text) {
        statusLabel->setText("状态：" + text);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

#ifdef NUTRITION_DB_PATH
    QFileInfo dbFile(QString::fromUtf8(NUTRITION_DB_PATH));
    if (dbFile.exists()) {
        QDir::setCurrent(dbFile.absolutePath());
    }
#endif

    DietWindow window;
    window.show();

    return app.exec();
}
