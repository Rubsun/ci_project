#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <vector>

#include "memory_game.h"

namespace MemoryTrainer {

struct User {
    ::std::string id;
    ::std::string username;
    ::std::string passwordHash;
    ::std::string email;
    int64_t totalScore = 0;
    int gamesPlayed = 0;
    int gamesWon = 0;
    ::std::chrono::system_clock::time_point createdAt;
    ::std::chrono::system_clock::time_point lastLogin;
    
    User() = default;
    User(const ::std::string& uname, const ::std::string& email, const ::std::string& pwdHash)
        : username(uname), email(email), passwordHash(pwdHash) {
        createdAt = ::std::chrono::system_clock::now();
        lastLogin = createdAt;
    }
};

struct GameSession {
    ::std::string sessionId;
    ::std::string userId;
    ::std::string gameId;
    GameType gameType;
    Difficulty difficulty;
    int score = 0;
    bool completed = false;
    ::std::chrono::system_clock::time_point startedAt;
    ::std::chrono::system_clock::time_point completedAt;
};

struct LeaderboardEntry {
    ::std::string userId;
    ::std::string username;
    int totalScore;
    int gamesWon;
    double winRate;
    int rank;
};

}

