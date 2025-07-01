#include <iostream>
#include <filesystem>
#include <curl/curl.h>
#include <string>

namespace fs = std::filesystem;

const std::string SERVER_PRO_URL = "http://10.0.0.106:3000/gbaSaveFiles";
const std::string SERVER_STG_URL = "http://10.0.0.106:3000/gbaSaveFiles";

void uploadFile(const fs::path& filePath) {
    CURL* curl;
    CURLcode res;
    curl_mime *form = nullptr;
    curl_mimepart *field = nullptr;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        form = curl_mime_init(curl);
        field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, filePath.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, SERVER_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            std::cerr << "Upload failed: " << curl_easy_strerror(res) << std::endl;
        else
            std::cout << "Uploaded: " << filePath << std::endl;

        curl_mime_free(form);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

void scanAndUpload(const fs::path& rootPath) {
    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sav") {
            uploadFile(entry.path());
        }
    }
}

int main(int argc, char* argv[]) {
    fs::path pathToScan = (argc > 1) ? fs::path(argv[1]) : fs::current_path();
    scanAndUpload(pathToScan);
    return 0;
}

