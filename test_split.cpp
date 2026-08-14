#include <iostream>
#include <string>
#include <vector>
#include <sstream>

int main() {
    std::string objectsString = "1,1,2,2;1,31,2,3,101,1;1,4,2,5;";
    std::vector<std::string> parts;
    std::stringstream ss(objectsString);
    std::string item;
    while (std::getline(ss, item, ';')) {
        if (!item.empty()) parts.push_back(item);
    }
    
    for (const auto& part : parts) {
        std::cout << part << std::endl;
    }
    return 0;
}
