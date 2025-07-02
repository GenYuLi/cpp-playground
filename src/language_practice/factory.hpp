#include <iostream>
#include <memory>
#include <string>

// 1) Product 介面
class Logger {
public:
    virtual ~Logger() = default;
    virtual void log(const std::string& msg) = 0;
};

// 2) ConcreteProduct
class ConsoleLogger : public Logger {
public:
    void log(const std::string& msg) override {
        std::cout << "[Console] " << msg << "\n";
    }
};

class FileLogger : public Logger {
public:
    void log(const std::string& msg) override {
        // 假設寫檔：省略實作
        std::cout << "[File] " << msg << "\n";
    }
};

// 3) Creator 抽象工廠
class LoggerFactory {
public:
    virtual ~LoggerFactory() = default;
    // 工廠方法：回傳 abstract Logger
    virtual std::unique_ptr<Logger> create() const = 0;
};

// 4) ConcreteCreator
class ConsoleLoggerFactory : public LoggerFactory {
public:
    std::unique_ptr<Logger> create() const override {
        return std::make_unique<ConsoleLogger>();
    }
};

class FileLoggerFactory : public LoggerFactory {
public:
    std::unique_ptr<Logger> create() const override {
        return std::make_unique<FileLogger>();
    }
};

// 5) 客戶端程式，與具體 Logger 解耦
void runApp(const LoggerFactory& factory) {
    auto logger = factory.create();
    logger->log("Hello Factory!");
}


