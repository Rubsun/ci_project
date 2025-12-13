#pragma once

#include <vector>
#include <string>
#include <map>

#include "memory_game.h"

namespace MemoryTrainer {


struct Card {
    int id;           
    int value;        
    bool flipped;     
    bool matched;     
    
    Card(int cardId, int val) : id(cardId), value(val), flipped(false), matched(false) {}
};


class CardPairsGame : public MemoryGame {
public:
    CardPairsGame(Difficulty difficulty);
    
    void generate() override;
    GameResult checkAnswer(const ::std::vector<int>& answer) override;
    ::std::vector<int> getSequence() const override;
    
    
    ::std::vector<Card> getCards() const { return cards_; }
    bool flipCard(int cardId);
    ::std::pair<int, int> getFlippedCards() const; 
    bool checkPair(int cardId1, int cardId2);
    bool isGameComplete() const;
    int getMovesCount() const { return movesCount_; }
    int getPairsFound() const { return pairsFound_; }
    int getCardCount() const; 
    void resetFlippedCards();
    
private:
    ::std::vector<Card> cards_;
    ::std::vector<int> flippedCardIds_; 
    int movesCount_ = 0;
    int pairsFound_ = 0;
    int totalPairs_ = 0;
    
    void shuffleCards();
};

} 

