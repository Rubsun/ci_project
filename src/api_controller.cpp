#include "api_controller.h"
#include "memory_service.h"
#include "user_service.h"
#include "card_pairs_game.h"
#include <sstream>
#include <iostream>
#include <regex>

namespace SimpleJson {
    ::std::string object(const ::std::vector<::std::pair<::std::string, ::std::string>>& fields) {
        ::std::ostringstream oss;
        oss << "{";
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << fields[i].first << "\":";
            ::std::string value = fields[i].second;
            
            if (!value.empty() && (value[0] == '{' || value[0] == '[')) {
                oss << value;
            } else if (value == "true" || value == "false") {
                oss << value;
            } else {
                oss << "\"";
                for (char c : value) {
                    if (c == '"') oss << "\\\"";
                    else if (c == '\\') oss << "\\\\";
                    else if (c == '\n') oss << "\\n";
                    else if (c == '\r') oss << "\\r";
                    else if (c == '\t') oss << "\\t";
                    else oss << c;
                }
                oss << "\"";
            }
        }
        oss << "}";
        return oss.str();
    }
    
    ::std::string array(const ::std::vector<int>& values) {
        ::std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < values.size(); ++i) {
            oss << values[i];
            if (i < values.size() - 1) oss << ",";
        }
        oss << "]";
        return oss.str();
    }
    
    ::std::string escape(const ::std::string& str) {
        ::std::ostringstream oss;
        for (char c : str) {
            if (c == '"') oss << "\\\"";
            else if (c == '\\') oss << "\\\\";
            else if (c == '\n') oss << "\\n";
            else oss << c;
        }
        return oss.str();
    }
}

namespace MemoryTrainer {

ApiController::ApiController(MemoryService& service, UserService& userService) 
    : service_(service), userService_(userService) {
}

::std::string ApiController::handleCreateGame(const ::std::string& type, const ::std::string& difficulty, const ::std::string& sessionId) {
    GameType gameType = GameType::SEQUENCE;
    if (type == "pairs" || type == "cards") gameType = GameType::PAIRS;
    else if (type == "numbers") gameType = GameType::NUMBERS;
    
    Difficulty diff = Difficulty::MEDIUM;
    if (difficulty == "easy") diff = Difficulty::EASY;
    else if (difficulty == "hard") diff = Difficulty::HARD;
    
    ::std::string gameId = service_.createGame(gameType, diff);
    auto game = service_.getGame(gameId);
    
    if (!game) {
        return SimpleJson::object({
            {"error", "Failed to create game"}
        });
    }
    
    if (gameType == GameType::PAIRS) {
        auto cardGame = ::std::dynamic_pointer_cast<CardPairsGame>(game);
        if (cardGame) {
            auto cards = cardGame->getCards();
            ::std::ostringstream cardsJson;
            cardsJson << "[";
            for (size_t i = 0; i < cards.size(); ++i) {
                if (i > 0) cardsJson << ",";
                cardsJson << "{";
                cardsJson << "\"id\":" << cards[i].id << ",";
                cardsJson << "\"value\":" << cards[i].value << ",";
                cardsJson << "\"flipped\":" << (cards[i].flipped ? "true" : "false") << ",";
                cardsJson << "\"matched\":" << (cards[i].matched ? "true" : "false");
                cardsJson << "}";
            }
            cardsJson << "]";
            
            ::std::string cardsJsonStr = cardsJson.str();
            
            return SimpleJson::object({
                {"gameId", gameId},
                {"type", "cards"},
                {"difficulty", difficulty},
                {"cards", cardsJsonStr},
                {"totalPairs", ::std::to_string(static_cast<int>(cards.size()) / 2)}
            });
        }
    }
    
    auto sequence = game->getSequence();
    
    return SimpleJson::object({
        {"gameId", gameId},
        {"type", type},
        {"difficulty", difficulty},
        {"sequence", SimpleJson::array(sequence)},
        {"memorizationTime", ::std::to_string(game->getMemorizationTime())}
    });
}

::std::string ApiController::handleGetGame(const ::std::string& gameId) {
    auto game = service_.getGame(gameId);
    
    if (!game) {
        return SimpleJson::object({
            {"error", "Game not found"}
        });
    }
    
    ::std::string typeStr = "sequence";
    if (game->getType() == GameType::PAIRS) typeStr = "cards";
    else if (game->getType() == GameType::NUMBERS) typeStr = "numbers";
    
    ::std::string diffStr = "medium";
    if (game->getDifficulty() == Difficulty::EASY) diffStr = "easy";
    else if (game->getDifficulty() == Difficulty::HARD) diffStr = "hard";
    
    if (game->getType() == GameType::PAIRS) {
        auto cardGame = ::std::dynamic_pointer_cast<CardPairsGame>(game);
        if (cardGame) {
            auto cards = cardGame->getCards();
            ::std::ostringstream cardsJson;
            cardsJson << "[";
            for (size_t i = 0; i < cards.size(); ++i) {
                if (i > 0) cardsJson << ",";
                cardsJson << "{";
                cardsJson << "\"id\":" << cards[i].id << ",";
                cardsJson << "\"value\":" << cards[i].value << ",";
                cardsJson << "\"flipped\":" << (cards[i].flipped ? "true" : "false") << ",";
                cardsJson << "\"matched\":" << (cards[i].matched ? "true" : "false");
                cardsJson << "}";
            }
            cardsJson << "]";
            
            return SimpleJson::object({
                {"gameId", gameId},
                {"type", "cards"},
                {"difficulty", diffStr},
                {"cards", cardsJson.str()},
                {"moves", ::std::to_string(cardGame->getMovesCount())},
                {"pairsFound", ::std::to_string(cardGame->getPairsFound())},
                {"isComplete", cardGame->isGameComplete() ? "true" : "false"}
            });
        }
    }
    
    auto sequence = game->getSequence();
    
    return SimpleJson::object({
        {"gameId", gameId},
        {"type", typeStr},
        {"difficulty", diffStr},
        {"sequence", SimpleJson::array(sequence)},
        {"memorizationTime", ::std::to_string(game->getMemorizationTime())}
    });
}

::std::string ApiController::handleCheckAnswer(const ::std::string& gameId, const ::std::vector<int>& answer, const ::std::string& sessionId) {
    auto game = service_.getGame(gameId);
    
    if (!game) {
        return SimpleJson::object({
            {"error", "Game not found"}
        });
    }
    
    auto result = game->checkAnswer(answer);
    
    if (!sessionId.empty()) {
        auto user = userService_.getUserBySession(sessionId);
        if (user) {
            userService_.updateUserStats(user->id, result.score, result.success);
        }
    }
    
    return SimpleJson::object({
        {"success", result.success ? "true" : "false"},
        {"score", ::std::to_string(result.score)},
        {"message", result.message}
    });
}

::std::string ApiController::handleFlipCard(const ::std::string& gameId, int cardId) {
    auto game = service_.getGame(gameId);
    
    if (!game || game->getType() != GameType::PAIRS) {
        return SimpleJson::object({
            {"error", "Game not found or invalid type"}
        });
    }
    
    auto cardGame = ::std::dynamic_pointer_cast<CardPairsGame>(game);
    if (!cardGame) {
        return SimpleJson::object({
            {"error", "Invalid game type"}
        });
    }
    
    bool flipped = cardGame->flipCard(cardId);
    if (!flipped) {
        return SimpleJson::object({
            {"error", "Cannot flip card"}
        });
    }
    
    auto cards = cardGame->getCards();
    auto flippedPair = cardGame->getFlippedCards();
    
    ::std::ostringstream cardsJson;
    cardsJson << "[";
    for (size_t i = 0; i < cards.size(); ++i) {
        cardsJson << "{";
        cardsJson << "\"id\":" << cards[i].id << ",";
        cardsJson << "\"value\":" << cards[i].value << ",";
        cardsJson << "\"flipped\":" << (cards[i].flipped ? "true" : "false") << ",";
        cardsJson << "\"matched\":" << (cards[i].matched ? "true" : "false");
        cardsJson << "}";
        if (i < cards.size() - 1) cardsJson << ",";
    }
    cardsJson << "]";
    
    ::std::ostringstream flippedJson;
    flippedJson << "[";
    if (flippedPair.first >= 0) {
        flippedJson << flippedPair.first;
        if (flippedPair.second >= 0) {
            flippedJson << "," << flippedPair.second;
        }
    }
    flippedJson << "]";
    
    return SimpleJson::object({
        {"success", "true"},
        {"cards", cardsJson.str()},
        {"flippedCards", flippedJson.str()},
        {"moves", ::std::to_string(cardGame->getMovesCount())},
        {"pairsFound", ::std::to_string(cardGame->getPairsFound())},
        {"isComplete", cardGame->isGameComplete() ? "true" : "false"}
    });
}

::std::string ApiController::handleCheckCardPair(const ::std::string& gameId, int cardId1, int cardId2, const ::std::string& sessionId) {
    auto game = service_.getGame(gameId);
    
    if (!game || game->getType() != GameType::PAIRS) {
        return SimpleJson::object({
            {"error", "Game not found or invalid type"}
        });
    }
    
    auto cardGame = ::std::dynamic_pointer_cast<CardPairsGame>(game);
    if (!cardGame) {
        return SimpleJson::object({
            {"error", "Invalid game type"}
        });
    }
    
    bool isPair = cardGame->checkPair(cardId1, cardId2);
    
    if (!isPair) {
        cardGame->resetFlippedCards();
    }

    auto cards = cardGame->getCards();
    ::std::ostringstream cardsJson;
    cardsJson << "[";
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) cardsJson << ",";
        cardsJson << "{";
        cardsJson << "\"id\":" << cards[i].id << ",";
        cardsJson << "\"value\":" << cards[i].value << ",";
        cardsJson << "\"flipped\":" << (cards[i].flipped ? "true" : "false") << ",";
        cardsJson << "\"matched\":" << (cards[i].matched ? "true" : "false");
        cardsJson << "}";
    }
    cardsJson << "]";
    
    ::std::string flippedJson = "[]";
    
    bool gameComplete = cardGame->isGameComplete();
    int score = 0;
    
    if (gameComplete) {
        auto result = cardGame->checkAnswer({});
        score = result.score;
        
        if (!sessionId.empty()) {
            auto user = userService_.getUserBySession(sessionId);
            if (user) {
                userService_.updateUserStats(user->id, score, true);
            }
        }
    }
    
    return SimpleJson::object({
        {"isPair", isPair ? "true" : "false"},
        {"cards", cardsJson.str()},
        {"flippedCards", flippedJson},
        {"moves", ::std::to_string(cardGame->getMovesCount())},
        {"pairsFound", ::std::to_string(cardGame->getPairsFound())},
        {"isComplete", gameComplete ? "true" : "false"},
        {"score", ::std::to_string(score)},
        {"message", gameComplete ? "Поздравляем! Все пары найдены!" : (isPair ? "Пара найдена!" : "Не пара, попробуйте еще раз")}
    });
}

::std::string ApiController::handleRegister(const ::std::string& username, const ::std::string& email, const ::std::string& password) {
    if (username.empty() || email.empty() || password.empty()) {
        return SimpleJson::object({
            {"error", "All fields are required"}
        });
    }
    
    ::std::string userId = userService_.registerUser(username, email, password);
    
    if (userId.empty()) {
        return SimpleJson::object({
            {"error", "Username or email already exists"}
        });
    }
    
    return SimpleJson::object({
        {"success", "true"},
        {"userId", userId},
        {"message", "User registered successfully"}
    });
}

::std::string ApiController::handleLogin(const ::std::string& username, const ::std::string& password) {
    ::std::string sessionId = userService_.loginUser(username, password);
    
    if (sessionId.empty()) {
        return SimpleJson::object({
            {"success", "false"},
            {"error", "Invalid username or password"}
        });
    }
    
    auto user = userService_.getUserBySession(sessionId);
    if (!user) {
        return SimpleJson::object({
            {"success", "false"},
            {"error", "Failed to get user data"}
        });
    }
    
    return SimpleJson::object({
        {"success", "true"},
        {"sessionId", sessionId},
        {"username", user->username},
        {"totalScore", ::std::to_string(user->totalScore)},
        {"gamesPlayed", ::std::to_string(user->gamesPlayed)},
        {"gamesWon", ::std::to_string(user->gamesWon)}
    });
}

::std::string ApiController::handleLogout(const ::std::string& sessionId) {
    bool success = userService_.logoutUser(sessionId);
    return SimpleJson::object({
        {"success", success ? "true" : "false"}
    });
}

::std::string ApiController::handleGetUser(const ::std::string& sessionId) {
    auto user = userService_.getUserBySession(sessionId);
    
    if (!user) {
        return SimpleJson::object({
            {"success", "false"},
            {"error", "User not found or session expired"}
        });
    }
    
    return SimpleJson::object({
        {"success", "true"},
        {"user", SimpleJson::object({
            {"userId", user->id},
            {"username", user->username},
            {"email", user->email},
            {"totalScore", ::std::to_string(user->totalScore)},
            {"gamesPlayed", ::std::to_string(user->gamesPlayed)},
            {"gamesWon", ::std::to_string(user->gamesWon)},
            {"winRate", ::std::to_string(user->gamesPlayed > 0 ? 
                (static_cast<double>(user->gamesWon) / user->gamesPlayed * 100.0) : 0.0)}
        })}
    });
}

::std::string ApiController::handleGetLeaderboard(int limit) {
    auto entries = userService_.getLeaderboard(limit);
    
    ::std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < entries.size(); ++i) {
        oss << "{";
        oss << "\"rank\":" << entries[i].rank << ",";
        oss << "\"username\":\"" << SimpleJson::escape(entries[i].username) << "\",";
        oss << "\"totalScore\":" << entries[i].totalScore << ",";
        oss << "\"gamesWon\":" << entries[i].gamesWon << ",";
        oss << "\"winRate\":" << entries[i].winRate;
        oss << "}";
        if (i < entries.size() - 1) oss << ",";
    }
    oss << "]";
    
    return oss.str();
}

::std::string ApiController::handleDeleteGame(const ::std::string& gameId) {
    service_.removeGame(gameId);
    return SimpleJson::object({
        {"status", "deleted"}
    });
}

}
