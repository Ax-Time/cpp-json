#include "json.hpp"
#include <sstream>
#include <fstream>
#include <string_view>
#include <iostream>

constexpr std::string_view sampleJsonFile = "tests/sample.json";

template <typename T>
void assertEqual(const T& a, const T& b) {
    if (a != b) {
        std::ostringstream oss;
        oss << "Assertion failed: " << a << " != " << b;
        throw std::runtime_error(oss.str());
    }
    std::cout << "Assertion passed." << std::endl;
}

void get() {
    json::Json json = json::fromFile(sampleJsonFile.data());
    std::string name = json["name"].as<std::string>();
    std::string expectedName = "Jane";
    assertEqual(name, expectedName);

    int age = json["age"].as<int>();
    int expectedAge = 24;
    assertEqual(age, expectedAge);

    bool student = json["student"].as<bool>();
    bool expectedstudent = true;
    assertEqual(student, expectedstudent);

    auto friends = json["friends"];

    std::string friend0 = friends[0].as<std::string>();
    std::string expectedFriend0 = "Bob";
    assertEqual(friend0, expectedFriend0);

    auto friend1 = friends[1];
    std::string friend1Name = friend1["name"].as<std::string>();
    std::string expectedFriend1Name = "John";
    assertEqual(friend1Name, expectedFriend1Name);

    int friend1Age = friend1["age"].as<int>();
    int expectedFriend1Age = 25;
    assertEqual(friend1Age, expectedFriend1Age);

    bool friend1student = friend1["student"].as<bool>();
    bool expectedFriend1student = false;
    assertEqual(friend1student, expectedFriend1student);

    float friend1Money = friend1["money"].as<float>();
    float expectedFriend1Money = 100.34;
    assertEqual(friend1Money, expectedFriend1Money);

    auto address = json["address"];
    std::string street = address["street"].as<std::string>();
    std::string expectedStreet = "123 Main St";
    assertEqual(street, expectedStreet);

    std::string city = address["city"].as<std::string>();
    std::string expectedCity = "Springfield";
    assertEqual(city, expectedCity);

    int zip = address["zip"].as<int>();
    int expectedZip = 62701;
    assertEqual(zip, expectedZip);
}

int main() {
    get();
    return 0;
}