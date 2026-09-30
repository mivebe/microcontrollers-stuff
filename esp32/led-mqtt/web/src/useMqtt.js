import { useEffect, useRef, useState } from 'react'
import mqtt from 'mqtt'

// Connects to the broker and tracks the device's retained status/state topics.
// Topics match ../../src/main.cpp: <prefix>/set, <prefix>/state, <prefix>/status
export function useMqtt(settings) {
  const clientRef = useRef(null)
  const [connection, setConnection] = useState('disconnected')
  const [error, setError] = useState(null)
  const [deviceStatus, setDeviceStatus] = useState('unknown')
  const [state, setState] = useState(null)
  const [lastSeen, setLastSeen] = useState(null)

  useEffect(() => {
    if (!settings) return
    const { url, username, password, prefix } = settings

    setConnection('connecting')
    setError(null)
    const client = mqtt.connect(url, {
      username,
      password,
      clientId: 'web-' + Math.random().toString(16).slice(2, 10),
      reconnectPeriod: 3000,
      connectTimeout: 10000,
    })
    clientRef.current = client

    client.on('connect', () => {
      setConnection('connected')
      setError(null)
      client.subscribe([`${prefix}/state`, `${prefix}/status`], { qos: 1 })
    })
    client.on('reconnect', () => setConnection('connecting'))
    client.on('close', () => setConnection('disconnected'))
    client.on('error', (e) => setError(e.message))
    client.on('message', (topic, payload) => {
      const text = payload.toString()
      if (topic.endsWith('/status')) setDeviceStatus(text)
      if (topic.endsWith('/state')) {
        try {
          setState(JSON.parse(text))
          setLastSeen(new Date())
        } catch {
          // ignore malformed payloads
        }
      }
    })

    return () => client.end(true)
  }, [settings])

  function send(command) {
    clientRef.current?.publish(`${settings.prefix}/set`, command, { qos: 1 })
  }

  return { connection, error, deviceStatus, state, lastSeen, send }
}
