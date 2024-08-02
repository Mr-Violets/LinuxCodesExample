#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <opencv2/opencv.hpp>
#include <condition_variable>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

std::queue<std::vector<cv::Mat>> frameQueue; // 修改为队列存储多帧图像
std::mutex queueMutex;
std::condition_variable dataCondition;
bool keepRunning = true;

namespace fs = std::filesystem;

std::string getCurrentDateTimeString() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&now_time_t, &tm);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d/%H-%M-%S");

    return ss.str();
}

void saveFrames(const std::vector<cv::Mat>& frames, const std::string& currentDateTime) {
    std::string baseDir = "."; // 假设程序当前目录为基础目录
    std::string carNumber = "0001";
    std::string cameraName = "camera01";

    std::string folderPath = baseDir + "/dataCapture/Car" + carNumber + "/" + currentDateTime + "/" + cameraName;
    fs::create_directories(folderPath);

    for (size_t i = 0; i < frames.size(); ++i) {
        std::string filename = folderPath + "/frame" + std::to_string(i) + ".jpg";
        cv::imwrite(filename, frames[i]);
        std::cout << "Saved " << filename << std::endl;
    }
}

void captureFrames() {
    cv::VideoCapture cap(0); // 打开默认摄像头
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera." << std::endl;
        return;
    }

    while (keepRunning) {
        std::vector<cv::Mat> frames;

        // 获取4帧图像
        for (int i = 0; i < 4; ++i) {
            cv::Mat frame;
            cap >> frame; // 从摄像头获取一帧

            if (frame.empty()) {
                std::cerr << "Error: Captured empty frame." << std::endl;
                continue;
            }

            frames.push_back(frame);
        }

        // 获取当前日期时间字符串
        std::string currentDateTime = getCurrentDateTimeString();

        // 保存这4帧图像
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            frameQueue.push(frames);
            dataCondition.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒获取4帧
    }
}

void saveFramesWorker() {
    while (keepRunning) {
        std::vector<cv::Mat> frames;
        std::string currentDateTime;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            dataCondition.wait(lock, [] { return !frameQueue.empty() || !keepRunning; });

            if (!frameQueue.empty()) {
                frames = std::move(frameQueue.front());
                frameQueue.pop();
                currentDateTime = getCurrentDateTimeString();
            }
        }

        if (!frames.empty()) {
            saveFrames(frames, currentDateTime);
        }
    }
}

void sendFrame() {
    // 暂时不做处理
}

int main() {
    std::thread captureThread(captureFrames);
    std::thread saveThread(saveFramesWorker);
    std::thread sendThread(sendFrame);

    std::this_thread::sleep_for(std::chrono::seconds(10)); // 运行10秒后结束

    keepRunning = false;
    dataCondition.notify_all();

    captureThread.join();
    saveThread.join();
    sendThread.join();

    return 0;
}
