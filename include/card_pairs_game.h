#pragma once

#include "memory_game.h"
#include <vector>
#include <string>
#include <map>

namespace MemoryTrainer {

// Карточка для игры в пары
struct Card {
    int id;           // Уникальный ID карточки
    int value;        // Значение (для поиска пары)
    bool flipped;     // Перевернута ли карточка
    bool matched;     // Найдена ли пара
    
    Card(int cardId, int val) : id(cardId), value(val), flipped(false), matched(false) {}
};

// Игра с карточками (классическая игра на память)
class CardPairsGame : public MemoryGame {
public:
    CardPairsGame(Difficulty difficulty);
    
    void generate() override;
    GameResult checkAnswer(const std::vector<int>& answer) override;
    std::vector<int> getSequence() const override;
    
    // Специфичные для карточной игры методы
    std::vector<Card> getCards() const { return cards_; }
    bool flipCard(int cardId);
    std::pair<int, int> getFlippedCards() const; // Возвращает ID двух открытых карточек
    bool checkPair(int cardId1, int cardId2);
    bool isGameComplete() const;
    int getMovesCount() const { return movesCount_; }
    int getPairsFound() const { return pairsFound_; }
    int getCardCount() const; // Количество карточек для текущей сложности
    void resetFlippedCards();
    
private:
    std::vector<Card> cards_;
    std::vector<int> flippedCardIds_; // Максимум 2 карточки
    int movesCount_ = 0;
    int pairsFound_ = 0;
    int totalPairs_ = 0;
    
    void shuffleCards();
};

} // namespace MemoryTrainer

