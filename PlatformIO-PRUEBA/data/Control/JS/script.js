// Configuración de los bombillos
const bulbConfigs = [
    {
      id: 0,
      name: "Garaje",
      hasAutoMode: true,
      hasBrightness: true,
      description: "Control automático por sensor de luz ambiental. En modo automático, el brillo se ajusta según la luz ambiental.",
      icon: "🚗"
    },
    {
      id: 1,
      name: "Sala",
      hasAutoMode: false,
      hasBrightness: true,
      description: "Control manual con regulación de intensidad",
      icon: "🛋️"
    },
    {
      id: 2,
      name: "Cocina",
      hasAutoMode: false,
      hasBrightness: true,
      description: "Control de intensidad manual con alto brillo",
      icon: "🍳"
    },
    {
      id: 3,
      name: "Baño",
      hasAutoMode: false,
      hasBrightness: true,
      description: "Control manual con regulación de intensidad",
      icon: "🚿"
    },
    {
      id: 4,
      name: "Dormitorio",
      hasAutoMode: false,
      hasBrightness: true,
      description: "Control manual con regulación de intensidad",
      icon: "🛏️"
    }
  ];
  
  // Estado global de la aplicación
  const appState = {
    connection: false,
    lastUpdate: null,
    bulbs: {},
    isInitialized: false
  };
  
  // Cache de elementos del DOM
  const domElements = {
    bulbContainer: document.getElementById('bulbContainer'),
    connectionStatus: document.getElementById('connectionStatus'),
    connectionText: document.getElementById('connectionText'),
    lastUpdate: document.getElementById('lastUpdate'),
    notification: document.getElementById('notification')
  };
  
  // ======================
  // Funciones de utilidad
  // ======================
  
  /**
   * Muestra una notificación al usuario
   * @param {string} message - Mensaje a mostrar
   * @param {string} type - Tipo de notificación (error, success, warning)
   * @param {number} duration - Duración en milisegundos (opcional)
   */
  function showNotification(message, type = 'error', duration = 5000) {
    const notification = domElements.notification;
    notification.textContent = message;
    notification.className = 'notification show';
    notification.classList.add(type);
    
    // Limpiar notificación anterior
    clearTimeout(notification.timeout);
    
    // Ocultar después de la duración especificada
    notification.timeout = setTimeout(() => {
      notification.classList.remove('show', 'error', 'success', 'warning');
    }, duration);
  }
  
  /**
   * Actualiza el estado de conexión en la UI
   * @param {boolean} connected - Estado de conexión
   */
  function updateConnectionStatus(connected) {
    appState.connection = connected;
    const indicator = domElements.connectionStatus;
    const text = domElements.connectionText;
    
    if (connected) {
      indicator.classList.add('connected');
      text.textContent = 'Conectado al servidor';
      showNotification('Conexión establecida', 'success', 3000);
    } else {
      indicator.classList.remove('connected');
      text.textContent = 'Sin conexión';
    }
  }
  
  /**
   * Formatea la fecha para mostrarla
   * @param {Date} date - Fecha a formatear
   * @returns {string} Fecha formateada
   */
  function formatDateTime(date) {
    if (!date) return '--';
    return date.toLocaleTimeString() + ' ' + date.toLocaleDateString();
  }
  
  /**
   * Marca un bombillo como en estado de error
   * @param {number} bulbId - ID del bombillo
   */
  function markBulbError(bulbId) {
    const bulbElement = document.getElementById(`bulb-${bulbId}`);
    if (bulbElement) {
      bulbElement.classList.add('error');
      const bulbVisual = bulbElement.querySelector('.bulb');
      if (bulbVisual) bulbVisual.classList.add('error');
    }
  }
  
  // ==============================
  // Funciones de control de bombillos
  // ==============================
  
  /**
   * Controla un bombillo enviando comandos al servidor
   * @param {number} bulbId - ID del bombillo
   * @param {string} action - Acción a realizar (toggle, setBrightness, setMode)
   * @param {any} value - Valor asociado a la acción (opcional)
   * @returns {Promise} Promesa con la respuesta del servidor
   */
  async function controlBulb(bulbId, action, value = null) {
    // Mostrar estado de carga
    const bulbElement = document.getElementById(`bulb-${bulbId}`);
    if (bulbElement) bulbElement.classList.add('loading');
    
    const params = new URLSearchParams({
      id: bulbId,
      action: action
    });
    
    if (value !== null) {
      // Convertir porcentaje de brillo (0-100) a valor (0-255)
      if (action === 'setBrightness') {
        value = Math.round(value * 2.55);
      }
      params.append('value', value);
    }
    
    try {
      const response = await fetch(`/api/control?${params.toString()}`);
      
      if (!response.ok) {
        throw new Error(`Error HTTP: ${response.status}`);
      }
      
      const data = await response.json();
      
      // Convertir valor de brillo (0-255) a porcentaje (0-100)
      if (data.value !== undefined) {
        data.displayValue = Math.round((data.value / 255) * 100);
      }
      
      updateBulbUI(bulbId, data);
      updateConnectionStatus(true);
      appState.lastUpdate = new Date();
      domElements.lastUpdate.textContent = `Última actualización: ${formatDateTime(appState.lastUpdate)}`;
      
      return data;
    } catch (error) {
      console.error(`Error al ${action} bombillo ${bulbId}:`, error);
      updateConnectionStatus(false);
      showNotification(`Error al ${action} bombillo ${bulbId}: ${error.message}`);
      markBulbError(bulbId);
      throw error;
    } finally {
      if (bulbElement) bulbElement.classList.remove('loading');
    }
  }
  
  /**
   * Actualiza la interfaz de usuario de un bombillo
   * @param {number} bulbId - ID del bombillo
   * @param {object} state - Estado del bombillo
   */
  function updateBulbUI(bulbId, state) {
    appState.bulbs[bulbId] = state;
    const bulbElement = document.getElementById(`bulb-${bulbId}`);
    
    if (!bulbElement) return;
    
    bulbElement.classList.remove('error');
    
    // Actualizar bombilla visual
    updateBulbVisual(bulbElement, state);
    
    // Actualizar controles
    updateToggleControl(bulbElement, 'power-toggle', state.isOn);
    updateBrightnessControl(bulbElement, bulbId, state);
    updateAutoModeControl(bulbElement, bulbId, state);
  }
  
  /**
   * Actualiza la representación visual del bombillo
   * @param {HTMLElement} element - Elemento del bombillo
   * @param {object} state - Estado del bombillo
   */
  function updateBulbVisual(element, state) {
    const bulbVisual = element.querySelector('.bulb');
    if (!bulbVisual) return;
    
    bulbVisual.classList.remove('error');
    
    if (state.isOn) {
      bulbVisual.classList.add('on');
      const brightness = state.displayValue !== undefined ? 
                       state.displayValue : 
                       Math.round((state.value || 0) / 255 * 100);
      
      // Ajustar brillo visual
      const intensity = brightness / 100;
      bulbVisual.style.backgroundColor = `rgba(255, 255, ${Math.min(255, 180 * intensity)}, ${intensity * 0.8})`;
      bulbVisual.style.boxShadow = `0 0 ${30 + (brightness * 0.7)}px rgba(255, 255, 100, ${intensity * 0.6})`;
    } else {
      bulbVisual.classList.remove('on');
      bulbVisual.style.backgroundColor = 'var(--bulb-off)';
      bulbVisual.style.boxShadow = 'none';
    }
  }
  
  /**
   * Actualiza el control de encendido/apagado
   * @param {HTMLElement} element - Elemento del bombillo
   * @param {string} className - Clase del control
   * @param {boolean} isOn - Estado del bombillo
   */
  function updateToggleControl(element, className, isOn) {
    const toggle = element.querySelector(`.${className}`);
    if (toggle) toggle.checked = isOn;
  }
  
  /**
   * Actualiza el control de brillo
   * @param {HTMLElement} element - Elemento del bombillo
   * @param {number} bulbId - ID del bombillo
   * @param {object} state - Estado del bombillo
   */
  function updateBrightnessControl(element, bulbId, state) {
    const brightnessSlider = element.querySelector('.brightness-control');
    const brightnessValue = element.querySelector('.brightness-value');
    
    if (brightnessSlider && brightnessValue) {
      const percent = state.displayValue !== undefined ? 
                     state.displayValue : 
                     Math.round((state.value || 0) / 255 * 100);
      
      brightnessSlider.value = percent;
      brightnessSlider.disabled = !state.isOn || 
                                (bulbConfigs[bulbId].hasAutoMode && state.autoMode);
      
      brightnessValue.innerHTML = `
        <span>Brillo:</span>
        <span>${percent}%</span>
      `;
    }
  }
  
  /**
   * Actualiza el control de modo automático
   * @param {HTMLElement} element - Elemento del bombillo
   * @param {number} bulbId - ID del bombillo
   * @param {object} state - Estado del bombillo
   */
  function updateAutoModeControl(element, bulbId, state) {
    const config = bulbConfigs[bulbId];
    const autoToggle = element.querySelector('.auto-toggle');
    const autoModeInfo = element.querySelector('.auto-mode');
    
    if (autoToggle) {
      autoToggle.checked = state.autoMode;
      autoToggle.disabled = !config.hasAutoMode;
    }
    
    if (autoModeInfo) {
      autoModeInfo.style.display = (config.hasAutoMode && state.autoMode) ? 'block' : 'none';
    }
  }
  
  // ==============================
  // Manejadores de eventos
  // ==============================
  
  /**
   * Maneja el evento de encendido/apagado
   * @param {number} bulbId - ID del bombillo
   * @param {boolean} isOn - Nuevo estado
   */
  async function handleBulbToggle(bulbId, isOn) {
    try {
      await controlBulb(bulbId, isOn ? 'turnOn' : 'turnOff');
    } catch (error) {
      // Revertir el cambio en la UI si falla
      const bulb = document.getElementById(`bulb-${bulbId}`);
      if (bulb) {
        const toggle = bulb.querySelector('.power-toggle');
        if (toggle) toggle.checked = !isOn;
      }
    }
  }
  
  /**
   * Maneja el cambio de brillo
   * @param {number} bulbId - ID del bombillo
   * @param {number} value - Nuevo valor de brillo (0-100)
   */
  async function handleBrightnessChange(bulbId, value) {
    try {
      await controlBulb(bulbId, 'setBrightness', value);
    } catch (error) {
      showNotification('Error al ajustar brillo', 'error');
    }
  }
  
  /**
   * Maneja el cambio de modo automático
   * @param {number} bulbId - ID del bombillo
   * @param {boolean} autoMode - Nuevo modo
   */
  async function handleAutoModeToggle(bulbId, autoMode) {
    try {
      await controlBulb(bulbId, 'setMode', autoMode ? 'auto' : 'manual');
    } catch (error) {
      showNotification('Error al cambiar modo', 'error');
    }
  }
  
  // ==============================
  // Creación de la interfaz
  // ==============================
  
  /**
   * Crea los elementos HTML para cada bombillo
   */
  function createBulbElements() {
    domElements.bulbContainer.innerHTML = '';
    
    bulbConfigs.forEach(config => {
      const bulbElement = document.createElement('div');
      bulbElement.className = 'bulb-card';
      bulbElement.id = `bulb-${config.id}`;
      
      bulbElement.innerHTML = `
        <h3 class="bulb-title">${config.icon || '💡'} ${config.name}</h3>
        <div class="bulb-visual">
          <div class="bulb"></div>
          <div class="bulb-base"></div>
        </div>
        <div class="controls">
          ${createBulbControls(config)}
        </div>
      `;
      
      domElements.bulbContainer.appendChild(bulbElement);
    });
  }
  
  /**
   * Crea los controles HTML para un bombillo
   * @param {object} config - Configuración del bombillo
   * @returns {string} HTML de los controles
   */
  function createBulbControls(config) {
    let controlsHTML = '';
    
    if (config.hasAutoMode) {
      controlsHTML += `
        <div class="control-group">
          <label class="control-label">Modo Automático</label>
          <div class="toggle-container">
            <label class="toggle-switch">
              <input type="checkbox" class="auto-toggle" 
                onchange="handleAutoModeToggle(${config.id}, this.checked)"
                ${!config.hasAutoMode ? 'disabled' : ''}>
              <span class="slider"></span>
            </label>
            <div class="auto-mode" style="display: none;">
              ${config.description}
            </div>
          </div>
        </div>
      `;
    }
    
    controlsHTML += `
      <div class="control-group">
        <label class="control-label">Encendido</label>
        <div class="toggle-container">
          <label class="toggle-switch">
            <input type="checkbox" class="power-toggle" 
              onchange="handleBulbToggle(${config.id}, this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>
      <div class="control-group brightness-container">
        <label class="control-label">Intensidad</label>
        <input type="range" min="0" max="100" value="0" step="1"
          class="brightness-control brightness-slider"
          oninput="handleBrightnessChange(${config.id}, this.value)"
          onchange="handleBrightnessChange(${config.id}, this.value)">
        <div class="brightness-value">
          <span>Brillo:</span>
          <span>0%</span>
        </div>
      </div>
    `;
    
    return controlsHTML;
  }
  
  // ==============================
  // Funciones de inicialización
  // ==============================
  
  /**
   * Actualiza el estado de todos los bombillos
   */
  async function updateAllBulbsStatus() {
    if (!appState.connection) return;
    
    try {
      const promises = bulbConfigs.map(config => 
        controlBulb(config.id, 'getStatus')
          .catch(error => {
            console.error(`Error actualizando estado bombillo ${config.id}:`, error);
            return null;
          })
      );
      
      await Promise.all(promises);
      updateConnectionStatus(true);
    } catch (error) {
      console.error('Error actualizando estados:', error);
      updateConnectionStatus(false);
    }
  }
  
  /**
   * Inicializa la aplicación
   */
  async function initApp() {
    if (appState.isInitialized) return;
    appState.isInitialized = true;
    
    createBulbElements();
    
    try {
      // Obtener estado inicial de todos los bombillos
      await updateAllBulbsStatus();
      updateConnectionStatus(true);
      
      // Configurar actualización periódica para el garaje en modo automático
      setInterval(() => {
        if (bulbConfigs[0].hasAutoMode && appState.bulbs[0]?.autoMode) {
          controlBulb(0, 'getStatus').catch(console.error);
        }
      }, 1000);
      
      // Actualizar todos los bombillos periódicamente
      setInterval(updateAllBulbsStatus, 10000);
      
      showNotification('Aplicación iniciada correctamente', 'success', 3000);
    } catch (error) {
      console.error('Error inicializando aplicación:', error);
      showNotification('Error al conectar con el servidor', 'error');
      updateConnectionStatus(false);
    }
  }
  
  // Iniciar la aplicación cuando el DOM esté listo
  document.addEventListener('DOMContentLoaded', initApp);
  
  // Hacer funciones accesibles globalmente para los eventos en línea
  window.handleBulbToggle = handleBulbToggle;
  window.handleBrightnessChange = handleBrightnessChange;
  window.handleAutoModeToggle = handleAutoModeToggle;