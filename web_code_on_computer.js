// รันบน vs code ด้วย Node. โดยต้องติดตั้ง Node.js ก่อน
// ใช้คําสั่ง npm start บน terminal ของ vs code


import React, { useState, useEffect, useRef, useCallback } from 'react';
import { Play, Square, Wifi, WifiOff, Settings, Map, Satellite, Activity, Thermometer, Navigation, MapPin, Signal, Search, RefreshCw, Crosshair, Move } from 'lucide-react';

const CanSatDashboard = () => {
  // Connection timeout tracking
  const connectionTimeouts = useRef({});
  const CONNECTION_TIMEOUT = 10000; // 10 seconds

  // Real-time Map state
  const [mapCenter, setMapCenter] = useState([13.736717, 100.523186]); // Default to Bangkok
  const [mapZoom, setMapZoom] = useState(10);
  const [showSatelliteView, setShowSatelliteView] = useState(false);
  const [gpsHistory, setGpsHistory] = useState([]);
  const [followGPS, setFollowGPS] = useState(true);
  const mapRef = useRef(null);
  const markerRef = useRef(null);
  const pathRef = useRef(null);

  // Auto-discovery state
  const [isScanning, setIsScanning] = useState(false);
  const [discoveredDevices, setDiscoveredDevices] = useState([]);
  const [scanProgress, setScanProgress] = useState(0);
  const [lastScanTime, setLastScanTime] = useState(null);

  // Load saved IPs from localStorage
  const loadSavedIPs = () => {
    try {
      const saved = localStorage.getItem('cansat_esp32_ips');
      if (saved) {
        const parsedIPs = JSON.parse(saved);
        return [
          { id: 1, ip: parsedIPs.esp1 || '192.168.1.100', isConnected: false, isConnecting: false, lastReceived: 'Never', status: 'offline', autoReconnect: false, deviceName: '' },
          { id: 2, ip: parsedIPs.esp2 || '192.168.1.101', isConnected: false, isConnecting: false, lastReceived: 'Never', status: 'offline', autoReconnect: false, deviceName: '' }
        ];
      }
    } catch (error) {
      console.error('Failed to load saved IPs:', error);
    }
    return [
      { id: 1, ip: '192.168.1.100', isConnected: false, isConnecting: false, lastReceived: 'Never', status: 'offline', autoReconnect: false, deviceName: '' },
      { id: 2, ip: '192.168.1.101', isConnected: false, isConnecting: false, lastReceived: 'Never', status: 'offline', autoReconnect: false, deviceName: '' }
    ];
  };

  // State for ESP32 connections (Multiple receivers)
  const [espConnections, setEspConnections] = useState(loadSavedIPs());
  const [showConnectionModal, setShowConnectionModal] = useState(false);
  const [editingConnection, setEditingConnection] = useState(null);

  // Global last received timestamp
  const [globalLastReceived, setGlobalLastReceived] = useState('Never');

  // State for sensor data
  const [sensorData, setSensorData] = useState({
    gps: {
      valid: false,
      latitude: '0',
      longitude: '0',
      altitude: '0',
      satellites: '0',
      speed: '0'
    },
    accelerometer: { x: '0', y: '0', z: '0' },
    gyroscope: { x: '0', y: '0', z: '0' },
    bmp280: {
      temperature: '--',
      pressure: '--',
      altitude: '--'
    },
    motion: {
      zDirection: 'UNKNOWN',
      totalMagnitude: '0'
    },
    packet: '0',
    counter: '0',
    rawData: 'Waiting for data...',
    source_esp_id: null,
    source_esp_ip: null
  });

  // Recording state
  const [isRecording, setIsRecording] = useState(false);
  const [recordedData, setRecordedData] = useState([]);
  const [startTime, setStartTime] = useState(null);

  // Modern clean styles
  const styles = {
    container: {
      minHeight: '100vh',
      background: '#f8fafc',
      fontFamily: "'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif",
      color: '#1e293b'
    },
    header: {
      background: 'white',
      borderBottom: '1px solid #e2e8f0',
      padding: '20px',
      boxShadow: '0 1px 3px 0 rgba(0, 0, 0, 0.1)'
    },
    headerContent: {
      maxWidth: '1200px',
      margin: '0 auto'
    },
    title: {
      fontSize: '1.875rem',
      fontWeight: '700',
      color: '#1e293b',
      marginBottom: '8px',
      display: 'flex',
      alignItems: 'center',
      gap: '12px'
    },
    subtitle: {
      color: '#64748b',
      fontSize: '1rem',
      marginBottom: '20px'
    },
    statusBar: {
      display: 'flex',
      gap: '16px',
      marginBottom: '20px',
      flexWrap: 'wrap'
    },
    statusChip: {
      display: 'flex',
      alignItems: 'center',
      gap: '8px',
      padding: '8px 16px',
      borderRadius: '20px',
      fontSize: '0.875rem',
      fontWeight: '500'
    },
    statusOnline: {
      background: '#dcfce7',
      color: '#166534',
      border: '1px solid #bbf7d0'
    },
    statusOffline: {
      background: '#fef2f2',
      color: '#dc2626',
      border: '1px solid #fecaca'
    },
    statusConnecting: {
      background: '#fef3c7',
      color: '#92400e',
      border: '1px solid #fcd34d'
    },
    statusNeutral: {
      background: '#f1f5f9',
      color: '#475569',
      border: '1px solid #e2e8f0'
    },
    statusTimeout: {
      background: '#fef3c7',
      color: '#92400e',
      border: '1px solid #fcd34d'
    },
    statusScanning: {
      background: '#f0f9ff',
      color: '#0369a1',
      border: '1px solid #0ea5e9'
    },
    controls: {
      display: 'flex',
      gap: '12px',
      flexWrap: 'wrap'
    },
    button: {
      display: 'flex',
      alignItems: 'center',
      gap: '8px',
      padding: '10px 20px',
      borderRadius: '8px',
      border: 'none',
      fontSize: '0.875rem',
      fontWeight: '500',
      cursor: 'pointer',
      transition: 'all 0.2s ease'
    },
    buttonPrimary: {
      background: '#3b82f6',
      color: 'white'
    },
    buttonSuccess: {
      background: '#10b981',
      color: 'white'
    },
    buttonDanger: {
      background: '#ef4444',
      color: 'white'
    },
    buttonSecondary: {
      background: '#f1f5f9',
      color: '#475569',
      border: '1px solid #e2e8f0'
    },
    main: {
      maxWidth: '1200px',
      margin: '0 auto',
      padding: '24px 20px'
    },
    grid: {
      display: 'grid',
      gridTemplateColumns: 'repeat(auto-fit, minmax(300px, 1fr))',
      gap: '24px',
      marginBottom: '24px'
    },
    card: {
      background: 'white',
      borderRadius: '12px',
      border: '1px solid #e2e8f0',
      overflow: 'hidden',
      boxShadow: '0 1px 3px 0 rgba(0, 0, 0, 0.1)'
    },
    cardHeader: {
      padding: '20px 24px 16px 24px',
      borderBottom: '1px solid #f1f5f9'
    },
    cardTitle: {
      fontSize: '1.125rem',
      fontWeight: '600',
      color: '#1e293b',
      display: 'flex',
      alignItems: 'center',
      gap: '8px',
      marginBottom: '4px'
    },
    cardSubtitle: {
      fontSize: '0.875rem',
      color: '#64748b'
    },
    cardBody: {
      padding: '20px 24px'
    },
    dataGrid: {
      display: 'grid',
      gridTemplateColumns: 'repeat(auto-fit, minmax(120px, 1fr))',
      gap: '16px'
    },
    dataItem: {
      textAlign: 'center'
    },
    dataValue: {
      fontSize: '1.5rem',
      fontWeight: '700',
      color: '#1e293b',
      marginBottom: '4px'
    },
    dataLabel: {
      fontSize: '0.75rem',
      color: '#64748b',
      textTransform: 'uppercase',
      letterSpacing: '0.05em',
      fontWeight: '500'
    },
    statusGood: {
      color: '#059669'
    },
    statusBad: {
      color: '#dc2626'
    },
    statusWarning: {
      color: '#d97706'
    },
    mapContainer: {
      gridColumn: '1 / -1',
      minHeight: '400px',
      background: '#f8fafc',
      borderRadius: '8px',
      border: '1px solid #e2e8f0',
      position: 'relative',
      overflow: 'hidden'
    },
    mapControls: {
      position: 'absolute',
      top: '12px',
      right: '12px',
      zIndex: 500,
      display: 'flex',
      gap: '8px'
    },
    mapButton: {
      padding: '8px 12px',
      background: 'white',
      border: '1px solid #e2e8f0',
      borderRadius: '6px',
      fontSize: '0.75rem',
      cursor: 'pointer',
      boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
      display: 'flex',
      alignItems: 'center',
      gap: '4px'
    },
    mapStats: {
      position: 'absolute',
      bottom: '12px',
      left: '12px',
      zIndex: 500,
      background: 'rgba(255,255,255,0.95)',
      border: '1px solid #e2e8f0',
      borderRadius: '6px',
      padding: '8px 12px',
      fontSize: '0.75rem',
      color: '#374151',
      backdropFilter: 'blur(4px)'
    },
    settingsModal: {
      position: 'fixed',
      top: '50%',
      left: '50%',
      transform: 'translate(-50%, -50%)',
      background: 'white',
      borderRadius: '12px',
      padding: '32px',
      boxShadow: '0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 10px 10px -5px rgba(0, 0, 0, 0.04)',
      zIndex: 10000,
      minWidth: '600px',
      maxWidth: '90vw',
      maxHeight: '90vh',
      overflow: 'auto',
      border: '1px solid #e2e8f0'
    },
    settingsOverlay: {
      position: 'fixed',
      top: 0,
      left: 0,
      width: '100%',
      height: '100%',
      background: 'rgba(0, 0, 0, 0.5)',
      zIndex: 9999
    },
    input: {
      width: '100%',
      padding: '14px 20px',
      border: '1px solid #d1d5db',
      borderRadius: '10px',
      fontSize: '1rem',
      marginBottom: '20px',
      transition: 'border-color 0.2s ease',
      outline: 'none',
      fontFamily: 'inherit',
      boxSizing: 'border-box'
    },
    progressBar: {
      width: '100%',
      height: '6px',
      background: '#e2e8f0',
      borderRadius: '3px',
      overflow: 'hidden',
      marginBottom: '16px'
    },
    progressFill: {
      height: '100%',
      background: '#3b82f6',
      borderRadius: '3px',
      transition: 'width 0.3s ease'
    },
    discoveredDevice: {
      display: 'flex',
      justifyContent: 'space-between',
      alignItems: 'center',
      padding: '12px',
      background: '#f8fafc',
      borderRadius: '8px',
      border: '1px solid #e2e8f0',
      marginBottom: '8px'
    },
    recordingBadge: {
      position: 'fixed',
      top: '20px',
      right: '20px',
      background: '#ef4444',
      color: 'white',
      padding: '8px 16px',
      borderRadius: '20px',
      fontSize: '0.875rem',
      fontWeight: '500',
      display: 'flex',
      alignItems: 'center',
      gap: '8px',
      zIndex: 100,
      boxShadow: '0 4px 6px -1px rgba(0, 0, 0, 0.1)'
    },
    terminalCard: {
      gridColumn: '1 / -1'
    },
    terminal: {
      background: '#1e293b',
      color: '#e2e8f0',
      padding: '16px',
      borderRadius: '8px',
      fontFamily: "'JetBrains Mono', 'Fira Code', monospace",
      fontSize: '0.875rem',
      lineHeight: '1.5',
      overflow: 'auto'
    }
  };

  // Initialize Leaflet Map
  useEffect(() => {
    // Initialize map function
    const initMap = () => {
      try {
        if (!mapRef.current || !window.L) return;

        // Clean up existing map
        if (window.cansatMap) {
          window.cansatMap.remove();
          window.cansatMap = null;
        }

        // Create map
        const map = window.L.map(mapRef.current, {
          center: mapCenter,
          zoom: mapZoom,
          zoomControl: false
        });

        // Add tile layers
        const osmLayer = window.L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
          attribution: '© OpenStreetMap contributors',
          maxZoom: 19
        });

        const satelliteLayer = window.L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
          attribution: '© Esri, DigitalGlobe, GeoEye',
          maxZoom: 19
        });

        // Add default layer
        (showSatelliteView ? satelliteLayer : osmLayer).addTo(map);

        // Store references
        window.cansatMap = map;
        window.cansatLayers = { osm: osmLayer, satellite: satelliteLayer };

        // Create custom marker
        const createMarkerIcon = () => {
          return window.L.divIcon({
            html: `<div class="cansat-marker">
              <div class="cansat-marker-pulse"></div>
              <div class="cansat-marker-dot"></div>
            </div>`,
            className: 'custom-cansat-marker',
            iconSize: [20, 20],
            iconAnchor: [10, 10]
          });
        };

        // Initialize marker (hidden until GPS is available)
        markerRef.current = window.L.marker(mapCenter, { 
          icon: createMarkerIcon(),
          opacity: 0 
        });
        
        // Create path for GPS history
        pathRef.current = window.L.polyline([], {
          color: '#3b82f6',
          weight: 3,
          opacity: 0.7,
          smoothFactor: 1
        }).addTo(map);

        // Add zoom control
        window.L.control.zoom({ position: 'topleft' }).addTo(map);

        console.log('Map initialized successfully');
        
      } catch (error) {
        console.error('Error initializing map:', error);
      }
    };

    // Check if Leaflet is already loaded
    if (window.L) {
      initMap();
      return;
    }

    // Load Leaflet CSS
    const loadCSS = () => {
      return new Promise((resolve) => {
        if (document.querySelector('link[href*="leaflet"]')) {
          resolve();
          return;
        }
        
        const css = document.createElement('link');
        css.rel = 'stylesheet';
        css.href = 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/leaflet.css';
        css.onload = resolve;
        css.onerror = resolve; // Continue even if CSS fails
        document.head.appendChild(css);
      });
    };

    // Load Leaflet JS
    const loadJS = () => {
      return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/leaflet.min.js';
        script.onload = resolve;
        script.onerror = reject;
        document.head.appendChild(script);
      });
    };

    // Load resources sequentially
    const loadMap = async () => {
      try {
        await loadCSS();
        await loadJS();
        // Give a small delay to ensure Leaflet is fully loaded
        setTimeout(initMap, 100);
      } catch (error) {
        console.error('Error loading Leaflet:', error);
        // Show fallback message or create simple map placeholder
        if (mapRef.current) {
          mapRef.current.innerHTML = `
            <div style="
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100%;
              color: #64748b;
              text-align: center;
              flex-direction: column;
              gap: 12px;
            ">
              <div style="font-size: 2rem;">🗺️</div>
              <div>Map loading failed</div>
              <div style="font-size: 0.875rem;">GPS coordinates will still be displayed below</div>
            </div>
          `;
        }
      }
    };

    loadMap();

    // Cleanup
    return () => {
      if (window.cansatMap) {
        try {
          window.cansatMap.remove();
          window.cansatMap = null;
        } catch (error) {
          console.error('Error cleaning up map:', error);
        }
      }
    };
  }, []); // Remove dependencies to prevent re-initialization

  // Update map when GPS data changes
  useEffect(() => {
    if (!window.cansatMap || !window.L || !sensorData.gps.valid) return;

    try {
      const lat = parseFloat(sensorData.gps.latitude);
      const lng = parseFloat(sensorData.gps.longitude);
      
      if (isNaN(lat) || isNaN(lng) || lat === 0 || lng === 0) return;

      const newPosition = [lat, lng];
      
      // Update marker position and make it visible
      if (markerRef.current) {
        markerRef.current.setLatLng(newPosition);
        markerRef.current.setOpacity(1);
        
        if (!window.cansatMap.hasLayer(markerRef.current)) {
          markerRef.current.addTo(window.cansatMap);
        }
      }

      // Update GPS history
      setGpsHistory(prev => {
        const now = Date.now();
        const newPoint = { lat, lng, timestamp: now };
        
        // Add new point if it's different enough from the last one (avoid duplicates)
        const shouldAdd = prev.length === 0 || 
          Math.abs(prev[prev.length - 1].lat - lat) > 0.0001 || 
          Math.abs(prev[prev.length - 1].lng - lng) > 0.0001;
          
        if (!shouldAdd) return prev;
        
        const newHistory = [...prev, newPoint];
        const limitedHistory = newHistory.slice(-100); // Keep last 100 points
        
        // Update path
        if (pathRef.current && limitedHistory.length > 1) {
          try {
            pathRef.current.setLatLngs(limitedHistory.map(point => [point.lat, point.lng]));
          } catch (error) {
            console.error('Error updating path:', error);
          }
        }
        
        return limitedHistory;
      });

      // Auto-follow GPS if enabled
      if (followGPS) {
        const currentZoom = window.cansatMap.getZoom();
        const targetZoom = Math.max(currentZoom, 15);
        
        window.cansatMap.setView(newPosition, targetZoom, {
          animate: true,
          duration: 1
        });
      }
      
    } catch (error) {
      console.error('Error updating GPS on map:', error);
    }
  }, [sensorData.gps.latitude, sensorData.gps.longitude, sensorData.gps.valid, followGPS]);

  // Toggle satellite view
  const toggleSatelliteView = () => {
    if (!window.cansatMap || !window.cansatLayers) return;
    
    try {
      const newSatelliteView = !showSatelliteView;
      setShowSatelliteView(newSatelliteView);
      
      // Remove current tile layers
      window.cansatMap.eachLayer((layer) => {
        if (layer._url && (layer._url.includes('openstreetmap') || layer._url.includes('arcgisonline'))) {
          window.cansatMap.removeLayer(layer);
        }
      });
      
      // Add new layer
      const layerToAdd = newSatelliteView ? window.cansatLayers.satellite : window.cansatLayers.osm;
      layerToAdd.addTo(window.cansatMap);
      
    } catch (error) {
      console.error('Error toggling satellite view:', error);
    }
  };

  // Center map on GPS
  const centerOnGPS = () => {
    if (!window.cansatMap || !sensorData.gps.valid) return;
    
    try {
      const lat = parseFloat(sensorData.gps.latitude);
      const lng = parseFloat(sensorData.gps.longitude);
      
      if (!isNaN(lat) && !isNaN(lng) && lat !== 0 && lng !== 0) {
        window.cansatMap.setView([lat, lng], 18, {
          animate: true,
          duration: 1
        });
      }
    } catch (error) {
      console.error('Error centering on GPS:', error);
    }
  };

  // Clear GPS history
  const clearGPSHistory = () => {
    try {
      setGpsHistory([]);
      if (pathRef.current) {
        pathRef.current.setLatLngs([]);
      }
    } catch (error) {
      console.error('Error clearing GPS history:', error);
    }
  };

  // Auto-discovery functions
  const scanForDevices = async () => {
    setIsScanning(true);
    setScanProgress(0);
    setDiscoveredDevices([]);
    
    const baseIP = '192.168.1.'; // หรือ subnet ที่คุณใช้
    const startRange = 100;
    const endRange = 150;
    const totalIPs = endRange - startRange + 1;
    const found = [];

    for (let i = startRange; i <= endRange; i++) {
      const ip = baseIP + i;
      const progress = ((i - startRange + 1) / totalIPs) * 100;
      setScanProgress(progress);

      try {
        // ใช้ AbortController เพื่อ timeout
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 2000);

        const response = await fetch(`http://${ip}/data`, {
          method: 'GET',
          signal: controller.signal,
          headers: {
            'Accept': 'application/json',
          }
        });

        clearTimeout(timeoutId);

        if (response.ok) {
          const data = await response.json();
          
          // ตรวจสอบว่าเป็น CanSat device จริงๆ
          if (data && (data.gps || data.accelerometer || data.packet !== undefined)) {
            found.push({
              ip: ip,
              deviceName: data.deviceName || `CanSat-${ip.split('.').pop()}`,
              lastPacket: data.packet || 'N/A',
              signal: 'Good',
              discovered: new Date().toLocaleTimeString()
            });
          }
        }
      } catch (error) {
        // Ignore errors (device not found or timeout)
      }
    }

    setDiscoveredDevices(found);
    setIsScanning(false);
    setScanProgress(100);
    setLastScanTime(new Date().toLocaleTimeString());
  };

  // Enhanced connection with IP validation (Removed smart reconnect)
  const validateAndConnect = async (ip) => {
    try {
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 3000);

      const response = await fetch(`http://${ip}/data`, {
        method: 'GET',
        signal: controller.signal,
        headers: {
          'Accept': 'application/json',
        }
      });

      clearTimeout(timeoutId);

      if (response.ok) {
        const data = await response.json();
        
        // ตรวจสอบว่าเป็น CanSat device
        if (data && (data.gps || data.accelerometer || data.packet !== undefined)) {
          return {
            success: true,
            deviceName: data.deviceName || `CanSat-${ip.split('.').pop()}`,
            data: data
          };
        }
      }
      
      return { success: false, error: 'Not a CanSat device' };
    } catch (error) {
      return { success: false, error: error.message };
    }
  };

  // Fetch data from ESP32 (Simplified - removed smart reconnect)
  const fetchDataFromESP = useCallback(async (connection) => {
    if (!connection.ip.trim()) return;
    
    try {
      // Update connecting status only if not already connecting
      setEspConnections(prev => prev.map(conn => 
        conn.id === connection.id && !conn.isConnecting
          ? { ...conn, isConnecting: true }
          : conn
      ));

      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 5000);

      const response = await fetch(`http://${connection.ip}/data`, {
        method: 'GET',
        headers: {
          'Accept': 'application/json',
        },
        signal: controller.signal
      });

      clearTimeout(timeoutId);

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      
      // Clear connection timeout for this device
      if (connectionTimeouts.current[connection.id]) {
        clearTimeout(connectionTimeouts.current[connection.id]);
      }

      // Set new timeout to detect disconnection
      connectionTimeouts.current[connection.id] = setTimeout(() => {
        console.log(`Connection timeout for ESP32 #${connection.id}`);
        setEspConnections(prev => prev.map(conn => 
          conn.id === connection.id 
            ? { 
                ...conn, 
                isConnected: false, 
                isConnecting: false,
                status: 'timeout',
                lastReceived: conn.lastReceived // Keep last received time
              }
            : conn
        ));
      }, CONNECTION_TIMEOUT);
      
      // Update successful connection and global timestamp
      const currentTime = new Date().toLocaleTimeString('th-TH');
      setGlobalLastReceived(currentTime);
      
      setEspConnections(prev => prev.map(conn => 
        conn.id === connection.id 
          ? { 
              ...conn, 
              isConnected: true, 
              isConnecting: false, 
              lastReceived: currentTime,
              status: 'online',
              autoReconnect: true,
              deviceName: data.deviceName || conn.deviceName || `CanSat-${connection.ip.split('.').pop()}`
            }
          : conn
      ));

      // Update sensor data (use latest successful data)
      setSensorData(prevData => ({
        ...data,
        source_esp_id: connection.id,
        source_esp_ip: connection.ip
      }));

      // Record data if recording is active
      if (isRecording && startTime) {
        const currentTime = ((Date.now() - startTime) / 1000).toFixed(1);
        const realTime = new Date();
        
        const newRecord = {
          timestamp_seconds: currentTime,
          date: realTime.toLocaleDateString('th-TH'),
          time: realTime.toLocaleTimeString('th-TH'),
          datetime_iso: realTime.toISOString(),
          year: realTime.getFullYear(),
          month: realTime.getMonth() + 1,
          day: realTime.getDate(),
          hour: realTime.getHours(),
          minute: realTime.getMinutes(),
          second: realTime.getSeconds(),
          millisecond: realTime.getMilliseconds(),
          packet: data.packet,
          source_esp_id: connection.id,
          source_esp_ip: connection.ip,
          gps_valid: data.gps.valid,
          latitude: parseFloat(data.gps.latitude),
          longitude: parseFloat(data.gps.longitude),
          gps_altitude: parseFloat(data.gps.altitude),
          satellites: parseInt(data.gps.satellites),
          speed: parseFloat(data.gps.speed),
          accel_x: parseFloat(data.accelerometer.x),
          accel_y: parseFloat(data.accelerometer.y),
          accel_z: parseFloat(data.accelerometer.z),
          gyro_x: parseFloat(data.gyroscope.x),
          gyro_y: parseFloat(data.gyroscope.y),
          gyro_z: parseFloat(data.gyroscope.z),
          temperature: parseFloat(data.bmp280.temperature),
          pressure: parseFloat(data.bmp280.pressure),
          bmp_altitude: parseFloat(data.bmp280.altitude),
          z_direction: data.motion.zDirection,
          total_magnitude: parseFloat(data.motion.totalMagnitude),
          raw_data: data.rawData
        };

        setRecordedData(prev => [...prev, newRecord]);
      }

    } catch (error) {
      console.error(`Error fetching data from ESP32 #${connection.id}:`, error);
      
      // Clear any existing timeout
      if (connectionTimeouts.current[connection.id]) {
        clearTimeout(connectionTimeouts.current[connection.id]);
        delete connectionTimeouts.current[connection.id];
      }
      
      // Update failed connection - no smart reconnect
      setEspConnections(prev => prev.map(conn => 
        conn.id === connection.id 
          ? { 
              ...conn, 
              isConnected: false, 
              isConnecting: false,
              status: connection.autoReconnect ? 'offline' : 'offline'
            }
          : conn
      ));
    }
  }, [isRecording, startTime]);

  // Fetch data from all connected ESP32s
  const fetchAllData = useCallback(async () => {
    const shouldFetchConnections = espConnections.filter(conn => 
      conn.ip.trim() && (conn.isConnected || conn.autoReconnect)
    );
    
    const fetchPromises = shouldFetchConnections.map(conn => fetchDataFromESP(conn));
    await Promise.allSettled(fetchPromises);
  }, [espConnections, fetchDataFromESP]);

  // Manual connect to specific ESP32 (Fixed to prevent button flashing)
  const handleConnect = async (connectionId) => {
    const connection = espConnections.find(conn => conn.id === connectionId);
    if (!connection || !connection.ip.trim() || connection.isConnecting) return;
    
    // Clear any existing timeout
    if (connectionTimeouts.current[connectionId]) {
      clearTimeout(connectionTimeouts.current[connectionId]);
      delete connectionTimeouts.current[connectionId];
    }
    
    setEspConnections(prev => prev.map(conn => 
      conn.id === connectionId 
        ? { 
            ...conn, 
            autoReconnect: true,
            status: 'connecting',
            isConnecting: true
          }
        : conn
    ));

    await fetchDataFromESP(connection);
  };

  // Disconnect specific ESP32 (Enhanced with proper cleanup)
  const handleDisconnect = (connectionId) => {
    // Clear connection timeout
    if (connectionTimeouts.current[connectionId]) {
      clearTimeout(connectionTimeouts.current[connectionId]);
      delete connectionTimeouts.current[connectionId];
    }
    
    setEspConnections(prev => prev.map(conn => 
      conn.id === connectionId 
        ? { 
            ...conn, 
            isConnected: false, 
            isConnecting: false, 
            status: 'manually_disconnected',
            autoReconnect: false
          }
        : conn
    ));
  };

  // Connect to discovered device
  const connectToDiscovered = (device) => {
    const availableSlot = espConnections.find(conn => !conn.isConnected) || espConnections[0];
    
    const updatedConnections = espConnections.map(conn => 
      conn.id === availableSlot.id 
        ? { ...conn, ip: device.ip, deviceName: device.deviceName }
        : conn
    );
    
    setEspConnections(updatedConnections);
    saveIPsToStorage(updatedConnections);
    handleConnect(availableSlot.id);
  };

  // Save IPs to localStorage
  const saveIPsToStorage = (connections) => {
    try {
      const ipsToSave = {
        esp1: connections.find(conn => conn.id === 1)?.ip || '192.168.1.100',
        esp2: connections.find(conn => conn.id === 2)?.ip || '192.168.1.101'
      };
      localStorage.setItem('cansat_esp32_ips', JSON.stringify(ipsToSave));
    } catch (error) {
      console.error('Failed to save IPs:', error);
    }
  };

  // Update ESP32 IP and save to storage
  const updateESPConnection = (id, newIP) => {
    const updatedConnections = espConnections.map(conn => 
      conn.id === id ? { ...conn, ip: newIP } : conn
    );
    setEspConnections(updatedConnections);
    saveIPsToStorage(updatedConnections);
  };

  // Cleanup timeouts on unmount
  useEffect(() => {
    return () => {
      Object.values(connectionTimeouts.current).forEach(timeoutId => {
        clearTimeout(timeoutId);
      });
      connectionTimeouts.current = {};
    };
  }, []);

  // Auto-fetch data every 2 seconds with better error handling
  useEffect(() => {
    const shouldAutoFetch = espConnections.some(conn => 
      conn.ip.trim() && (conn.isConnected || conn.autoReconnect)
    );
    
    if (!shouldAutoFetch) return;
    
    const interval = setInterval(fetchAllData, 2000);
    return () => clearInterval(interval);
  }, [fetchAllData, espConnections]);

  // Recording functions
  const toggleRecording = () => {
    if (!isRecording) {
      setIsRecording(true);
      setRecordedData([]);
      setStartTime(Date.now());
    } else {
      setIsRecording(false);
      if (recordedData.length > 0) {
        downloadCSV();
      }
    }
  };

  const downloadCSV = () => {
    if (recordedData.length === 0) return;

    const headers = Object.keys(recordedData[0]).join(',');
    const csvContent = recordedData.map(row => 
      Object.values(row).map(value => 
        typeof value === 'string' && value.includes(',') ? `"${value}"` : value
      ).join(',')
    ).join('\n');

    const csv = headers + '\n' + csvContent;
    const blob = new Blob([csv], { type: 'text/csv;charset-utf-8;' });
    const link = document.createElement('a');
    const url = URL.createObjectURL(blob);
    link.setAttribute('href', url);
    
    const now = new Date();
    const filename = `cansat_data_${now.getFullYear()}-${String(now.getMonth()+1).padStart(2,'0')}-${String(now.getDate()).padStart(2,'0')}_${String(now.getHours()).padStart(2,'0')}-${String(now.getMinutes()).padStart(2,'0')}-${String(now.getSeconds()).padStart(2,'0')}.csv`;
    
    link.setAttribute('download', filename);
    link.style.visibility = 'hidden';
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  return (
    <div style={styles.container}>
      {isRecording && (
        <div style={styles.recordingBadge}>
          <div style={{
            width: '6px',
            height: '6px',
            borderRadius: '50%',
            background: 'white',
            animation: 'pulse 1s infinite'
          }}></div>
          Recording • {recordedData.length} points
        </div>
      )}

      {/* Header */}
      <header style={styles.header}>
        <div style={styles.headerContent}>
          <h1 style={styles.title}>
            <Satellite size={28} color="#3b82f6" />
            CanSat Mission Control
          </h1>
          <p style={styles.subtitle}>
            Real-time telemetry monitoring with auto-discovery
          </p>

          {/* Status Bar */}
          <div style={styles.statusBar}>
            {espConnections.map((connection, index) => (
              <div key={connection.id}
                style={{
                  ...styles.statusChip,
                  ...(connection.isConnected ? styles.statusOnline : 
                     connection.isConnecting ? styles.statusConnecting : 
                     connection.status === 'timeout' ? styles.statusTimeout :
                     styles.statusOffline),
                  cursor: 'pointer'
                }}
                onClick={() => {
                  if (connection.isConnecting) return;
                  
                  if (connection.isConnected) {
                    if (window.confirm(`Disconnect from ${connection.deviceName || `ESP32 #${connection.id}`}?`)) {
                      handleDisconnect(connection.id);
                    }
                  } else {
                    setEditingConnection(connection.id);
                    setShowConnectionModal(true);
                  }
                }}
              >
                {connection.isConnected ? <Wifi size={16} /> : 
                 connection.isConnecting ? <Settings size={16} className="animate-spin" /> : 
                 <WifiOff size={16} />}
                {connection.deviceName || `ESP32 #${connection.id}`}: {
                  connection.isConnected ? 'Connected' : 
                  connection.isConnecting ? 'Connecting...' : 
                  connection.status === 'timeout' ? 'Timeout' :
                  connection.status === 'manually_disconnected' ? 'Disconnected' :
                  'Click to Connect'
                }
                {connection.autoReconnect && !connection.isConnected && connection.status !== 'manually_disconnected' && (
                  <span style={{fontSize: '0.7rem', opacity: 0.7}}>
                    {' '}(Auto)
                  </span>
                )}
              </div>
            ))}
            
            <div style={{...styles.statusChip, ...styles.statusNeutral}}>
              <Signal size={16} />
              Packet #{sensorData.packet}
              {sensorData.source_esp_id && (
                <span style={{fontSize: '0.7rem', opacity: 0.7}}>
                  {' '}(ESP#{sensorData.source_esp_id})
                </span>
              )}
            </div>
            
            <div style={{...styles.statusChip, ...styles.statusNeutral}}>
              <Activity size={16} />
              Updated {globalLastReceived}
            </div>

            <div style={{
              ...styles.statusChip,
              ...(sensorData.gps.valid ? styles.statusOnline : styles.statusOffline)
            }}>
              <MapPin size={16} />
              GPS {sensorData.gps.valid ? 'Locked' : 'No Fix'}
            </div>

            {isScanning && (
              <div style={{...styles.statusChip, ...styles.statusScanning}}>
                <Search size={16} className="animate-spin" />
                Scanning... {Math.round(scanProgress)}%
              </div>
            )}

            {lastScanTime && !isScanning && (
              <div style={{...styles.statusChip, ...styles.statusNeutral}}>
                <Search size={16} />
                Last scan: {lastScanTime} ({discoveredDevices.length} found)
              </div>
            )}
          </div>

          {/* Controls */}
          <div style={styles.controls}>
            <button
              onClick={toggleRecording}
              style={{
                ...styles.button,
                ...(isRecording ? styles.buttonDanger : styles.buttonSuccess)
              }}
            >
              {isRecording ? <Square size={16} /> : <Play size={16} />}
              {isRecording ? 'Stop Recording' : 'Start Recording'}
              {recordedData.length > 0 && (
                <span style={{
                  marginLeft: '8px',
                  fontSize: '0.75rem',
                  background: 'rgba(255,255,255,0.2)',
                  padding: '2px 6px',
                  borderRadius: '10px'
                }}>
                  {recordedData.length}
                </span>
              )}
            </button>

            <button
              onClick={() => setShowConnectionModal(true)}
              style={{...styles.button, ...styles.buttonPrimary}}
            >
              <Settings size={16} />
              Configure ESP32s
            </button>

            <button
              onClick={scanForDevices}
              disabled={isScanning}
              style={{
                ...styles.button,
                ...styles.buttonSecondary,
                opacity: isScanning ? 0.5 : 1
              }}
            >
              {isScanning ? (
                <>
                  <Search size={16} className="animate-spin" />
                  Scanning...
                </>
              ) : (
                <>
                  <Search size={16} />
                  Auto-Discover
                </>
              )}
            </button>
          </div>
        </div>
      </header>

      {/* Connection Modal */}
      {showConnectionModal && (
        <>
          <div style={styles.settingsOverlay} onClick={() => setShowConnectionModal(false)}></div>
          <div style={styles.settingsModal}>
            <h3 style={{marginBottom: '12px', fontSize: '1.25rem', fontWeight: '600'}}>
              Configure ESP32 Receivers & Auto-Discovery
            </h3>
            <p style={{marginBottom: '20px', color: '#64748b', fontSize: '0.9rem'}}>
              Setup multiple ESP32 receivers with auto-discovery
            </p>

            {/* Auto-Discovery Section */}
            <div style={{
              background: '#f0f9ff',
              border: '1px solid #0ea5e9',
              borderRadius: '8px',
              padding: '16px',
              marginBottom: '24px'
            }}>
              <div style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                marginBottom: '12px'
              }}>
                <h4 style={{fontSize: '1rem', fontWeight: '600', color: '#0369a1', margin: 0}}>
                  🔍 Auto-Discovery
                </h4>
                <button
                  onClick={scanForDevices}
                  disabled={isScanning}
                  style={{
                    ...styles.button,
                    ...styles.buttonPrimary,
                    padding: '8px 16px',
                    fontSize: '0.8rem'
                  }}
                >
                  {isScanning ? (
                    <>
                      <Search size={14} className="animate-spin" />
                      Scanning...
                    </>
                  ) : (
                    <>
                      <RefreshCw size={14} />
                      Scan Network
                    </>
                  )}
                </button>
              </div>

              {isScanning && (
                <div style={styles.progressBar}>
                  <div style={{...styles.progressFill, width: `${scanProgress}%`}}></div>
                </div>
              )}

              <div style={{fontSize: '0.8rem', color: '#0284c7', marginBottom: '12px'}}>
                Scanning network for CanSat devices... This will check IPs from 192.168.1.100 to 192.168.1.150
              </div>

              {discoveredDevices.length > 0 && (
                <div>
                  <div style={{fontSize: '0.875rem', fontWeight: '500', marginBottom: '8px', color: '#0369a1'}}>
                    Found {discoveredDevices.length} CanSat device(s):
                  </div>
                  {discoveredDevices.map((device, index) => (
                    <div key={index} style={styles.discoveredDevice}>
                      <div>
                        <div style={{fontSize: '0.875rem', fontWeight: '500'}}>
                          {device.deviceName} ({device.ip})
                        </div>
                        <div style={{fontSize: '0.75rem', color: '#6b7280'}}>
                          Packet: {device.lastPacket} • Signal: {device.signal} • Found: {device.discovered}
                        </div>
                      </div>
                      <button
                        onClick={() => connectToDiscovered(device)}
                        style={{
                          ...styles.button,
                          ...styles.buttonSuccess,
                          padding: '6px 12px',
                          fontSize: '0.75rem'
                        }}
                      >
                        Connect
                      </button>
                    </div>
                  ))}
                </div>
              )}

              {!isScanning && discoveredDevices.length === 0 && lastScanTime && (
                <div style={{fontSize: '0.8rem', color: '#64748b', fontStyle: 'italic'}}>
                  No CanSat devices found in the network. Make sure your ESP32s are powered on and connected to the same network.
                </div>
              )}
            </div>

            {/* Manual Configuration */}
            <h4 style={{fontSize: '1rem', fontWeight: '600', marginBottom: '16px', color: '#374151'}}>
              Manual Configuration
            </h4>
            
            {espConnections.map((connection) => (
              <div key={connection.id} style={{marginBottom: '24px'}}>
                <div style={{
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'space-between',
                  marginBottom: '8px'
                }}>
                  <label style={{
                    fontSize: '0.875rem',
                    fontWeight: '500',
                    color: '#374151'
                  }}>
                    ESP32 Receiver #{connection.id} ({connection.deviceName || 'Unnamed'}):
                  </label>
                  <div style={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: '8px',
                    fontSize: '0.75rem'
                  }}>
                    <div style={{
                      width: '8px',
                      height: '8px',
                      borderRadius: '50%',
                      background: connection.isConnected ? '#22c55e' : 
                                 connection.isConnecting ? '#f59e0b' : 
                                 connection.status === 'timeout' ? '#f59e0b' :
                                 '#ef4444'
                    }}></div>
                    {connection.isConnected ? `Online (${connection.lastReceived})` : 
                     connection.isConnecting ? 'Connecting...' : 
                     connection.status === 'timeout' ? 'Connection Timeout' :
                     connection.status === 'manually_disconnected' ? 'Manually Disconnected' :
                     'Offline'}
                    {connection.autoReconnect && !connection.isConnected && connection.status !== 'manually_disconnected' && (
                      <span style={{
                        fontSize: '0.7rem',
                        background: '#dbeafe',
                        color: '#1d4ed8',
                        padding: '2px 6px',
                        borderRadius: '10px'
                      }}>
                        Auto Reconnect
                      </span>
                    )}
                  </div>
                </div>
                
                <div style={{display: 'flex', gap: '8px', alignItems: 'center'}}>
                  <input
                    type="text"
                    value={connection.ip}
                    onChange={(e) => updateESPConnection(connection.id, e.target.value)}
                    placeholder={`192.168.1.${99 + connection.id}`}
                    style={{
                      ...styles.input,
                      marginBottom: '0',
                      flex: 1
                    }}
                  />
                  <button
                    onClick={() => handleConnect(connection.id)}
                    disabled={connection.isConnecting || !connection.ip.trim()}
                    style={{
                      ...styles.button,
                      ...styles.buttonPrimary,
                      marginBottom: '0',
                      opacity: (connection.isConnecting || !connection.ip.trim()) ? 0.5 : 1
                    }}
                  >
                    {connection.isConnecting ? 'Connecting...' : 'Connect'}
                  </button>
                  {connection.isConnected && (
                    <button
                      onClick={() => handleDisconnect(connection.id)}
                      style={{
                        ...styles.button,
                        ...styles.buttonDanger,
                        marginBottom: '0',
                        padding: '10px 16px'
                      }}
                    >
                      Disconnect
                    </button>
                  )}
                </div>
              </div>
            ))}
            
            {/* Information Box */}
            <div style={{
              background: '#f0f9ff',
              border: '1px solid #0ea5e9',
              borderRadius: '8px',
              padding: '12px',
              marginBottom: '20px',
              fontSize: '0.8rem',
              color: '#0369a1'
            }}>
              <div style={{fontWeight: '600', marginBottom: '8px'}}>🚀 Features:</div>
              <div style={{marginBottom: '4px'}}>
                • <strong>Auto-Discovery:</strong> Automatically finds CanSat devices on your network
              </div>
              <div style={{marginBottom: '4px'}}>
                • <strong>Auto Reconnect:</strong> Automatically reconnects if connection is lost
              </div>
              <div style={{marginBottom: '4px'}}>
                • <strong>Auto-Save:</strong> IP addresses are automatically saved and restored
              </div>
              <div>
                • <strong>Multi-Device:</strong> Connect to multiple ESP32s for redundancy
              </div>
            </div>
            
            <div style={{display: 'flex', gap: '12px', justifyContent: 'flex-end'}}>
              <button
                onClick={() => setShowConnectionModal(false)}
                style={{...styles.button, ...styles.buttonSecondary}}
              >
                Close
              </button>
              <button
                onClick={() => {
                  espConnections.forEach(conn => {
                    if (conn.ip.trim() && !conn.isConnected) {
                      handleConnect(conn.id);
                    }
                  });
                }}
                style={{...styles.button, ...styles.buttonSuccess}}
                disabled={espConnections.every(conn => conn.isConnected || conn.isConnecting || !conn.ip.trim())}
              >
                Connect All
              </button>
            </div>
          </div>
        </>
      )}

      {/* Main Content */}
      <main style={styles.main}>
        <div style={styles.grid}>
          
          {/* Real-time GPS Map */}
          <div style={{...styles.card, ...styles.mapContainer}}>
            <div style={styles.cardHeader}>
              <div style={styles.cardTitle}>
                <Map size={20} color="#3b82f6" />
                Real-time GPS Tracking
                {sensorData.gps.valid && (
                  <span style={{
                    fontSize: '0.75rem',
                    background: '#dcfce7',
                    color: '#166534',
                    padding: '4px 8px',
                    borderRadius: '12px',
                    marginLeft: '12px'
                  }}>
                    LIVE
                  </span>
                )}
              </div>
              <div style={styles.cardSubtitle}>
                Interactive map with GPS position and movement history
              </div>
            </div>
            
            <div style={{position: 'relative', height: '400px'}}>
              {/* Map Container */}
              <div 
                ref={mapRef} 
                style={{
                  width: '100%',
                  height: '100%',
                  borderRadius: '0 0 12px 12px'
                }}
              />
              
              {/* Map Controls */}
              <div style={styles.mapControls}>
                <button
                  onClick={toggleSatelliteView}
                  style={{
                    ...styles.mapButton,
                    background: showSatelliteView ? '#3b82f6' : 'white',
                    color: showSatelliteView ? 'white' : '#374151'
                  }}
                >
                  <Satellite size={14} />
                  {showSatelliteView ? 'Street' : 'Satellite'}
                </button>
                
                <button
                  onClick={() => setFollowGPS(!followGPS)}
                  style={{
                    ...styles.mapButton,
                    background: followGPS ? '#10b981' : 'white',
                    color: followGPS ? 'white' : '#374151'
                  }}
                >
                  <Move size={14} />
                  Follow GPS
                </button>
                
                <button
                  onClick={centerOnGPS}
                  disabled={!sensorData.gps.valid}
                  style={{
                    ...styles.mapButton,
                    opacity: sensorData.gps.valid ? 1 : 0.5
                  }}
                >
                  <Crosshair size={14} />
                  Center
                </button>
                
                <button
                  onClick={clearGPSHistory}
                  disabled={gpsHistory.length === 0}
                  style={{
                    ...styles.mapButton,
                    opacity: gpsHistory.length > 0 ? 1 : 0.5
                  }}
                >
                  Clear Path
                </button>
              </div>
              
              {/* Map Stats */}
              <div style={styles.mapStats}>
                <div style={{marginBottom: '4px'}}>
                  <strong>GPS Status:</strong> {sensorData.gps.valid ? '🟢 Active' : '🔴 No Fix'}
                </div>
                <div style={{marginBottom: '4px'}}>
                  <strong>Satellites:</strong> {sensorData.gps.satellites} | <strong>Speed:</strong> {sensorData.gps.speed} km/h
                </div>
                <div style={{marginBottom: '4px'}}>
                  <strong>Position:</strong> {parseFloat(sensorData.gps.latitude).toFixed(6)}, {parseFloat(sensorData.gps.longitude).toFixed(6)}
                </div>
                <div>
                  <strong>Altitude:</strong> {sensorData.gps.altitude}m | <strong>Path Points:</strong> {gpsHistory.length}
                </div>
              </div>
              
              {/* No GPS Overlay */}
              {!sensorData.gps.valid && (
                <div style={{
                  position: 'absolute',
                  top: 0,
                  left: 0,
                  right: 0,
                  bottom: 0,
                  background: 'rgba(248, 250, 252, 0.9)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  zIndex: 600,
                  backdropFilter: 'blur(2px)'
                }}>
                  <div style={{textAlign: 'center', color: '#64748b'}}>
                    <MapPin size={48} style={{marginBottom: '12px', opacity: 0.5}} />
                    <div style={{fontSize: '1.125rem', fontWeight: '500', marginBottom: '4px'}}>
                      Waiting for GPS Signal
                    </div>
                    <div style={{fontSize: '0.875rem'}}>
                      Map will show real-time position when GPS is locked
                    </div>
                  </div>
                </div>
              )}
            </div>
          </div>

          {/* GPS Card */}
          <div style={styles.card}>
            <div style={styles.cardHeader}>
              <div style={styles.cardTitle}>
                <Navigation size={20} color="#3b82f6" />
                GPS Navigation
              </div>
              <div style={styles.cardSubtitle}>
                Global positioning and location data
              </div>
            </div>
            <div style={styles.cardBody}>
              <div style={styles.dataGrid}>
                <div style={styles.dataItem}>
                  <div style={{
                    ...styles.dataValue,
                    ...(sensorData.gps.valid ? styles.statusGood : styles.statusBad)
                  }}>
                    {sensorData.gps.valid ? 'LOCKED' : 'NO FIX'}
                  </div>
                  <div style={styles.dataLabel}>Status</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.gps.latitude).toFixed(4)}
                  </div>
                  <div style={styles.dataLabel}>Latitude</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.gps.longitude).toFixed(4)}
                  </div>
                  <div style={styles.dataLabel}>Longitude</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {sensorData.gps.altitude}
                  </div>
                  <div style={styles.dataLabel}>Altitude (m)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {sensorData.gps.satellites}
                  </div>
                  <div style={styles.dataLabel}>Satellites</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {sensorData.gps.speed}
                  </div>
                  <div style={styles.dataLabel}>Speed (km/h)</div>
                </div>
              </div>
              
              {sensorData.gps.valid && parseFloat(sensorData.gps.latitude) !== 0 && (
                <div style={{display: 'flex', gap: '8px', marginTop: '12px'}}>
                  <a
                    href={`https://www.google.com/maps?q=${sensorData.gps.latitude},${sensorData.gps.longitude}`}
                    target="_blank"
                    rel="noopener noreferrer"
                    style={{
                      display: 'inline-flex',
                      alignItems: 'center',
                      gap: '8px',
                      padding: '8px 16px',
                      background: '#3b82f6',
                      textDecoration: 'none',
                      borderRadius: '6px',
                      fontSize: '0.875rem',
                      fontWeight: '500',
                      transition: 'background-color 0.2s ease'
                    }}
                  >
                    <Map size={16} />
                    Google Maps
                  </a>
                  
                  <button
                    onClick={centerOnGPS}
                    style={{
                      display: 'inline-flex',
                      alignItems: 'center',
                      gap: '8px',
                      padding: '8px 16px',
                      background: '#10b981',
                      color: 'white',
                      border: 'none',
                      borderRadius: '6px',
                      fontSize: '0.875rem',
                      fontWeight: '500',
                      cursor: 'pointer',
                      transition: 'background-color 0.2s ease'
                    }}
                  >
                    <Crosshair size={16} />
                    Center on Map
                  </button>
                </div>
              )}
            </div>
          </div>

          {/* Accelerometer Card */}
          <div style={styles.card}>
            <div style={styles.cardHeader}>
              <div style={styles.cardTitle}>
                <Activity size={20} color="#10b981" />
                Accelerometer
              </div>
              <div style={styles.cardSubtitle}>
                Linear acceleration measurement
              </div>
            </div>
            <div style={styles.cardBody}>
              <div style={styles.dataGrid}>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.accelerometer.x).toFixed(2)}
                  </div>
                  <div style={styles.dataLabel}>X-axis (m/s²)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.accelerometer.y).toFixed(2)}
                  </div>
                  <div style={styles.dataLabel}>Y-axis (m/s²)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.accelerometer.z).toFixed(2)}
                  </div>
                  <div style={styles.dataLabel}>Z-axis (m/s²)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {sensorData.motion.zDirection}
                  </div>
                  <div style={styles.dataLabel}>Direction</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.motion.totalMagnitude).toFixed(2)}
                  </div>
                  <div style={styles.dataLabel}>Magnitude</div>
                </div>
              </div>
            </div>
          </div>

          {/* Gyroscope Card */}
          <div style={styles.card}>
            <div style={styles.cardHeader}>
              <div style={styles.cardTitle}>
                <Navigation size={20} color="#8b5cf6" />
                Gyroscope
              </div>
              <div style={styles.cardSubtitle}>
                Angular velocity measurement
              </div>
            </div>
            <div style={styles.cardBody}>
              <div style={styles.dataGrid}>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.gyroscope.x).toFixed(1)}
                  </div>
                  <div style={styles.dataLabel}>X-axis (°/s)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.gyroscope.y).toFixed(1)}
                  </div>
                  <div style={styles.dataLabel}>Y-axis (°/s)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {parseFloat(sensorData.gyroscope.z).toFixed(1)}
                  </div>
                  <div style={styles.dataLabel}>Z-axis (°/s)</div>
                </div>
              </div>
            </div>
          </div>

          {/* Environmental Card */}
          <div style={styles.card}>
            <div style={styles.cardHeader}>
              <div style={styles.cardTitle}>
                <Thermometer size={20} color="#f59e0b" />
                Environmental
              </div>
              <div style={styles.cardSubtitle}>
                Atmospheric conditions
              </div>
            </div>
            <div style={styles.cardBody}>
              <div style={styles.dataGrid}>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {sensorData.bmp280.temperature}
                  </div>
                  <div style={styles.dataLabel}>Temperature (°C)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {sensorData.bmp280.pressure}
                  </div>
                  <div style={styles.dataLabel}>Pressure (hPa)</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>
                    {sensorData.bmp280.altitude}
                  </div>
                  <div style={styles.dataLabel}>Altitude (m)</div>
                </div>
              </div>
            </div>
          </div>

          {/* System Information Card */}
          <div style={{...styles.card, ...styles.terminalCard}}>
            <div style={styles.cardHeader}>
              <div style={styles.cardTitle}>
                <Activity size={20} color="#64748b" />
                System Telemetry
              </div>
              <div style={styles.cardSubtitle}>
                Raw data stream and connection information
              </div>
            </div>
            <div style={styles.cardBody}>
              <div style={styles.dataGrid}>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>{sensorData.packet}</div>
                  <div style={styles.dataLabel}>Packet ID</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>{sensorData.counter}</div>
                  <div style={styles.dataLabel}>Counter</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>{recordedData.length}</div>
                  <div style={styles.dataLabel}>Recorded</div>
                </div>
                <div style={styles.dataItem}>
                  <div style={styles.dataValue}>{globalLastReceived}</div>
                  <div style={styles.dataLabel}>Last Update</div>
                </div>
              </div>
              
              {/* Connection Status */}
              <div style={{
                background: '#f8fafc',
                border: '1px solid #e2e8f0',
                borderRadius: '8px',
                padding: '12px',
                marginBottom: '16px'
              }}>
                <div style={{fontSize: '0.875rem', fontWeight: '500', marginBottom: '8px', color: '#374151'}}>
                  📡 Active Connections
                </div>
                {espConnections.map(conn => (
                  <div key={conn.id} style={{
                    fontSize: '0.75rem',
                    color: '#6b7280',
                    marginBottom: '4px',
                    display: 'flex',
                    justifyContent: 'space-between'
                  }}>
                    <span>{conn.deviceName || `ESP32 #${conn.id}`} ({conn.ip})</span>
                    <span style={{
                      color: conn.isConnected ? '#059669' : '#dc2626'
                    }}>
                      {conn.isConnected ? '✅ Online' : conn.autoReconnect && conn.status !== 'manually_disconnected' ? '🔄 Auto-reconnecting' : '❌ Offline'}
                    </span>
                  </div>
                ))}
              </div>
              
              <div style={styles.terminal}>
                <div style={{marginBottom: '8px', color: '#94a3b8'}}>Raw LoRa Data:</div>
                <div>{sensorData.rawData}</div>
                {sensorData.source_esp_id && (
                  <div style={{marginTop: '8px', color: '#94a3b8', fontSize: '0.8rem'}}>
                    Source: {espConnections.find(c => c.id === sensorData.source_esp_id)?.deviceName || `ESP32 #${sensorData.source_esp_id}`} ({sensorData.source_esp_ip})
                  </div>
                )}
              </div>
            </div>
          </div>

        </div>
      </main>

      <style>{`
        @keyframes pulse {
          0%, 100% { opacity: 1; }
          50% { opacity: 0.5; }
        }
        
        @keyframes spin {
          from { transform: rotate(0deg); }
          to { transform: rotate(360deg); }
        }
        
        .cansat-marker {
          position: relative;
          width: 20px;
          height: 20px;
        }
        
        .cansat-marker-dot {
          width: 20px;
          height: 20px;
          background: #ef4444;
          border: 3px solid white;
          border-radius: 50%;
          box-shadow: 0 2px 4px rgba(0,0,0,0.3);
          position: relative;
          z-index: 2;
        }
        
        .cansat-marker-pulse {
          position: absolute;
          top: -8px;
          left: -8px;
          width: 36px;
          height: 36px;
          background: rgba(239, 68, 68, 0.3);
          border-radius: 50%;
          animation: ping 2s cubic-bezier(0, 0, 0.2, 1) infinite;
          z-index: 1;
        }
        
        @keyframes ping {
          75%, 100% {
            transform: scale(2);
            opacity: 0;
          }
        }
        
        .custom-cansat-marker {
          background: transparent !important;
          border: none !important;
        }
        
        .animate-spin {
          animation: spin 1s linear infinite;
        }
        
        button:hover {
          transform: translateY(-1px);
          box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
        }
        
        .card:hover {
          box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
        }
        
        input:focus {
          border-color: #3b82f6 !important;
          box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
        }
      `}</style>
    </div>
  );
};

export default CanSatDashboard;
