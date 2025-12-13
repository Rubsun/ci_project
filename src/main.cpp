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
#include <exception>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <regex>

namespace SimpleHttp {
    struct Request {
        ::std::string method;
        ::std::string path;
        ::std::string body;
        ::std::map<::std::string, ::std::string> queryParams;
    };
    
    struct Response {
        int statusCode = 200;
        ::std::string body;
        ::std::map<::std::string, ::std::string> headers;
        
        ::std::string toString() const {
            ::std::ostringstream oss;
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
        
        void start(::std::function<Response(const Request&)> handler) {
            handler_ = handler;
            running_ = true;
            
            int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket < 0) {
                ::std::cerr << "Error creating socket" << ::std::endl;
                return;
            }
            
            int opt = 1;
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port_);
            
            if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
                ::std::cerr << "Error binding socket" << ::std::endl;
                close(serverSocket);
                return;
            }
            
            if (listen(serverSocket, 10) < 0) {
                ::std::cerr << "Error listening" << ::std::endl;
                close(serverSocket);
                return;
            }
            
            ::std::cout << "Server started on port " << port_ << ::std::endl;
            
            while (running_) {
                sockaddr_in clientAddress{};
                socklen_t clientLen = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLen);
                
                if (clientSocket < 0) {
                    continue;
                }
                
                ::std::thread([this, clientSocket]() {
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
            
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            
            ::std::string requestData;
            char buffer[8192] = {0};
            
            
            ssize_t totalRead = 0;
            bool headersComplete = false;
            
            while (totalRead < 8191 && !headersComplete) {
                ssize_t bytesRead = recv(clientSocket, buffer + totalRead, 8191 - totalRead, 0);
                if (bytesRead <= 0) {
                    close(clientSocket);
                    return;
                }
                totalRead += bytesRead;
                buffer[totalRead] = '\0';
                
                
                ::std::string currentData(buffer, totalRead);
                size_t headerEnd = currentData.find("\r\n\r\n");
                if (headerEnd != ::std::string::npos) {
                    headersComplete = true;
                    requestData = currentData;
                }
            }
            
            if (!headersComplete) {
                requestData = ::std::string(buffer, totalRead);
            }
            
            
            size_t contentLength = 0;
            size_t headerEnd = requestData.find("\r\n\r\n");
            if (headerEnd != ::std::string::npos) {
                ::std::string headers = requestData.substr(0, headerEnd);
                size_t clPos = headers.find("Content-Length:");
                if (clPos != ::std::string::npos) {
                    size_t clStart = headers.find(":", clPos) + 1;
                    size_t clEnd = headers.find("\r\n", clStart);
                    if (clEnd == ::std::string::npos) clEnd = headers.length();
                    ::std::string clStr = headers.substr(clStart, clEnd - clStart);
                    
                    clStr.erase(0, clStr.find_first_not_of(" \t"));
                    size_t lastNonSpace = clStr.find_last_not_of(" \t");
                    if (lastNonSpace != ::std::string::npos) {
                        clStr = clStr.substr(0, lastNonSpace + 1);
                    }
                    if (!clStr.empty()) {
                        try {
                            contentLength = ::std::stoul(clStr);
                        } catch (...) {
                            contentLength = 0;
                        }
                    }
                }
                
                
                ::std::string body = requestData.substr(headerEnd + 4);
                size_t bodyRead = body.length();
                
                
                while (bodyRead < contentLength && bodyRead < 8192) {
                    char bodyBuffer[1024] = {0};
                    size_t toRead = (contentLength - bodyRead < 1024) ? (contentLength - bodyRead) : 1024;
                    ssize_t bytesRead = recv(clientSocket, bodyBuffer, toRead, 0);
                    if (bytesRead <= 0) {
                        
                        break;
                    }
                    body.append(bodyBuffer, bytesRead);
                    bodyRead += bytesRead;
                }
                
                requestData = headers + "\r\n\r\n" + body;
            }
            
            Request req = parseRequest(requestData);
            Response res;
            
            try {
                res = handler_(req);
            } catch (const ::std::exception& e) {
                ::std::cerr << "Exception in handler: " << e.what() << ::std::endl;
                ::std::cerr << "Request path: " << req.path << ::std::endl;
                res.statusCode = 500;
                res.body = "{\"error\":\"Internal server error\"}";
            } catch (...) {
                ::std::cerr << "Unknown exception in handler" << ::std::endl;
                ::std::cerr << "Request path: " << req.path << ::std::endl;
                res.statusCode = 500;
                res.body = "{\"error\":\"Internal server error\"}";
            }
            
            
            if (res.body.find("{") == 0 || res.body.find("[") == 0) {
                res.headers["Content-Type"] = "application/json";
            }
            res.headers["Access-Control-Allow-Origin"] = "*";
            
            ::std::string responseStr = res.toString();
            ssize_t sent = send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
            if (sent < 0) {
                ::std::cerr << "Error sending response" << ::std::endl;
            } else {
                
                size_t totalSent = sent;
                while (totalSent < responseStr.length()) {
                    ssize_t bytesSent = send(clientSocket, responseStr.c_str() + totalSent, 
                                            responseStr.length() - totalSent, 0);
                    if (bytesSent <= 0) break;
                    totalSent += bytesSent;
                }
            }
            
            
            shutdown(clientSocket, SHUT_WR);
            close(clientSocket);
        }
        
        Request parseRequest(const ::std::string& raw) {
            Request req;
            
            
            size_t headerEnd = raw.find("\r\n\r\n");
            if (headerEnd == ::std::string::npos) {
                headerEnd = raw.find("\n\n");
            }
            
            ::std::string headers;
            if (headerEnd != ::std::string::npos) {
                headers = raw.substr(0, headerEnd);
                req.body = raw.substr(headerEnd + (raw.find("\r\n\r\n") != ::std::string::npos ? 4 : 2));
            } else {
                headers = raw;
            }
            
            
            size_t firstLineEnd = headers.find("\r\n");
            if (firstLineEnd == ::std::string::npos) {
                firstLineEnd = headers.find("\n");
            }
            if (firstLineEnd != ::std::string::npos) {
                ::std::string firstLine = headers.substr(0, firstLineEnd);
                ::std::istringstream firstLineStream(firstLine);
                firstLineStream >> req.method >> req.path;
            }
            
            
            size_t qPos = req.path.find('?');
            if (qPos != ::std::string::npos) {
                ::std::string query = req.path.substr(qPos + 1);
                req.path = req.path.substr(0, qPos);
                
                ::std::regex paramRegex("([^=&]+)=([^&]*)");
                ::std::sregex_iterator iter(query.begin(), query.end(), paramRegex);
                ::std::sregex_iterator end;
                
                for (; iter != end; ++iter) {
                    req.queryParams[iter->str(1)] = iter->str(2);
                }
            }
            
            return req;
        }
        
        int port_;
        bool running_;
        ::std::function<Response(const Request&)> handler_;
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
        
        
        if (req.path == "/api/register" && req.method == "POST") {
            ::std::string username, email, password;
            ::std::string cleanBody = req.body;
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\n'), cleanBody.end());
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\r'), cleanBody.end());
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\t'), cleanBody.end());
            
            ::std::regex usernameRegex("\"username\"\\s*:\\s*\"([^\"]+)\"");
            ::std::regex emailRegex("\"email\"\\s*:\\s*\"([^\"]+)\"");
            ::std::regex passwordRegex("\"password\"\\s*:\\s*\"([^\"]+)\"");
            ::std::smatch match;
            
            if (::std::regex_search(cleanBody, match, usernameRegex)) username = match[1].str();
            if (::std::regex_search(cleanBody, match, emailRegex)) email = match[1].str();
            if (::std::regex_search(cleanBody, match, passwordRegex)) password = match[1].str();
            
            res.body = ApiControllerAccess::registerUser(controller, username, email, password);
        }
        else if (req.path == "/api/login" && req.method == "POST") {
            ::std::string username, password;
            ::std::string cleanBody = req.body;
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\n'), cleanBody.end());
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\r'), cleanBody.end());
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\t'), cleanBody.end());
            
            ::std::regex usernameRegex("\"username\"\\s*:\\s*\"([^\"]+)\"");
            ::std::regex passwordRegex("\"password\"\\s*:\\s*\"([^\"]+)\"");
            ::std::smatch match;
            
            if (::std::regex_search(cleanBody, match, usernameRegex)) username = match[1].str();
            if (::std::regex_search(cleanBody, match, passwordRegex)) password = match[1].str();
            
            res.body = ApiControllerAccess::loginUser(controller, username, password);
        }
        else if (req.path == "/api/logout" && req.method == "POST") {
            ::std::string sessionId;
            ::std::string cleanBody = req.body;
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\n'), cleanBody.end());
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\r'), cleanBody.end());
            cleanBody.erase(::std::remove(cleanBody.begin(), cleanBody.end(), '\t'), cleanBody.end());
            ::std::regex sessionRegex("\"sessionId\"\\s*:\\s*\"([^\"]+)\"");
            ::std::smatch match;
            if (::std::regex_search(cleanBody, match, sessionRegex)) sessionId = match[1].str();
            res.body = ApiControllerAccess::logoutUser(controller, sessionId);
        }
        else if (req.path == "/api/user" && req.method == "GET") {
            ::std::string sessionId = req.queryParams.count("sessionId") ? req.queryParams.at("sessionId") : "";
            res.body = ApiControllerAccess::getUser(controller, sessionId);
        }
        else if (req.path == "/api/leaderboard" && req.method == "GET") {
            int limit = req.queryParams.count("limit") ? ::std::stoi(req.queryParams.at("limit")) : 20;
            ::std::string leaderboardData = ApiControllerAccess::getLeaderboard(controller, limit);
            res.body = "{\"leaderboard\":" + leaderboardData + "}";
        }
        else if (req.path == "/api/game" && req.method == "POST") {
            ::std::string type = req.queryParams.count("type") ? req.queryParams.at("type") : "sequence";
            ::std::string difficulty = req.queryParams.count("difficulty") ? req.queryParams.at("difficulty") : "medium";
            res.body = ApiControllerAccess::createGame(controller, type, difficulty, "");
        }
        else if (req.path.find("/api/game/") == 0 && req.method == "GET") {
            ::std::string gameId = req.path.substr(10); 
            res.body = ApiControllerAccess::getGame(controller, gameId);
        }
        else if (req.path.find("/api/game/") == 0 && req.path.find("/flip") != ::std::string::npos && req.method == "POST") {
            size_t gameIdStart = 10;
            size_t gameIdEnd = req.path.find("/flip");
            ::std::string gameId = req.path.substr(gameIdStart, gameIdEnd - gameIdStart);
            
            ::std::regex cardRegex("\"cardId\"\\s*:\\s*(\\d+)");
            ::std::smatch match;
            int cardId = -1;
            if (::std::regex_search(req.body, match, cardRegex)) {
                cardId = ::std::stoi(match[1].str());
            }
            
            if (cardId >= 0) {
                res.body = ApiControllerAccess::flipCard(controller, gameId, cardId);
            } else {
                res.body = "{\"error\":\"Invalid cardId\"}";
            }
        }
        else if (req.path.find("/api/game/") == 0 && req.path.find("/check-pair") != ::std::string::npos && req.method == "POST") {
            size_t gameIdStart = 10;
            size_t gameIdEnd = req.path.find("/check-pair");
            ::std::string gameId = req.path.substr(gameIdStart, gameIdEnd - gameIdStart);
            
            int cardId1 = -1, cardId2 = -1;
            ::std::string sessionId = "";
            
            ::std::regex card1Regex("\"cardId1\"\\s*:\\s*(\\d+)");
            ::std::regex card2Regex("\"cardId2\"\\s*:\\s*(\\d+)");
            ::std::regex sessionRegex("\"sessionId\"\\s*:\\s*\"([^\"]+)\"");
            ::std::smatch match;
            if (::std::regex_search(req.body, match, card1Regex)) {
                cardId1 = ::std::stoi(match[1].str());
            }
            if (::std::regex_search(req.body, match, card2Regex)) {
                cardId2 = ::std::stoi(match[1].str());
            }
            if (::std::regex_search(req.body, match, sessionRegex)) {
                sessionId = match[1].str();
            }
            
            if (cardId1 >= 0 && cardId2 >= 0) {
                res.body = ApiControllerAccess::checkCardPair(controller, gameId, cardId1, cardId2, sessionId);
            } else {
                res.body = "{\"error\":\"Invalid cardIds\"}";
            }
        }
        else if (req.path.find("/api/game/") == 0 && req.path.find("/check") != ::std::string::npos && req.method == "POST") {
            try {
                size_t gameIdStart = 10;
                size_t gameIdEnd = req.path.find("/check");
                if (gameIdEnd == ::std::string::npos) {
                    res.statusCode = 400;
                    res.body = "{\"error\":\"Invalid path\"}";
                } else {
                    ::std::string gameId = req.path.substr(gameIdStart, gameIdEnd - gameIdStart);
                    
                    ::std::vector<int> answer;
                    ::std::string sessionId = "";
                    
                    
                    size_t sessionStart = req.body.find("\"sessionId\"");
                    if (sessionStart != ::std::string::npos) {
                        size_t colonPos = req.body.find(":", sessionStart);
                        if (colonPos != ::std::string::npos) {
                            size_t quoteStart = req.body.find("\"", colonPos);
                            if (quoteStart != ::std::string::npos) {
                                size_t quoteEnd = req.body.find("\"", quoteStart + 1);
                                if (quoteEnd != ::std::string::npos) {
                                    sessionId = req.body.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                                }
                            }
                        }
                    }
                    
                    
                    
                    size_t answerStart = req.body.find("\"answer\"");
                    if (answerStart != ::std::string::npos) {
                        size_t bracketStart = req.body.find("[", answerStart);
                        if (bracketStart != ::std::string::npos) {
                            size_t bracketEnd = req.body.find("]", bracketStart);
                            if (bracketEnd != ::std::string::npos) {
                                ::std::string answerStr = req.body.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                                
                                ::std::istringstream iss(answerStr);
                                ::std::string token;
                                while (::std::getline(iss, token, ',')) {
                                    
                                    token.erase(0, token.find_first_not_of(" \t"));
                                    token.erase(token.find_last_not_of(" \t") + 1);
                                    if (!token.empty()) {
                                        try {
                                            answer.push_back(::std::stoi(token));
                                        } catch (...) {
                                            
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    res.body = ApiControllerAccess::checkAnswer(controller, gameId, answer, sessionId);
                }
            } catch (const ::std::exception& e) {
                ::std::cerr << "Exception in check handler: " << e.what() << ::std::endl;
                res.statusCode = 500;
                res.body = "{\"error\":\"Internal server error\"}";
            } catch (...) {
                ::std::cerr << "Unknown exception in check handler" << ::std::endl;
                res.statusCode = 500;
                res.body = "{\"error\":\"Internal server error\"}";
            }
        }
        else if (req.path.find("/api/game/") == 0 && req.method == "DELETE") {
            ::std::string gameId = req.path.substr(10);
            res.body = ApiControllerAccess::deleteGame(controller, gameId);
        }
        else if (req.path == "/web/leaderboard.html" || req.path == "/leaderboard.html") {
            ::std::vector<::std::string> paths = {"web/leaderboard.html", "../web/leaderboard.html", "../../web/leaderboard.html"};
            bool found = false;
            for (const auto& path : paths) {
                ::std::ifstream file(path);
                if (file.is_open()) {
                    res.statusCode = 200;
                    res.headers["Content-Type"] = "text/html";
                    ::std::ostringstream oss;
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
        else if (req.path == "/" || req.path == "/index.html" || req.path == "/web/index.html") {
            ::std::vector<::std::string> paths = {"web/index.html", "../web/index.html", "../../web/index.html"};
            bool found = false;
            for (const auto& path : paths) {
                ::std::ifstream file(path);
                if (file.is_open()) {
                    res.statusCode = 200;
                    res.headers["Content-Type"] = "text/html";
                    ::std::ostringstream oss;
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
            ::std::string filePath = req.path.substr(1);
            ::std::vector<::std::string> paths = {filePath, "../" + filePath, "../../" + filePath};
            bool found = false;
            for (const auto& path : paths) {
                ::std::ifstream file(path);
                if (file.is_open()) {
                    res.statusCode = 200;
                    if (req.path.find(".css") != ::std::string::npos) {
                        res.headers["Content-Type"] = "text/css";
                    } else if (req.path.find(".js") != ::std::string::npos) {
                        res.headers["Content-Type"] = "application/javascript";
                    }
                    ::std::ostringstream oss;
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

