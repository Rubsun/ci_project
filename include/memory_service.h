#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>

#include "memory_game.h"

namespace MemoryTrainer {

class MemoryService {
public:
    MemoryService();
    
    ::std::string createGame(GameType type, Difficulty difficulty);
    
    ::std::shared_ptr<MemoryGame> getGame(const ::std::string& gameId);
    
    void removeGame(const ::std::string& gameId);
    
    void cleanup();

private:
    ::std::unordered_map<::std::string, ::std::shared_ptr<MemoryGame>> games_;
    ::std::mutex gamesMutex_;
    ::std::string generateGameId();
};

}


