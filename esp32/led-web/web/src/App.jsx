import { useEffect, useState } from 'react'

// Talks to the JSON API in ../src/main.cpp
export default function App() {
  const [status, setStatus] = useState(null)
  const [error, setError] = useState(null)

  async function request(method, body) {
    try {
      const res = await fetch('/api/led', {
        method,
        headers: body ? { 'Content-Type': 'application/json' } : undefined,
        body: body ? JSON.stringify(body) : undefined,
      })
      setStatus(await res.json())
      setError(null)
    } catch (e) {
      setError('Cannot reach the ESP32')
    }
  }

  // Poll so BOOT-button presses show up on the page too
  useEffect(() => {
    request('GET')
    const id = setInterval(() => request('GET'), 1000)
    return () => clearInterval(id)
  }, [])

  const on = status?.on

  return (
    <main>
      <h1>ESP32 LED</h1>
      <button
        className={`bulb ${on ? 'lit' : ''}`}
        onClick={() => request('POST', { on: !on })}
        disabled={!status}
        aria-label="Toggle LED"
      >
        {on ? 'ON' : 'OFF'}
      </button>
      <p className="hint">Tap to toggle — or press BOOT on the board</p>
      {status && <p className="meta">uptime {status.uptime}s · heap {Math.round(status.heap / 1024)} KB</p>}
      {error && <p className="error">{error}</p>}
    </main>
  )
}
