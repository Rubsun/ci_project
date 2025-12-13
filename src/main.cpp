#include "memory_service.h"
#include "user_service.h"
#include "api_controller.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <regex>

namespace SimpleHttp {
    struct Request {
        std::string method;
        std::string path;
        std::string body;
        std::map<std::string, std::string> queryParams;
    };
    
    struct Response {
        int statusCode = 200;
        std::string body;
        std::map<std::string, std::string> headers;
        
        std::string toString() const {
            std::ostringstream oss;
            oss << "HTTP/1.1 " << statusCode << " OK\r\n";
            
            if (headers.count("Content-Type")) {
                oss << "Content-Type: " << headers.at("Content-Type") << "\r\n";
            } else {
                oss << "Content-Type: application/json\r\n";
            }
            oss << "Access-Control-Allow-Origin: *\r\n";
            oss << "Content-Length: " << body.length() << "\r\n";
            oss << "\r\n";
            oss << body;
            return oss.str();
        }
    };
    
    class Server {
    public:
        Server(int port) : port_(port), running_(false) {}
        
        void start(std::function<Response(const Request&)> handler) {
            handler_ = handler;
            running_ = true;
            
            int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket < 0) {
                std::cerr << "Error creating socket" << std::endl;
                return;
            }
            
            int opt = 1;
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port_);
            
            if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
                std::cerr << "Error binding socket" << std::endl;
                close(serverSocket);
                return;
            }
            
            if (listen(serverSocket, 10) < 0) {
                std::cerr << "Error listening" << std::endl;
                close(serverSocket);
                return;
            }
            
            std::cout << "Server started on port " << port_ << std::endl;
            
            while (running_) {
                sockaddr_in clientAddress{};
                socklen_t clientLen = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLen);
                
                if (clientSocket < 0) {
                    continue;
                }
                
                std::thread([this, clientSocket]() {
                    handleClient(clientSocket);
                }).detach();
            }
            
            close(serverSocket);
        }
        
        void stop() {
            running_ = false;
        }
        
    private:
        void handleClient(int clientSocket) {
            char buffer[4096] = {0};
            ssize_t bytesRead = read(clientSocket, buffer, 4095);
            if (bytesRead <= 0) {
                close(clientSocket);
                return;
            }
            buffer[bytesRead] = '\0';
            
            Request req = parseRequest(std::string(buffer));
            Response res = handler_(req);
            
            std::string responseStr = res.toString();
            send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
            
            close(clientSocket);
        }
        
        Request parseRequest(const std::string& raw) {
            Request req;
            std::istringstream iss(raw);
            std::string line;
            
            if (std::getline(iss, line)) {
                std::istringstream firstLine(line);
                firstLine >> req.method >> req.path;
            }
            
            size_t qPos = req.path.find('?');
            if (qPos != std::string::npos) {
                std::string query = req.path.substr(qPos + 1);
                req.path = req.path.substr(0, qPos);
                
                std::regex paramRegex("([^=&]+)=([^&]*)");
                std::sregex_iterator iter(query.begin(), query.end(), paramRegex);
                std::sregex_iterator end;
                
                for (; iter != end; ++iter) {
                    req.queryParams[iter->str(1)] = iter->str(2);
                }
            }
            
            bool bodyStart = false;
            while (std::getline(iss, line)) {
                if (line == "\r" || line.empty()) {
                    bodyStart = true;
                    continue;
                }
                if (bodyStart) {
                    req.body += line;
                }
            }
            
            return req;
        }
        
        int port_;
        bool running_;
        std::function<Response(const Request&)> handler_;
    };
}

int main() {
    using namespace MemoryTrainer;
    using namespace SimpleHttp;
    
    MemoryService service;
    UserService userService;
    ApiController controller(service, userService);
    
    Server server(8080);
    
    server.start([&controller](const Request& req) -> Response {
        Response res;

        if (req.method == "OPTIONS") {
            res.statusCode = 200;
            res.headers["Access-Control-Allow-Methods"] = "GET, POST, DELETE, OPTIONS";
            res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
            return res;
        }
        
        if (req.path == "/api/game" && req.method == "POST") {
            std::string type = req.queryParams.count("type") ? req.queryParams.at("type") : "sequence";
            std::string difficulty = req.queryParams.count("difficulty") ? req.queryParams.at("difficulty") : "medium";
            res.body = ApiControllerAccess::createGame(controller, type, difficulty, "");
        }
        else if (req.path.find("/api/game/") == 0 && req.method == "GET") {
            std::string gameId = req.path.substr(10); // "/api/game/".length()
            res.body = ApiControllerAccess::getGame(controller, gameId);
        }
        else if (req.path.find("/api/game/") == 0 && req.path.find("/flip") != std::string::npos && req.method == "POST") {
            size_t gameIdStart = 10;
            size_t gameIdEnd = req.path.find("/flip");
            std::string gameId = req.path.substr(gameIdStart, gameIdEnd - gameIdStart);
            
            std::regex cardRegex("\"cardId\"\\s*:\\s*(\\d+)");
            std::smatch match;
            int cardId = -1;
            if (std::regex_search(req.body, match, cardRegex)) {
                cardId = std::stoi(match[1].str());
            }
            
            if (cardId >= 0) {
                res.body = ApiControllerAccess::flipCard(controller, gameId, cardId);
            } else {
                res.body = "{\"error\":\"Invalid cardId\"}";
            }
        }
        else if (req.path.find("/api/game/") == 0 && req.path.find("/check-pair") != std::string::npos && req.method == "POST") {
            size_t gameIdStart = 10;
            size_t gameIdEnd = req.path.find("/check-pair");
            std::string gameId = req.path.substr(gameIdStart, gameIdEnd - gameIdStart);
            
            int cardId1 = -1, cardId2 = -1;
            
            std::regex card1Regex("\"cardId1\"\\s*:\\s*(\\d+)");
            std::regex card2Regex("\"cardId2\"\\s*:\\s*(\\d+)");
            std::smatch match;
            if (std::regex_search(req.body, match, card1Regex)) {
                cardId1 = std::stoi(match[1].str());
            }
            if (std::regex_search(req.body, match, card2Regex)) {
                cardId2 = std::stoi(match[1].str());
            }
            
            if (cardId1 >= 0 && cardId2 >= 0) {
                res.body = ApiControllerAccess::checkCardPair(controller, gameId, cardId1, cardId2, "");
            } else {
                res.body = "{\"error\":\"Invalid cardIds\"}";
            }
        }
        else if (req.path.find("/api/game/") == 0 && req.path.find("/check") != std::string::npos && req.method == "POST") {
            size_t gameIdStart = 10;
            size_t gameIdEnd = req.path.find("/check");
            std::string gameId = req.path.substr(gameIdStart, gameIdEnd - gameIdStart);
            
            std::vector<int> answer;
            std::regex numRegex(R"(\d+)");
            std::sregex_iterator iter(req.body.begin(), req.body.end(), numRegex);
            std::sregex_iterator end;
            for (; iter != end; ++iter) {
                answer.push_back(std::stoi(iter->str()));
            }
            
            res.body = ApiControllerAccess::checkAnswer(controller, gameId, answer, "");
        }
        else if (req.path.find("/api/game/") == 0 && req.method == "DELETE") {
            std::string gameId = req.path.substr(10);
            res.body = ApiControllerAccess::deleteGame(controller, gameId);
        }
        else if (req.path == "/" || req.path == "/index.html" || req.path == "/web/index.html") {
            std::vector<std::string> paths = {"web/index.html", "../web/index.html", "../../web/index.html"};
            bool found = false;
            for (const auto& path : paths) {
                std::ifstream file(path);
                if (file.is_open()) {
                    res.statusCode = 200;
                    res.headers["Content-Type"] = "text/html";
                    std::ostringstream oss;
                    oss << file.rdbuf();
                    res.body = oss.str();
                    found = true;
                    break;
                }
            }
            if (!found) {
                res.statusCode = 404;
                res.body = "File not found";
            }
        }
        else if (req.path.find("/web/") == 0) {
            std::string filePath = req.path.substr(1);
            std::vector<std::string> paths = {filePath, "../" + filePath, "../../" + filePath};
            bool found = false;
            for (const auto& path : paths) {
                std::ifstream file(path);
                if (file.is_open()) {
                    res.statusCode = 200;
                    if (req.path.find(".css") != std::string::npos) {
                        res.headers["Content-Type"] = "text/css";
                    } else if (req.path.find(".js") != std::string::npos) {
                        res.headers["Content-Type"] = "application/javascript";
                    }
                    std::ostringstream oss;
                    oss << file.rdbuf();
                    res.body = oss.str();
                    found = true;
                    break;
                }
            }
            if (!found) {
                res.statusCode = 404;
                res.body = "File not found";
            }
        }
        else {
            res.statusCode = 404;
            res.body = "{\"error\":\"Not found\"}";
        }
        
        return res;
    });
    
    return 0;
}

