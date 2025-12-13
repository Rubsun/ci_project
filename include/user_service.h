#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <fstream>
#include <sstream>

#include "user.h"
#include "memory_game.h"

namespace MemoryTrainer {

class UserService {
public:
    UserService();
    ~UserService();
    
    ::std::string registerUser(const ::std::string& username, const ::std::string& email, const ::std::string& password);
    ::std::string loginUser(const ::std::string& username, const ::std::string& password);
    bool logoutUser(const ::std::string& sessionId);
    
    ::std::shared_ptr<User> getUserById(const ::std::string& userId);
    ::std::shared_ptr<User> getUserBySession(const ::std::string& sessionId);
    ::std::shared_ptr<User> getUserByUsername(const ::std::string& username);
    
    void updateUserStats(const ::std::string& userId, int score, bool won);
    
    ::std::vector<LeaderboardEntry> getLeaderboard(int limit = 10);
    
    void saveUsers();
    void loadUsers();

private:
    ::std::unordered_map<::std::string, ::std::shared_ptr<User>> users_;
    ::std::unordered_map<::std::string, ::std::string> sessions_; 
    ::std::mutex usersMutex_;
    ::std::string dataFile_ = "users.dat";
    
    ::std::string generateUserId();
    ::std::string generateSessionId();
    ::std::string hashPassword(const ::std::string& password);
    bool verifyPassword(const ::std::string& password, const ::std::string& hash);
};

}


