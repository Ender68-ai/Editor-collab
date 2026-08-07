#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::map<std::string, std::string> parseSaveString(const std::string& str) {
    std::map<std::string, std::string> result;
    auto tokens = split(str, ',');
    for (size_t i = 0; i + 1 < tokens.size(); i += 2) {
        result[tokens[i]] = tokens[i + 1];
    }
    return result;
}

int main() {
    std::string remoteSaveStr = "1,31,2,10,3,10,93,1,kA1,1";
    std::string localSaveStr = "1,31,2,10,3,10,kA1,1";

    auto remoteVec = parseSaveString(remoteSaveStr);
    auto localMap = parseSaveString(localSaveStr);

    std::cout << "Remote 93: " << remoteVec.count("93") << "\n";
    std::cout << "Local 93: " << localMap.count("93") << "\n";

    return 0;
}
