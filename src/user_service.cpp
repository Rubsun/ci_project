#include "user_service.h"
#include "user.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <random>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/evp.h>

namespace MemoryTrainer {

UserService::UserService() {
    loadUsers();
}

UserService::~UserService() {
    saveUsers();
}

::std::string UserService::registerUser(const ::std::string& username, const ::std::string& email, const ::std::string& password) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    
    for (const auto& [id, user] : users_) {
        if (user->username == username) {
            return "";
        }
        if (user->email == email) {
            return "";
        }
    }
    
    ::std::string userId = generateUserId();
    ::std::string passwordHash = hashPassword(password);
    
    auto user = ::std::make_shared<User>(username, email, passwordHash);
    user->id = userId;
    
    users_[userId] = user;
    saveUsers();
    
    return userId;
}

::std::string UserService::loginUser(const ::std::string& username, const ::std::string& password) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    
    for (const auto& [id, user] : users_) {
        if (user->username == username) {
            if (verifyPassword(password, user->passwordHash)) {
                ::std::string sessionId = generateSessionId();
                sessions_[sessionId] = user->id;
                user->lastLogin = ::std::chrono::system_clock::now();
                saveUsers();
                return sessionId;
            }
            return "";
        }
    }
    
    return "";
}

bool UserService::logoutUser(const ::std::string& sessionId) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    return sessions_.erase(sessionId) > 0;
}

::std::shared_ptr<User> UserService::getUserById(const ::std::string& userId) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    auto it = users_.find(userId);
    return (it != users_.end()) ? it->second : nullptr;
}

::std::shared_ptr<User> UserService::getUserBySession(const ::std::string& sessionId) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        
        auto userIt = users_.find(it->second);
        return (userIt != users_.end()) ? userIt->second : nullptr;
    }
    return nullptr;
}

::std::shared_ptr<User> UserService::getUserByUsername(const ::std::string& username) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    for (const auto& [id, user] : users_) {
        if (user->username == username) {
            return user;
        }
    }
    return nullptr;
}

void UserService::updateUserStats(const ::std::string& userId, int score, bool won) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    
    auto userIt = users_.find(userId);
    if (userIt != users_.end()) {
        auto user = userIt->second;
        user->totalScore += score;
        user->gamesPlayed++;
        if (won) {
            user->gamesWon++;
        }
        saveUsers();
    }
}

::std::vector<LeaderboardEntry> UserService::getLeaderboard(int limit) {
    ::std::lock_guard<::std::mutex> lock(usersMutex_);
    
    ::std::vector<LeaderboardEntry> entries;
    
    for (const auto& [id, user] : users_) {
        LeaderboardEntry entry;
        entry.userId = user->id;
        entry.username = user->username;
        entry.totalScore = user->totalScore;
        entry.gamesWon = user->gamesWon;
        entry.winRate = (user->gamesPlayed > 0) ? 
            (static_cast<double>(user->gamesWon) / user->gamesPlayed * 100.0) : 0.0;
        entries.push_back(entry);
    }

    ::std::sort(entries.begin(), entries.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.totalScore > b.totalScore;
        });
    
    for (size_t i = 0; i < entries.size() && i < static_cast<size_t>(limit); ++i) {
        entries[i].rank = static_cast<int>(i + 1);
    }
    
    if (entries.size() > static_cast<size_t>(limit)) {
        entries.resize(limit);
    }
    
    return entries;
}

void UserService::saveUsers() {
    ::std::ofstream file(dataFile_, ::std::ios::binary);
    if (!file.is_open()) return;
    
    for (const auto& [id, user] : users_) {
        file << user->id << "\n";
        file << user->username << "\n";
        file << user->email << "\n";
        file << user->passwordHash << "\n";
        file << user->totalScore << "\n";
        file << user->gamesPlayed << "\n";
        file << user->gamesWon << "\n";
        
        auto time1 = ::std::chrono::duration_cast<::std::chrono::seconds>(
            user->createdAt.time_since_epoch()).count();
        auto time2 = ::std::chrono::duration_cast<::std::chrono::seconds>(
            user->lastLogin.time_since_epoch()).count();
        file << time1 << "\n" << time2 << "\n";
        file << "---\n";
    }
}

void UserService::loadUsers() {
    ::std::ifstream file(dataFile_);
    if (!file.is_open()) return;
    
    ::std::string line;
    while (::std::getline(file, line)) {
        if (line == "---") continue;
        
        auto user = ::std::make_shared<User>();
        user->id = line;
        
        ::std::getline(file, user->username);
        ::std::getline(file, user->email);
        ::std::getline(file, user->passwordHash);
        
        ::std::getline(file, line);
        user->totalScore = ::std::stoi(line);
        
        ::std::getline(file, line);
        user->gamesPlayed = ::std::stoi(line);
        
        ::std::getline(file, line);
        user->gamesWon = ::std::stoi(line);
        
        ::std::getline(file, line);
        int64_t createdAt = ::std::stoll(line);
        user->createdAt = ::std::chrono::system_clock::time_point(
            ::std::chrono::seconds(createdAt));
        
        ::std::getline(file, line);
        int64_t lastLogin = ::std::stoll(line);
        user->lastLogin = ::std::chrono::system_clock::time_point(
            ::std::chrono::seconds(lastLogin));
        
        users_[user->id] = user;
        
        ::std::getline(file, line);
    }
}

::std::string UserService::generateUserId() {
    static ::std::random_device rd;
    static ::std::mt19937 gen(rd());
    static ::std::uniform_int_distribution<> dis(100000, 999999);
    
    auto now = ::std::chrono::system_clock::now();
    auto time = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    ::std::ostringstream oss;
    oss << "user_" << time << "_" << dis(gen);
    return oss.str();
}

::std::string UserService::generateSessionId() {
    static ::std::random_device rd;
    static ::std::mt19937 gen(rd());
    static ::std::uniform_int_distribution<> dis(100000, 999999);
    
    auto now = ::std::chrono::system_clock::now();
    auto time = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    ::std::ostringstream oss;
    oss << "session_" << time << "_" << dis(gen);
    return oss.str();
}

::std::string UserService::hashPassword(const ::std::string& password) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
    EVP_DigestInit_ex(mdctx, md, nullptr);
    EVP_DigestUpdate(mdctx, password.c_str(), password.length());
    EVP_DigestFinal_ex(mdctx, hash, &hashLen);
    EVP_MD_CTX_free(mdctx);
    
    ::std::ostringstream oss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        oss << ::std::hex << ::std::setw(2) << ::std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

bool UserService::verifyPassword(const ::std::string& password, const ::std::string& hash) {
    return hashPassword(password) == hash;
}

}

