import { useCallback, useEffect, useRef, useState } from 'react'
import mqtt, { type MqttClient } from 'mqtt'
import { toast } from 'sonner'
import type { Command, ConnectionSettings, DeviceState, LogEntry, RawLog } from '@/lib/device'

export type BrokerStatus = 'idle' | 'connecting' | 'connected' | 'disconnected'
export type DeviceStatus = 'online' | 'offline' | 'unknown'

const MAX_LOGS = 1000
const COMMAND_TIMEOUT_MS = 5000

/**
 * Connects to the MQTT broker and exposes the device's state, logs and a command sender.
 * Topics: <prefix>/state, /status, /log (subscribed) and <prefix>/cmd (published).
 */
export function useDevice(settings: ConnectionSettings | null) {
  const clientRef = useRef<MqttClient | null>(null)
  const bootStartRef = useRef<number | null>(null) // device boot time in epoch ms
  const pendingTimer = useRef<ReturnType<typeof setTimeout> | null>(null)

  const [broker, setBroker] = useState<BrokerStatus>('idle')
  const [error, setError] = useState<string | null>(null)
  const [status, setStatus] = useState<DeviceStatus>('unknown')
  const [state, setState] = useState<DeviceState | null>(null)
  const [stateAt, setStateAt] = useState<number | null>(null)
  const [logs, setLogs] = useState<LogEntry[]>([])
  const [pending, setPending] = useState(false)

  useEffect(() => {
    if (!settings) return
    const { url, username, password, prefix } = settings

    setBroker('connecting')
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
      setBroker('connected')
      setError(null)
      client.subscribe([`${prefix}/state`, `${prefix}/status`, `${prefix}/log`], { qos: 1 }, () => {
        // Ask the device to replay its recent log history
        client.publish(`${prefix}/cmd`, JSON.stringify({ action: 'logs' }), { qos: 1 })
      })
    })
    client.on('reconnect', () => setBroker('connecting'))
    client.on('close', () => setBroker('disconnected'))
    client.on('error', (e) => setError(e.message))

    client.on('message', (topic, payload) => {
      const text = payload.toString()
      const kind = topic.slice(prefix.length + 1)

      if (kind === 'status') {
        setStatus(text === 'online' ? 'online' : text === 'offline' ? 'offline' : 'unknown')
      } else if (kind === 'state') {
        try {
          const s = JSON.parse(text) as DeviceState
          if (s.time > 0) bootStartRef.current = (s.time - s.uptime) * 1000
          setState(s)
          setStateAt(Date.now())
          setPending(false)
          if (pendingTimer.current) clearTimeout(pendingTimer.current)
        } catch {
          // ignore malformed state
        }
      } else if (kind === 'log') {
        try {
          const raw = JSON.parse(text) as RawLog
          const entry = toEntry(raw, bootStartRef.current)
          setLogs((prev) => mergeLog(prev, entry))
        } catch {
          // ignore malformed log line
        }
      }
    })

    return () => {
      client.end(true)
      clientRef.current = null
      setBroker('idle')
      setStatus('unknown')
      setState(null)
      setLogs([])
    }
  }, [settings])

  const send = useCallback(
    (cmd: Command) => {
      const client = clientRef.current
      if (!settings || !client?.connected) {
        toast.error('Not connected to the broker')
        return
      }
      client.publish(`${settings.prefix}/cmd`, JSON.stringify(cmd), { qos: 1 }, (err) => {
        if (err) toast.error(`Send failed: ${err.message}`)
      })

      // Settings changes are confirmed by the next state message
      if (!cmd.action || cmd.action === 'state') {
        setPending(true)
        if (pendingTimer.current) clearTimeout(pendingTimer.current)
        pendingTimer.current = setTimeout(() => {
          setPending(false)
          toast.warning('The device did not confirm the change')
        }, COMMAND_TIMEOUT_MS)
      }
    },
    [settings],
  )

  const clearLogs = useCallback(() => setLogs([]), [])

  return { broker, error, status, state, stateAt, logs, pending, send, clearLogs }
}

function toEntry(raw: RawLog, bootStart: number | null): LogEntry {
  // Lines logged before the device clock synced have ts = 0; place them using the boot time
  const at = raw.ts > 0 ? raw.ts : bootStart !== null ? bootStart + raw.up : null
  return {
    key: `${raw.seq}:${raw.up}`,
    seq: raw.seq,
    at,
    up: raw.up,
    sortKey: at ?? Date.now(),
    lvl: raw.lvl,
    msg: raw.msg,
    hist: !!raw.hist,
  }
}

function mergeLog(prev: LogEntry[], entry: LogEntry) {
  if (prev.some((e) => e.key === entry.key)) return prev
  const next = [...prev, entry]
  // History replays arrive after newer live lines; keep chronological order
  if (prev.length && entry.sortKey < prev[prev.length - 1].sortKey) next.sort((a, b) => a.sortKey - b.sortKey)
  return next.length > MAX_LOGS ? next.slice(next.length - MAX_LOGS) : next
}
