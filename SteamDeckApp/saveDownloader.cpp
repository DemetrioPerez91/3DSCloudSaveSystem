#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <curl/curl.h>

namespace fs = std::filesystem;

const std::string SERVER_PRO_URL = "http://10.0.0.106:3000/gbaSaveFiles";
const std::string SERVER_STG_URL = "http://10.0.0.32:3000/gbaSaveFiles";

// Helper to store response in a string
size_t writeToString(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Helper to write binary data to a file
size_t writeToFile(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::ofstream* outFile = static_cast<std::ofstream*>(stream);
    outFile->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// Very simple JSON parsing just to get filenames
std::vector<std::string> extractFilenames(const std::string& json) {
    std::vector<std::string> filenames;
    std::string key = "\"filename\":\"";
    size_t pos = 0;

    while ((pos = json.find(key, pos)) != std::string::npos) {
        pos += key.length();
        size_t end = json.find("\"", pos);
        if (end != std::string::npos) {
            filenames.push_back(json.substr(pos, end - pos));
            pos = end;
        }
    }

    return filenames;
}

// Encode file name
std::string urlEncode(const std::string& value) {
    std::ostringstream encoded;
    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return encoded.str();
}

// Download one file
void downloadFile(const std::string& filename) {
    std::string encodedFileName = urlEncode(filename);
    std::string fileUrl = SERVER_STG_URL + "/" + encodedFileName;
    
    std::ofstream outFile(filename, std::ios::binary);
    std::cout << "Opening: " << filename << std::endl;
    if (!outFile) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }


    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, fileUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
        CURLcode res = curl_easy_perform(curl);
        
      std::cout << "Downloading: " << fileUrl << std::endl;
        if (res != CURLE_OK) {
            std::cerr << "Download failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Downloaded: " << filename << std::endl;
        }
        curl_easy_cleanup(curl);
    }

    outFile.close();
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    std::string jsonResponse;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, SERVER_STG_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed to get file list: " << curl_easy_strerror(res) << std::endl;
            return 1;
        }

        curl_easy_cleanup(curl);
    }

    std::vector<std::string> files = extractFilenames(jsonResponse);
    for (const auto& file : files) {
        downloadFile(file);
    }

    curl_global_cleanup();
    return 0;
}

