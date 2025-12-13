#pragma once

#include <string>
#include <memory>
#include <vector>

#include "memory_service.h"
#include "user_service.h"

namespace MemoryTrainer {

class ApiController {
public:
    ApiController(MemoryService& service, UserService& userService);

private:
    MemoryService& service_;
    UserService& userService_;
    
    
    ::std::string handleCreateGame(const ::std::string& type, const ::std::string& difficulty, const ::std::string& sessionId);
    ::std::string handleGetGame(const ::std::string& gameId);
    ::std::string handleCheckAnswer(const ::std::string& gameId, const ::std::vector<int>& answer, const ::std::string& sessionId);
    ::std::string handleDeleteGame(const ::std::string& gameId);
    
    
    ::std::string handleFlipCard(const ::std::string& gameId, int cardId);
    ::std::string handleCheckCardPair(const ::std::string& gameId, int cardId1, int cardId2, const ::std::string& sessionId);
    
    
    ::std::string handleRegister(const ::std::string& username, const ::std::string& email, const ::std::string& password);
    ::std::string handleLogin(const ::std::string& username, const ::std::string& password);
    ::std::string handleLogout(const ::std::string& sessionId);
    ::std::string handleGetUser(const ::std::string& sessionId);
    ::std::string handleGetLeaderboard(int limit);
    
    
    friend class ApiControllerAccess;
};


class ApiControllerAccess {
public:
    static ::std::string createGame(ApiController& ctrl, const ::std::string& type, const ::std::string& difficulty, const ::std::string& sessionId = "") {
        return ctrl.handleCreateGame(type, difficulty, sessionId);
    }
    static ::std::string getGame(ApiController& ctrl, const ::std::string& gameId) {
        return ctrl.handleGetGame(gameId);
    }
    static ::std::string checkAnswer(ApiController& ctrl, const ::std::string& gameId, const ::std::vector<int>& answer, const ::std::string& sessionId = "") {
        return ctrl.handleCheckAnswer(gameId, answer, sessionId);
    }
    static ::std::string deleteGame(ApiController& ctrl, const ::std::string& gameId) {
        return ctrl.handleDeleteGame(gameId);
    }
    static ::std::string flipCard(ApiController& ctrl, const ::std::string& gameId, int cardId) {
        return ctrl.handleFlipCard(gameId, cardId);
    }
    static ::std::string checkCardPair(ApiController& ctrl, const ::std::string& gameId, int cardId1, int cardId2, const ::std::string& sessionId = "") {
        return ctrl.handleCheckCardPair(gameId, cardId1, cardId2, sessionId);
    }
    static ::std::string registerUser(ApiController& ctrl, const ::std::string& username, const ::std::string& email, const ::std::string& password) {
        return ctrl.handleRegister(username, email, password);
    }
    static ::std::string loginUser(ApiController& ctrl, const ::std::string& username, const ::std::string& password) {
        return ctrl.handleLogin(username, password);
    }
    static ::std::string logoutUser(ApiController& ctrl, const ::std::string& sessionId) {
        return ctrl.handleLogout(sessionId);
    }
    static ::std::string getUser(ApiController& ctrl, const ::std::string& sessionId) {
        return ctrl.handleGetUser(sessionId);
    }
    static ::std::string getLeaderboard(ApiController& ctrl, int limit = 10) {
        return ctrl.handleGetLeaderboard(limit);
    }
};

} 

