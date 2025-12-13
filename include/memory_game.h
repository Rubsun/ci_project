#pragma once

#include <vector>
#include <string>
#include <memory>
#include <random>
#include <chrono>
#include <cstdint>

namespace MemoryTrainer {

enum class GameType {
    SEQUENCE,
    PAIRS,
    NUMBERS
};

enum class Difficulty {
    EASY,
    MEDIUM,
    HARD
};

struct GameResult {
    bool success;
    int score;
    int timeMs;
    ::std::string message;
};

class MemoryGame {
public:
    MemoryGame(GameType type, Difficulty difficulty);
    virtual ~MemoryGame() = default;

    virtual void generate() = 0;
    
    virtual GameResult checkAnswer(const ::std::vector<int>& answer) = 0;
    
    virtual ::std::vector<int> getSequence() const = 0;
    
    GameType getType() const { return type_; }
    Difficulty getDifficulty() const { return difficulty_; }
    
    int getMemorizationTime() const;

protected:
    GameType type_;
    Difficulty difficulty_;
    ::std::vector<int> sequence_;
    ::std::mt19937 rng_;
    
    int getSequenceLength() const;
    int getNumberRange() const;
};

class SequenceGame : public MemoryGame {
public:
    SequenceGame(Difficulty difficulty);
    
    void generate() override;
    GameResult checkAnswer(const ::std::vector<int>& answer) override;
    ::std::vector<int> getSequence() const override;
};

class PairsGame : public MemoryGame {
public:
    PairsGame(Difficulty difficulty);
    
    void generate() override;
    GameResult checkAnswer(const ::std::vector<int>& answer) override;
    ::std::vector<int> getSequence() const override;
    
    ::std::vector<::std::pair<int, int>> getPairs() const;

private:
    ::std::vector<::std::pair<int, int>> pairs_;
};

} 