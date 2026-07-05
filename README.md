每日膳食营养统计 - 源码包
项目简介
本项目是基于 Qt Widgets 的每日膳食营养统计程序，支持用户注册登录、个人资料维护、食物录入、营养统计、历史记录查看，以及通过 DeepSeek API 解析自然语言饮食描述并生成膳食分析报告。

主要功能
用户注册、登录、退出登录
年龄、性别、身高、体重、活动水平、目标等资料维护
BMI 与用户信息展示
手动添加食物并统计营养
自然语言输入饮食内容，AI 解析后批量添加
未知食物通过 AI 查询营养并写入本地 CSV 食物库
保存每日饮食记录
查看历史饮食记录
生成当日饮食报告和历史饮食报告
异步网络请求，避免界面卡顿
文件说明
main.cpp：Qt 主界面与交互逻辑
bighomework.pro：Qt 工程文件
food.h、food_factory.h：食物类与工厂类
nutrition_db.h、nutrition_db.csv：营养数据库读取与食物营养数据
user_manager.h：用户注册、登录和资料存储
record_manager.h：饮食记录保存与读取
diet_manager.h、diet_system.h：饮食统计核心逻辑
ai_parser.h、deepseek_config.h：AI 解析与 DeepSeek 配置
编译运行
使用 Qt Creator 打开 bighomework.pro。
选择 Qt Widgets 可用的编译套件。
如需使用 AI 解析和报告功能，在 deepseek_config.h 中填写 DeepSeek API Key。
构建并运行项目。
如果只发送可执行程序，请同时把 nutrition_db.csv 放在 exe 同级目录，否则营养数据库无法加载。

说明

演示视频：https://disk.pku.edu.cn/link/AA6C907185E2E5433A8E110F6888A9EDA0
提取码：pS88

源码包已排除编译产物、Qt 临时文件、个人用户数据和饮食记录数据。
