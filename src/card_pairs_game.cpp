#include "card_pairs_game.h"
#include <algorithm>
#include <random>

namespace MemoryTrainer {

CardPairsGame::CardPairsGame(Difficulty difficulty)
    : MemoryGame(GameType::PAIRS, difficulty) {
    totalPairs_ = getCardCount() / 2;
    movesCount_ = 0;
    pairsFound_ = 0;
}

int CardPairsGame::getCardCount() const {
    switch (difficulty_) {
        case Difficulty::EASY:
            return 8;  
        case Difficulty::MEDIUM:
            return 12; 
        case Difficulty::HARD:
            return 16; 
        default:
            return 8;
    }
}

void CardPairsGame::generate() {
    cards_.clear();
    flippedCardIds_.clear();
    movesCount_ = 0;
    pairsFound_ = 0;
    
    int cardCount = getCardCount();
    int pairCount = cardCount / 2;
    
    
    ::std::uniform_int_distribution<int> dist(1, getNumberRange());
    
    int cardId = 0;
    for (int i = 0; i < pairCount; ++i) {
        int value = dist(rng_);
        
        cards_.emplace_back(cardId++, value);
        cards_.emplace_back(cardId++, value);
    }
    
    shuffleCards();
}

void CardPairsGame::shuffleCards() {
    ::std::shuffle(cards_.begin(), cards_.end(), rng_);
}

bool CardPairsGame::flipCard(int cardId) {
    
    auto it = ::std::find_if(cards_.begin(), cards_.end(),
        [cardId](const Card& card) { return card.id == cardId; });
    
    if (it == cards_.end() || it->flipped || it->matched) {
        return false;
    }
    
    
    if (flippedCardIds_.size() >= 2) {
        return false;
    }
    
    it->flipped = true;
    flippedCardIds_.push_back(cardId);
    
    return true;
}

::std::pair<int, int> CardPairsGame::getFlippedCards() const {
    if (flippedCardIds_.size() == 0) {
        return {-1, -1};
    } else if (flippedCardIds_.size() == 1) {
        return {flippedCardIds_[0], -1};
    } else {
        return {flippedCardIds_[0], flippedCardIds_[1]};
    }
}

bool CardPairsGame::checkPair(int cardId1, int cardId2) {
    auto it1 = ::std::find_if(cards_.begin(), cards_.end(),
        [cardId1](const Card& card) { return card.id == cardId1; });
    auto it2 = ::std::find_if(cards_.begin(), cards_.end(),
        [cardId2](const Card& card) { return card.id == cardId2; });
    
    if (it1 == cards_.end() || it2 == cards_.end()) {
        return false;
    }
    
    movesCount_++;
    
    if (it1->value == it2->value) {
        
        it1->matched = true;
        it2->matched = true;
        pairsFound_++;
        return true;
    }
    
    return false;
}

void CardPairsGame::resetFlippedCards() {
    for (auto& card : cards_) {
        if (!card.matched) {
            card.flipped = false;
        }
    }
    flippedCardIds_.clear();
}

bool CardPairsGame::isGameComplete() const {
    return pairsFound_ >= totalPairs_;
}

GameResult CardPairsGame::checkAnswer(const ::std::vector<int>& answer) {
    GameResult result;
    result.success = isGameComplete();
    
    if (result.success) {
        
        int baseScore = totalPairs_ * 10;
        int moveBonus = ::std::max(0, (totalPairs_ * 2 - movesCount_) * 5);
        result.score = baseScore + moveBonus;
        result.message = "Поздравляем! Все пары найдены!";
    } else {
        result.score = 0;
        result.message = "Игра ещё не завершена";
    }
    
    return result;
}

::std::vector<int> CardPairsGame::getSequence() const {
    ::std::vector<int> seq;
    seq.reserve(cards_.size());
    for (const auto& card : cards_) {
        seq.push_back(card.value);
    }
    return seq;
}

}



