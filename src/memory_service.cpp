#include "memory_service.h"
#include "memory_game.h"
#include "card_pairs_game.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>

namespace MemoryTrainer {

MemoryService::MemoryService() {
}

::std::string MemoryService::createGame(GameType type, Difficulty difficulty) {
    ::std::lock_guard<::std::mutex> lock(gamesMutex_);
    
    ::std::string gameId = generateGameId();
    
    ::std::shared_ptr<MemoryGame> game;
    switch (type) {
        case GameType::SEQUENCE:
            game = ::std::make_shared<SequenceGame>(difficulty);
            break;
        case GameType::PAIRS:
            game = ::std::make_shared<CardPairsGame>(difficulty);
            break;
        case GameType::NUMBERS:
            game = ::std::make_shared<SequenceGame>(difficulty);
            break;
    }
    
    game->generate();
    games_[gameId] = game;
    
    return gameId;
}

::std::shared_ptr<MemoryGame> MemoryService::getGame(const ::std::string& gameId) {
    ::std::lock_guard<::std::mutex> lock(gamesMutex_);
    
    auto it = games_.find(gameId);
    if (it != games_.end()) {
        return it->second;
    }
    return nullptr;
}

void MemoryService::removeGame(const ::std::string& gameId) {
    ::std::lock_guard<::std::mutex> lock(gamesMutex_);
    games_.erase(gameId);
}

void MemoryService::cleanup() {
    ::std::lock_guard<::std::mutex> lock(gamesMutex_);
    games_.clear();
}

::std::string MemoryService::generateGameId() {
    auto now = ::std::chrono::system_clock::now();
    auto time = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_int_distribution<> dis(1000, 9999);
    
    ::std::ostringstream oss;
    oss << time << "_" << dis(gen);
    return oss.str();
}

}

