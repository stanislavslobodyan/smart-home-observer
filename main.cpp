#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#pragma execution_character_set("utf-8")
#endif

struct ClimateSnapshot {
    double temperatureC = 0.0;
    double humidityPercent = 0.0;
    unsigned eventNumber = 0;
};

// Observer: усі смарт-пристрої отримують однаковий знімок клімату.
class ISmartDevice {
public:
    virtual ~ISmartDevice() = default;
    virtual std::string_view Name() const = 0;
    virtual void Update(const ClimateSnapshot& snapshot) = 0;
};

// Subject: центральний хаб керує підписками та сповіщеннями.
class IClimateSubject {
public:
    virtual ~IClimateSubject() = default;
    virtual bool Subscribe(ISmartDevice* device) = 0;
    virtual bool Unsubscribe(ISmartDevice* device) = 0;
    virtual void Notify() = 0;
};

class ClimateHub final : public IClimateSubject {
public:
    bool Subscribe(ISmartDevice* device) override {
        if (device == nullptr) {
            return false;
        }

        const auto found = std::find(observers_.begin(), observers_.end(), device);
        if (found != observers_.end()) {
            std::cout << "[Хаб] " << device->Name()
                      << " уже підписаний. Повторну підписку пропущено.\n";
            return false;
        }

        observers_.push_back(device);
        std::cout << "[Хаб] Підключено: " << device->Name()
                  << ". Підписників: " << observers_.size() << "\n";
        return true;
    }

    bool Unsubscribe(ISmartDevice* device) override {
        const auto found = std::find(observers_.begin(), observers_.end(), device);
        if (found == observers_.end()) {
            return false;
        }

        std::cout << "[Хаб] Відключено: " << (*found)->Name() << "\n";
        observers_.erase(found);
        return true;
    }

    void SetClimate(double temperatureC, double humidityPercent) {
        if (temperatureC < -80.0 || temperatureC > 80.0) {
            throw std::out_of_range("Температура повинна бути в межах від -80 до 80 C.");
        }
        if (humidityPercent < 0.0 || humidityPercent > 100.0) {
            throw std::out_of_range("Вологість повинна бути в межах від 0 до 100 %.");
        }

        snapshot_ = {temperatureC, humidityPercent, snapshot_.eventNumber + 1};

        std::cout << "\n=== Подія " << snapshot_.eventNumber << " ===\n"
                  << "Клімат: " << std::fixed << std::setprecision(1)
                  << snapshot_.temperatureC << " C, "
                  << snapshot_.humidityPercent << " %\n"
                  << "Одержувачів сповіщення: " << observers_.size() << "\n";

        Notify();
    }

    void Notify() override {
        // Копія списку робить обхід стабільним, навіть якщо підписки зміняться
        // під час обробки поточної події.
        const auto currentObservers = observers_;
        for (ISmartDevice* device : currentObservers) {
            device->Update(snapshot_);
        }
    }

private:
    std::vector<ISmartDevice*> observers_;
    ClimateSnapshot snapshot_;
};

const char* StateLabel(bool enabled) {
    return enabled ? "ON" : "OFF";
}
void PrintReaction(std::string_view deviceName,
                   bool enabled,
                   std::string_view reason) {
    std::cout << "  - " << deviceName << ": " << StateLabel(enabled)
              << " (" << reason << ")\n";
}
class Heater final : public ISmartDevice {
public:
    std::string_view Name() const override {
        return "Обігрівач";
    }

    void Update(const ClimateSnapshot& snapshot) override {
        enabled_ = snapshot.temperatureC < 18.0;
        PrintReaction(Name(), enabled_, enabled_
            ? "температура нижча за 18 C"
            : "нагрів не потрібний");
    }

private:
    bool enabled_ = false;
};

class AirConditioner final : public ISmartDevice {
public:
    std::string_view Name() const override {
        return "Кондиціонер";
    }

    void Update(const ClimateSnapshot& snapshot) override {
        enabled_ = snapshot.temperatureC > 26.0;
        PrintReaction(Name(), enabled_, enabled_
            ? "температура вища за 26 C"
            : "охолодження не потрібне");
    }

private:
    bool enabled_ = false;
};

class Humidifier final : public ISmartDevice {
public:
    std::string_view Name() const override {
        return "Зволожувач";
    }

    void Update(const ClimateSnapshot& snapshot) override {
        enabled_ = snapshot.humidityPercent < 35.0;
        PrintReaction(Name(), enabled_, enabled_
            ? "вологість нижча за 35 %"
            : "зволоження не потрібне");
    }

private:
    bool enabled_ = false;
};

class Ventilation final : public ISmartDevice {
public:
    std::string_view Name() const override {
        return "Вентиляція";
    }

    void Update(const ClimateSnapshot& snapshot) override {
        enabled_ = snapshot.humidityPercent > 70.0;
        PrintReaction(Name(), enabled_, enabled_
            ? "вологість вища за 70 %"
            : "посилений повітрообмін не потрібний");
    }

private:
    bool enabled_ = false;
};

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try {
        ClimateHub hub;
        Heater heater;
        AirConditioner airConditioner;
        Humidifier humidifier;
        Ventilation ventilation;

        std::cout << "Демонстрація патерна Спостерігач у системі Розумний дім\n\n";

        // Крок 1. Перші два пристрої підписуються на хаб.
        hub.Subscribe(&heater);
        hub.Subscribe(&humidifier);
        hub.Subscribe(&humidifier);  // Хаб відхилить повторну підписку.
        hub.SetClimate(17.0, 30.0);

        // Крок 2. Кондиціонер підключається під час виконання програми.
        std::cout << "\n--- Підключаємо кондиціонер ---\n";
        hub.Subscribe(&airConditioner);
        hub.SetClimate(29.0, 50.0);

        // Крок 3. Обігрівач відключається і більше не отримує Update.
        std::cout << "\n--- Відключаємо обігрівач ---\n";
        hub.Unsubscribe(&heater);
        hub.SetClimate(16.0, 40.0);

        // Крок 4. Додаємо вентиляцію і перевіряємо високу вологість.
        std::cout << "\n--- Підключаємо вентиляцію ---\n";
        hub.Subscribe(&ventilation);
        hub.SetClimate(23.0, 75.0);

        std::cout << "\nДемонстрацію завершено.\n";
    } catch (const std::exception& error) {
        std::cerr << "Помилка: " << error.what() << "\n";
        return 1;
    }

    return 0;
}

