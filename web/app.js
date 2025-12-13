// Глобальные переменные
let currentGameId = null;
let currentGameType = null;
let currentSequence = [];
let currentAnswer = [];
let currentSlotIndex = 0;
let memorizationTimer = null;
let gameTimer = null;
let startTime = null;
let flippedCards = [];
let checkPairTimeout = null;

async function startGame() {
    console.log('startGame called');
    const gameType = document.getElementById('gameType').value;
    const difficulty = document.getElementById('difficulty').value;
    
    console.log('Game type:', gameType, 'Difficulty:', difficulty);
    
    try {
        const url = `/api/game?type=${gameType}&difficulty=${difficulty}`;
        console.log('Fetching:', url);
        const response = await fetch(url, { method: 'POST' });
        console.log('Response status:', response.status);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const responseText = await response.text();
        console.log('Response text (first 100 chars):', responseText.substring(0, 100));
        console.log('Response text (full):', responseText);
        
        let data;
        try {
            data = JSON.parse(responseText);
            console.log('Response data:', data);
        } catch (parseError) {
            console.error('JSON Parse Error:', parseError);
            console.error('Problematic JSON:', responseText);
            throw parseError;
        }
        
        if (data.error) {
            alert('Ошибка: ' + data.error);
            return;
        }
        
        currentGameId = data.gameId;
        currentGameType = gameType;
        
        document.getElementById('gameSetup').style.display = 'none';
        document.getElementById('gameArea').style.display = 'block';
        
        if (gameType === 'cards') {
            startCardGame(data);
        } else {
            startSequenceGame(data);
        }
        
        startTime = Date.now();
        startGameTimer();
        
    } catch (error) {
        console.error('Error:', error);
        alert('Ошибка при создании игры');
    }
}

function startSequenceGame(data) {
    currentSequence = data.sequence;
    currentAnswer = [];
    currentSlotIndex = 0;
    
    document.getElementById('sequenceGame').style.display = 'block';
    document.getElementById('cardsGame').style.display = 'none';
    
    showMemorizationPhase();
    const memorizationTime = parseInt(data.memorizationTime);
    startMemorizationTimer(memorizationTime);
}

function startCardGame(data) {
    document.getElementById('sequenceGame').style.display = 'none';
    document.getElementById('cardsGame').style.display = 'block';
    
    renderCards(data.cards);
    document.getElementById('moves').textContent = '0';
    document.getElementById('pairsFound').textContent = '0';
    flippedCards = [];
}

function renderCards(cards) {
    const grid = document.getElementById('cardsGrid');
    grid.innerHTML = '';
    grid.className = 'cards-grid';
    
    cards.forEach(card => {
        const cardDiv = document.createElement('div');
        cardDiv.className = 'card';
        cardDiv.dataset.cardId = card.id;
        cardDiv.dataset.value = card.value;
        
        if (card.matched) {
            cardDiv.classList.add('matched');
        }
        
        const front = document.createElement('div');
        front.className = 'card-front';
        front.textContent = '?';
        
        const back = document.createElement('div');
        back.className = 'card-back';
        back.textContent = card.value;
        
        if (card.flipped || card.matched) {
            cardDiv.classList.add('flipped');
        }
        
        cardDiv.appendChild(front);
        cardDiv.appendChild(back);
        
        if (!card.matched) {
            cardDiv.onclick = () => flipCard(card.id);
        }
        
        grid.appendChild(cardDiv);
    });
}

async function flipCard(cardId) {
    if (flippedCards.length >= 2) return;
    if (flippedCards.includes(cardId)) return;
    
    try {
        const url = `/api/game/${currentGameId}/flip`;
        const response = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cardId })
        });
        
        const data = await response.json();
        
        if (data.error) {
            alert('Ошибка: ' + data.error);
            return;
        }
        
        renderCards(data.cards);
        flippedCards = data.flippedCards.map(id => parseInt(id));
        document.getElementById('moves').textContent = data.moves;
        document.getElementById('pairsFound').textContent = data.pairsFound;
        
        if (flippedCards.length === 2) {
            if (checkPairTimeout) clearTimeout(checkPairTimeout);
            checkPairTimeout = setTimeout(() => {
                checkCardPair(flippedCards[0], flippedCards[1]);
            }, 1000);
        }
        
        if (data.isComplete === 'true' || data.isComplete === true) {
            showResult(true, 'Поздравляем! Все пары найдены!', parseInt(data.score || 0));
        }
        
    } catch (error) {
        console.error('Error:', error);
        alert('Ошибка при переворачивании карточки');
    }
}

async function checkCardPair(cardId1, cardId2) {
    try {
        const url = `/api/game/${currentGameId}/check-pair`;
        const response = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cardId1, cardId2 })
        });
        
        const data = await response.json();
        
        if (data.error) {
            alert('Ошибка: ' + data.error);
            return;
        }
        
        renderCards(data.cards);
        flippedCards = [];
        document.getElementById('moves').textContent = data.moves;
        document.getElementById('pairsFound').textContent = data.pairsFound;
        
        if (data.isPair === 'true' || data.isPair === true) {
        } else {
        }
        
        if (data.isComplete === 'true' || data.isComplete === true) {
            showResult(true, data.message, parseInt(data.score || 0));
        }
        
    } catch (error) {
        console.error('Error:', error);
    }
}

function showMemorizationPhase() {
    document.getElementById('memorizationPhase').style.display = 'block';
    document.getElementById('answerPhase').style.display = 'none';
    document.getElementById('resultMessage').style.display = 'none';
    
    const display = document.getElementById('sequenceDisplay');
    display.innerHTML = '';
    
    currentSequence.forEach((num, index) => {
        setTimeout(() => {
            const card = document.createElement('div');
            card.className = 'number-card';
            card.textContent = num;
            display.appendChild(card);
        }, index * 200);
    });
}

function startMemorizationTimer(timeMs) {
    let remaining = Math.ceil(timeMs / 1000);
    const timerDisplay = document.getElementById('memorizationTimer');
    timerDisplay.textContent = remaining;
    
    memorizationTimer = setInterval(() => {
        remaining--;
        timerDisplay.textContent = remaining;
        
        if (remaining <= 0) {
            clearInterval(memorizationTimer);
            showAnswerPhase();
        }
    }, 1000);
}

function showAnswerPhase() {
    document.getElementById('memorizationPhase').style.display = 'none';
    document.getElementById('answerPhase').style.display = 'block';
    
    const answerInput = document.getElementById('answerInput');
    answerInput.innerHTML = '';
    
    for (let i = 0; i < currentSequence.length; i++) {
        const slot = document.createElement('div');
        slot.className = 'answer-slot';
        slot.id = `slot-${i}`;
        slot.onclick = () => selectSlot(i);
        answerInput.appendChild(slot);
    }
    
    const selector = document.createElement('div');
    selector.className = 'number-selector';
    
    const maxNum = Math.max(...currentSequence);
    for (let i = 1; i <= maxNum + 2; i++) {
        const btn = document.createElement('button');
        btn.className = 'number-btn';
        btn.textContent = i;
        btn.onclick = () => selectNumber(i);
        selector.appendChild(btn);
    }
    
    answerInput.appendChild(selector);
    currentAnswer = new Array(currentSequence.length).fill(0);
    currentSlotIndex = 0;
}

function selectSlot(index) {
    currentSlotIndex = index;
    highlightSlot(index);
}

function selectNumber(num) {
    if (currentSlotIndex < currentAnswer.length) {
        currentAnswer[currentSlotIndex] = num;
        const slot = document.getElementById(`slot-${currentSlotIndex}`);
        slot.textContent = num;
        slot.classList.add('filled');
        currentSlotIndex = (currentSlotIndex + 1) % currentAnswer.length;
        highlightSlot(currentSlotIndex);
    }
}

function highlightSlot(index) {
    document.querySelectorAll('.answer-slot').forEach(slot => {
        slot.style.borderColor = '';
        slot.style.borderStyle = '';
    });
    
    const slot = document.getElementById(`slot-${index}`);
    if (slot) {
        slot.style.borderColor = '#667eea';
        slot.style.borderWidth = '3px';
        slot.style.borderStyle = 'solid';
    }
}

async function checkAnswer() {
    if (currentAnswer.some(val => val === 0)) {
        alert('Пожалуйста, заполните все поля!');
        return;
    }
    
    try {
        const url = `/api/game/${currentGameId}/check`;
        const response = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ answer: currentAnswer })
        });
        
        const data = await response.json();
        
        if (data.error) {
            alert('Ошибка: ' + data.error);
            return;
        }
        
        if (data.success === 'true' || data.success === true) {
            showResult(true, data.message, parseInt(data.score));
        } else {
            showResult(false, data.message, 0);
        }
        
        clearInterval(gameTimer);
    } catch (error) {
        console.error('Error:', error);
        alert('Ошибка при проверке ответа');
    }
}

function showResult(success, message, score) {
    const resultDiv = document.getElementById('resultMessage');
    resultDiv.className = `result-message ${success ? 'success' : 'error'}`;
    resultDiv.textContent = message + (score > 0 ? ` Очки: ${score}` : '');
    resultDiv.style.display = 'block';
    
    const currentScore = parseInt(document.getElementById('score').textContent) || 0;
    document.getElementById('score').textContent = currentScore + score;
}

function startGameTimer() {
    gameTimer = setInterval(() => {
        if (startTime) {
            const elapsed = Math.floor((Date.now() - startTime) / 1000);
            const timerEl = document.getElementById('timer');
            if (timerEl) timerEl.textContent = elapsed;
        }
    }, 1000);
}

function resetGame() {
    if (memorizationTimer) clearInterval(memorizationTimer);
    if (gameTimer) clearInterval(gameTimer);
    if (checkPairTimeout) clearTimeout(checkPairTimeout);
    
    currentGameId = null;
    currentGameType = null;
    currentSequence = [];
    currentAnswer = [];
    currentSlotIndex = 0;
    startTime = null;
    flippedCards = [];
    
    document.getElementById('gameSetup').style.display = 'block';
    document.getElementById('gameArea').style.display = 'none';
    document.getElementById('score').textContent = '0';
    const timerEl = document.getElementById('timer');
    if (timerEl) timerEl.textContent = '0';
    document.getElementById('resultMessage').style.display = 'none';
}

document.addEventListener('DOMContentLoaded', function() {
    console.log('DOM Content Loaded, attaching event handlers');
    
    const startBtn = document.getElementById('startGameBtn');
    if (startBtn) {
        startBtn.addEventListener('click', function(e) {
            e.preventDefault();
            console.log('Start game button clicked');
            startGame();
        });
        console.log('Start button handler attached');
    } else {
        console.error('Start button not found!');
    }
    
    const checkBtn = document.getElementById('checkAnswerBtn');
    if (checkBtn) {
        checkBtn.addEventListener('click', function(e) {
            e.preventDefault();
            checkAnswer();
        });
    }
    
    const resetBtns = document.querySelectorAll('#resetGameBtn, #resetGameBtn2');
    resetBtns.forEach(btn => {
        if (btn) {
            btn.addEventListener('click', function(e) {
                e.preventDefault();
                resetGame();
            });
        }
    });
    
    console.log('All event handlers attached');
});

window.startGame = startGame;
window.resetGame = resetGame;
window.checkAnswer = checkAnswer;
window.flipCard = flipCard;
window.selectSlot = selectSlot;
window.selectNumber = selectNumber;

console.log('Memory Trainer script loaded');
console.log('startGame function:', typeof startGame);
