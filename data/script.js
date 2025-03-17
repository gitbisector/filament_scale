// DOM Elements
const weightDisplay = document.getElementById('weight');
const filamentDisplay = document.getElementById('filament-weight');
const statusDisplay = document.getElementById('status');
const tareButton = document.getElementById('tare');
const calibrateButton = document.getElementById('calibrate');
const toggleUpdatesButton = document.getElementById('toggle-updates');
const vesselsList = document.getElementById('vessels-list');
const addVesselButton = document.getElementById('add-vessel');
const vesselModal = document.getElementById('vessel-modal');
const vesselForm = document.getElementById('vessel-form');
const modalClose = document.getElementById('modal-close');
const modalTitle = document.getElementById('modal-title');

// Debug check for elements
console.log('Elements found:', {
    weightDisplay: !!weightDisplay,
    filamentDisplay: !!filamentDisplay,
    statusDisplay: !!statusDisplay,
    tareButton: !!tareButton,
    calibrateButton: !!calibrateButton,
    toggleUpdatesButton: !!toggleUpdatesButton,
    vesselsList: !!vesselsList,
    addVesselButton: !!addVesselButton,
    vesselModal: !!vesselModal,
    vesselForm: !!vesselForm,
    modalClose: !!modalClose
});

// WebSocket connection
let ws = null;
let reconnectInterval = null;
let reconnectAttempts = 0;
const MAX_RECONNECT_ATTEMPTS = 5;
const INITIAL_RECONNECT_DELAY = 2000;
let currentReconnectDelay = INITIAL_RECONNECT_DELAY;
let updatesEnabled = false;
const wsUrl = `ws://${window.location.hostname}/ws`;

function connectWebSocket() {
    if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) {
        return;
    }
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
        console.log('WebSocket connected');
        statusDisplay.textContent = 'Connected';
        statusDisplay.style.color = '#27ae60';
        reconnectAttempts = 0;
        currentReconnectDelay = INITIAL_RECONNECT_DELAY;
        if (reconnectInterval) {
            clearInterval(reconnectInterval);
            reconnectInterval = null;
        }
        // Small delay to ensure server is ready
        setTimeout(() => {
            // Request vessel list on connection
            console.log('Requesting vessel list');
            try {
                ws.send(JSON.stringify({ command: 'getVessels' }));
            } catch (e) {
                console.error('Failed to request vessel list:', e);
                statusDisplay.textContent = 'Failed to load vessels';
                statusDisplay.style.color = '#e74c3c';
            }
            // Restore updates state if it was enabled
            if (updatesEnabled) {
                console.log('Restoring updates state:', updatesEnabled);
                try {
                    ws.send(JSON.stringify({ command: 'toggleUpdates', enabled: updatesEnabled }));
                } catch (e) {
                    console.error('Failed to restore updates state:', e);
                }
            }
        }, 500); // 500ms delay
    };
    
    ws.onclose = () => {
        console.log('WebSocket disconnected');
        statusDisplay.textContent = 'Disconnected - Reconnecting...';
        statusDisplay.style.color = '#e74c3c';
        
        if (!reconnectInterval) {
            if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                reconnectInterval = setTimeout(() => {
                    reconnectInterval = null;
                    reconnectAttempts++;
                    currentReconnectDelay *= 1.5; // Exponential backoff
                    connectWebSocket();
                }, currentReconnectDelay);
                console.log(`Reconnecting in ${currentReconnectDelay}ms (attempt ${reconnectAttempts + 1}/${MAX_RECONNECT_ATTEMPTS})`);
            } else {
                statusDisplay.textContent = 'Connection failed - Please refresh page';
                console.log('Max reconnection attempts reached');
            }
        }
    };
    
    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            console.log('Received message:', data);
            
            if (updatesEnabled) {
                if (data.weight !== undefined) {
                    weightDisplay.textContent = `Total: ${data.weight.toFixed(2)}g`;
                }
                if (data.filamentWeight !== undefined) {
                    filamentDisplay.textContent = `Filament: ${data.filamentWeight.toFixed(2)}g`;
                } else {
                    filamentDisplay.textContent = 'No vessel selected';
                }

                // Update selected vessel in UI
                if (data.selectedVessel !== undefined) {
                    updateSelectedVessel(data.selectedVessel);
                }
            }
            
            if (data.vessels) {
                updateVesselsList(data.vessels);
            }
            
            if (data.status) {
                statusDisplay.textContent = data.status;
                console.log('Status update:', data.status);
                // Refresh vessel list after successful operations
                if (data.status.includes('Vessel')) {
                    ws.send(JSON.stringify({ command: 'getVessels' }));
                }
            }
        } catch (e) {
            console.error('Error parsing message:', e);
        }
    };
    
    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        statusDisplay.textContent = 'Connection error';
        statusDisplay.style.color = '#e74c3c';
    };
}

// Toggle real-time updates
toggleUpdatesButton.addEventListener('click', () => {
    updatesEnabled = !updatesEnabled;
    console.log('Toggling updates:', updatesEnabled);
    
    toggleUpdatesButton.textContent = updatesEnabled ? 'Stop Updates' : 'Start Updates';
    toggleUpdatesButton.classList.toggle('active', updatesEnabled);
    
    if (ws && ws.readyState === WebSocket.OPEN) {
        const message = { command: 'toggleUpdates', enabled: updatesEnabled };
        console.log('Sending toggle message:', message);
        ws.send(JSON.stringify(message));
    }
});

// Vessel List Management
function updateVesselsList(vessels) {
    console.log('Updating vessels list:', vessels);
    vesselsList.innerHTML = '';
    vessels.forEach((vessel, index) => {
        const vesselElement = document.createElement('div');
        vesselElement.className = 'vessel-item';
        
        vesselElement.innerHTML = `
            <div class="vessel-info">
                <h3>${vessel.name}</h3>
                <p>Vessel: ${vessel.vesselWeight}g | Spool: ${vessel.spoolWeight}g</p>
            </div>
            <div class="vessel-actions">
                <button onclick="selectVessel(${index})" class="button">Select</button>
                <button onclick="editVessel(${index})" class="button">Edit</button>
                <button onclick="deleteVessel(${index})" class="button">Delete</button>
            </div>
        `;
        vesselsList.appendChild(vesselElement);
    });
}

function updateSelectedVessel(index) {
    // Remove selected class from all vessels
    const vessels = vesselsList.getElementsByClassName('vessel-item');
    Array.from(vessels).forEach(vessel => {
        vessel.classList.remove('selected');
    });

    // Add selected class to current vessel
    if (index >= 0 && index < vessels.length) {
        vessels[index].classList.add('selected');
    }
}

function selectVessel(index) {
    console.log('Selecting vessel:', index);
    ws.send(JSON.stringify({
        command: 'selectVessel',
        index: index
    }));
}

// Modal Management
function showModal(isEdit = false, vesselData = null) {
    console.log('Showing modal:', { isEdit, vesselData });
    modalTitle.textContent = isEdit ? 'Edit Vessel' : 'Add Vessel';
    vesselForm.reset();
    document.getElementById('vessel-index').value = isEdit ? vesselData.index : '';
    document.getElementById('vessel-name').value = isEdit ? vesselData.name : '';
    document.getElementById('vessel-weight').value = isEdit ? vesselData.vesselWeight : '';
    document.getElementById('spool-weight').value = isEdit ? vesselData.spoolWeight : '';
    vesselModal.style.display = 'block';
}

function hideModal() {
    console.log('Hiding modal');
    vesselModal.style.display = 'none';
}

// Vessel CRUD Operations
function editVessel(index) {
    console.log('Editing vessel:', index);
    const vesselElement = vesselsList.children[index];
    const name = vesselElement.querySelector('h3').textContent;
    const weights = vesselElement.querySelector('p').textContent.match(/\d+(\.\d+)?/g);
    
    showModal(true, {
        index,
        name,
        vesselWeight: parseFloat(weights[0]),
        spoolWeight: parseFloat(weights[1])
    });
}

function deleteVessel(index) {
    console.log('Deleting vessel:', index);
    if (confirm('Are you sure you want to delete this vessel?')) {
        ws.send(JSON.stringify({
            command: 'deleteVessel',
            index
        }));
    }
}

// Event Listeners
tareButton.addEventListener('click', () => {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ command: 'tare' }));
        statusDisplay.textContent = 'Taring...';
    }
});

calibrateButton.addEventListener('click', () => {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const weight = prompt('Enter calibration weight in grams:', '100');
        if (weight) {
            ws.send(JSON.stringify({
                command: 'calibrate',
                weight: parseFloat(weight)
            }));
        }
    }
});

// Make these functions globally accessible
window.editVessel = editVessel;
window.deleteVessel = deleteVessel;

// Vessel Form Submission
vesselForm.addEventListener('submit', (event) => {
    event.preventDefault();
    const index = document.getElementById('vessel-index').value;
    const name = document.getElementById('vessel-name').value;
    const vesselWeight = parseFloat(document.getElementById('vessel-weight').value);
    const spoolWeight = parseFloat(document.getElementById('spool-weight').value);

    const data = {
        command: index ? 'updateVessel' : 'addVessel',
        index: index ? parseInt(index) : undefined,
        vessel: {
            name,
            vesselWeight,
            spoolWeight
        }
    };

    console.log('Submitting vessel form:', data);
    ws.send(JSON.stringify(data));
    hideModal();
});

// Add Vessel Button
addVesselButton.addEventListener('click', () => {
    showModal(false);
});

// Modal Close
modalClose.addEventListener('click', hideModal);

// Initialize WebSocket connection
connectWebSocket();