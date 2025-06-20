let gateway = `ws://${window.location.hostname}/ws`;
let websocket;

// --- WebSocket Initialization ---
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    document.getElementById('ws-status').innerHTML = 'Connected';
}

function onClose(event) {
    console.log('Connection closed');
    document.getElementById('ws-status').innerHTML = 'Disconnected';
    setTimeout(initWebSocket, 2000); // Try to reconnect every 2 seconds
}

// --- Message Handling ---
function onMessage(event) {
    let data = JSON.parse(event.data);
    console.log('Received from server:', data);

    if (data.action === 'update') {
        updateUI(data);
    }
}

// --- UI Update Logic ---
function updateUI(data) {
    // Device Name
    document.getElementById('deviceName').innerText = data.deviceName |
| 'LED Bar Control';

    // LED Switch
    document.getElementById('ledSwitch').checked = data.ledState;

    // Brightness Slider
    const slider = document.getElementById('brightnessSlider');
    slider.value = data.brightness;
    document.getElementById('brightnessValue').innerText = data.brightness;
    slider.disabled =!data.ledState;

    // Timer Switch and Settings
    const timerSwitch = document.getElementById('timerSwitch');
    timerSwitch.checked = data.timerEnabled;
    const timerSettings = document.getElementById('timerSettings');
    if (data.timerEnabled) {
        timerSettings.classList.remove('d-none');
    } else {
        timerSettings.classList.add('d-none');
    }

    // Format time as HH:MM for input fields
    const formatTime = (h, m) => `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
    document.getElementById('onTime').value = formatTime(data.onHour, data.onMinute);
    document.getElementById('offTime').value = formatTime(data.offHour, data.offMinute);
}

// --- Event Listeners ---
function setupEventListeners() {
    // LED Power Switch
    document.getElementById('ledSwitch').addEventListener('change', (event) => {
        sendWebSocket({ action: 'toggle', state: event.target.checked });
    });

    // Brightness Slider
    const brightnessSlider = document.getElementById('brightnessSlider');
    brightnessSlider.addEventListener('input', (event) => {
        document.getElementById('brightnessValue').innerText = event.target.value;
    });
    brightnessSlider.addEventListener('change', (event) => {
        sendWebSocket({ action: 'brightness', value: parseInt(event.target.value) });
    });

    // Timer Enable Switch
    document.getElementById('timerSwitch').addEventListener('change', (event) => {
        const timerSettings = document.getElementById('timerSettings');
        if (event.target.checked) {
            timerSettings.classList.remove('d-none');
        } else {
            timerSettings.classList.add('d-none');
            // Send disable command immediately
            sendWebSocket({ action: 'updateTimer', enabled: false });
        }
    });

    // Save Timer Button
    document.getElementById('saveTimerBtn').addEventListener('click', () => {
        const onTime = document.getElementById('onTime').value.split(':');
        const offTime = document.getElementById('offTime').value.split(':');
        sendWebSocket({
            action: 'updateTimer',
            enabled: document.getElementById('timerSwitch').checked,
            onHour: parseInt(onTime),
            onMinute: parseInt(onTime),
            offHour: parseInt(offTime),
            offMinute: parseInt(offTime)
        });
        alert('Schedule saved!');
    });
}

// --- Send Data to Server ---
function sendWebSocket(data) {
    if (websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(data));
    }
}

// --- Page Load ---
window.addEventListener('load', () => {
    initWebSocket();
    setupEventListeners();
});