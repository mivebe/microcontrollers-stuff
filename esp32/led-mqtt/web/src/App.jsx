import { useState } from 'react'
import { useMqtt } from './useMqtt.js'

const STORAGE_KEY = 'led-mqtt-settings'

const DEFAULTS = {
  url: 'wss://5a476a1b4c814901b54584ec2ba638c7.s1.eu.hivemq.cloud:8884/mqtt',
  username: '',
  password: '',
  prefix: 'mivebe/esp32-1',
}

function loadSettings() {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY))
    return saved?.username ? saved : null
  } catch {
    return null
  }
}

export default function App() {
  const [settings, setSettings] = useState(loadSettings)
  const [editing, setEditing] = useState(!settings)

  function save(next) {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(next))
    } catch {
      // private mode etc. - settings just won't persist
    }
    setSettings(next)
    setEditing(false)
  }

  return (
    <main>
      <h1>ESP32 LED</h1>
      {editing ? (
        <SettingsForm
          initial={settings ?? DEFAULTS}
          onSave={save}
          onCancel={settings ? () => setEditing(false) : null}
        />
      ) : (
        <Controller settings={settings} onEdit={() => setEditing(true)} />
      )}
    </main>
  )
}

function Controller({ settings, onEdit }) {
  const { connection, error, deviceStatus, state, lastSeen, send } = useMqtt(settings)
  const online = deviceStatus === 'online'
  const on = state?.on

  return (
    <>
      <p className="badges">
        <span className={`badge ${connection === 'connected' ? 'ok' : 'warn'}`}>broker: {connection}</span>
        <span className={`badge ${online ? 'ok' : 'bad'}`}>device: {deviceStatus}</span>
      </p>

      <button
        className={`bulb ${on ? 'lit' : ''}`}
        onClick={() => send('toggle')}
        disabled={connection !== 'connected' || !online}
        aria-label="Toggle LED"
      >
        {on === undefined ? '?' : on ? 'ON' : 'OFF'}
      </button>

      {state && (
        <p className="meta">
          uptime {formatUptime(state.uptime)} · WiFi {state.rssi} dBm · heap {Math.round(state.heap / 1024)} KB
          {lastSeen && (
            <>
              <br />
              last update {lastSeen.toLocaleTimeString()}
            </>
          )}
        </p>
      )}
      {error && <p className="error">{error}</p>}

      <button className="link" onClick={onEdit}>
        Connection settings
      </button>
    </>
  )
}

function SettingsForm({ initial, onSave, onCancel }) {
  const [form, setForm] = useState(initial)
  const field = (key, label, type = 'text') => (
    <label>
      {label}
      <input
        type={type}
        value={form[key]}
        onChange={(e) => setForm({ ...form, [key]: e.target.value })}
        autoComplete="off"
        required
      />
    </label>
  )

  return (
    <form
      className="settings"
      onSubmit={(e) => {
        e.preventDefault()
        onSave(form)
      }}
    >
      <p className="hint">Use a HiveMQ credential made for this web app, not the device one.</p>
      {field('url', 'Broker WebSocket URL')}
      {field('username', 'Username')}
      {field('password', 'Password', 'password')}
      {field('prefix', 'Topic prefix')}
      <div className="row">
        <button type="submit" className="primary">
          Connect
        </button>
        {onCancel && (
          <button type="button" className="link" onClick={onCancel}>
            Cancel
          </button>
        )}
      </div>
    </form>
  )
}

function formatUptime(s) {
  if (s == null) return '?'
  const d = Math.floor(s / 86400)
  const h = Math.floor((s % 86400) / 3600)
  const m = Math.floor((s % 3600) / 60)
  return d ? `${d}d ${h}h` : h ? `${h}h ${m}m` : `${m}m ${s % 60}s`
}
