#include "memory_game.h"
#include <algorithm>
#include <random>
#include <chrono>

namespace MemoryTrainer {

MemoryGame::MemoryGame(GameType type, Difficulty difficulty)
    : type_(type), difficulty_(difficulty), rng_(::std::chrono::steady_clock::now().time_since_epoch().count()) {
}

int MemoryGame::getMemorizationTime() const {
    switch (difficulty_) {
        case Difficulty::EASY:
            return 5000;
        case Difficulty::MEDIUM:
            return 3000;
        case Difficulty::HARD:
            return 2000;
        default:
            return 3000;
    }
}

int MemoryGame::getSequenceLength() const {
    switch (difficulty_) {
        case Difficulty::EASY:
            return 4;
        case Difficulty::MEDIUM:
            return 6;
        case Difficulty::HARD:
            return 8;
        default:
            return 4;
    }
}

int MemoryGame::getNumberRange() const {
    switch (difficulty_) {
        case Difficulty::EASY:
            return 5;
        case Difficulty::MEDIUM:
            return 7;
        case Difficulty::HARD:
            return 9;
        default:
            return 5;
    }
}


SequenceGame::SequenceGame(Difficulty difficulty)
    : MemoryGame(GameType::SEQUENCE, difficulty) {
}

void SequenceGame::generate() {
    sequence_.clear();
    sequence_.reserve(getSequenceLength());
    
    ::std::uniform_int_distribution<int> dist(1, getNumberRange());
    
    for (int i = 0; i < getSequenceLength(); ++i) {
        sequence_.push_back(dist(rng_));
    }
}

GameResult SequenceGame::checkAnswer(const ::std::vector<int>& answer) {
    GameResult result;
    result.success = (answer == sequence_);
    
    if (result.success) {
        result.score = getSequenceLength() * 10;
        result.message = "Отлично! Вы правильно запомнили последовательность!";
    } else {
        result.score = 0;
        result.message = "Неверно. Попробуйте еще раз!";
    }
    
    return result;
}

::std::vector<int> SequenceGame::getSequence() const {
    return sequence_;
}


PairsGame::PairsGame(Difficulty difficulty)
    : MemoryGame(GameType::PAIRS, difficulty) {
}

void PairsGame::generate() {
    pairs_.clear();
    sequence_.clear();
    
    int pairCount = getSequenceLength() / 2;
    ::std::uniform_int_distribution<int> dist(1, getNumberRange());
    
    for (int i = 0; i < pairCount; ++i) {
        int value = dist(rng_);
        pairs_.emplace_back(value, value);
    }

    for (const auto& pair : pairs_) {
        sequence_.push_back(pair.first);
        sequence_.push_back(pair.second);
    }
    
    ::std::shuffle(sequence_.begin(), sequence_.end(), rng_);
}

GameResult PairsGame::checkAnswer(const ::std::vector<int>& answer) {
    GameResult result;
    
    if (answer.size() != sequence_.size()) {
        result.success = false;
        result.score = 0;
        result.message = "Неверное количество элементов!";
        return result;
    }
    
    bool allPairsCorrect = true;
    for (size_t i = 0; i < pairs_.size(); ++i) {
        int first = answer[i * 2];
        int second = answer[i * 2 + 1];
        
        if (first != second || first != pairs_[i].first) {
            allPairsCorrect = false;
            break;
        }
    }
    
    result.success = allPairsCorrect;
    if (result.success) {
        result.score = pairs_.size() * 15;
        result.message = "Отлично! Все пары найдены правильно!";
    } else {
        result.score = 0;
        result.message = "Неверно. Попробуйте еще раз!";
    }
    
    return result;
}

::std::vector<int> PairsGame::getSequence() const {
    return sequence_;
}

::std::vector<::std::pair<int, int>> PairsGame::getPairs() const {
    return pairs_;
}

}


