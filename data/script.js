let weightElement = document.getElementById('weight');
let statusElement = document.getElementById('status');
let tareButton = document.getElementById('tare');
let calibrateButton = document.getElementById('calibrate');

// WebSocket connection
let ws = null;
const connectWebSocket = () => {
    ws = new WebSocket(`ws://${window.location.hostname}/ws`);
    
    ws.onopen = () => {
        statusElement.textContent = 'Connected';
        statusElement.style.color = '#27ae60';
    };

    ws.onclose = () => {
        statusElement.textContent = 'Disconnected - Reconnecting...';
        statusElement.style.color = '#e74c3c';
        setTimeout(connectWebSocket, 2000);
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            if (data.weight !== undefined) {
                weightElement.textContent = data.weight.toFixed(2);
            }
            if (data.status) {
                statusElement.textContent = data.status;
            }
        } catch (e) {
            console.error('Error parsing message:', e);
        }
    };
};

// Button handlers
tareButton.addEventListener('click', () => {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ command: 'tare' }));
        statusElement.textContent = 'Taring...';
    }
});

calibrateButton.addEventListener('click', () => {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ command: 'calibrate' }));
        statusElement.textContent = 'Calibrating...';
    }
});

// Initial connection
connectWebSocket();