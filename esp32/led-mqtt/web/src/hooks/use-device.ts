import { useCallback, useEffect, useRef, useState } from 'react'
import mqtt, { type MqttClient } from 'mqtt'
import { toast } from 'sonner'
import type { Command, ConnectionSettings, DeviceState, LogEntry, OtaProgress, RawLog, RawOta } from '@/lib/device'

export type BrokerStatus = 'idle' | 'connecting' | 'connected' | 'disconnected'
export type DeviceStatus = 'online' | 'offline' | 'unknown'

const MAX_LOGS = 1000
const COMMAND_TIMEOUT_MS = 5000
const OTA_START_TIMEOUT_MS = 30000 // device must start downloading within this
const OTA_REBOOT_TIMEOUT_MS = 3 * 60 * 1000 // and be back online after rebooting within this

/**
 * Connects to the MQTT broker and exposes the device's state, logs and a command sender.
 * Topics: <prefix>/state, /status, /log, /ota (subscribed) and <prefix>/cmd (published).
 */
export function useDevice(settings: ConnectionSettings | null) {
  const clientRef = useRef<MqttClient | null>(null)
  const bootStartRef = useRef<number | null>(null) // device boot time in epoch ms
  const pendingTimer = useRef<ReturnType<typeof setTimeout> | null>(null)
  const otaTimer = useRef<ReturnType<typeof setTimeout> | null>(null)
  const otaRef = useRef<OtaProgress | null>(null) // mirrors `ota` for the message handler

  const [broker, setBroker] = useState<BrokerStatus>('idle')
  const [error, setError] = useState<string | null>(null)
  const [status, setStatus] = useState<DeviceStatus>('unknown')
  const [state, setState] = useState<DeviceState | null>(null)
  const [stateAt, setStateAt] = useState<number | null>(null)
  const [logs, setLogs] = useState<LogEntry[]>([])
  const [pending, setPending] = useState(false)
  const [ota, setOtaState] = useState<OtaProgress | null>(null)

  const setOta = useCallback((next: OtaProgress | null, timeoutMs?: number) => {
    otaRef.current = next
    setOtaState(next)
    if (otaTimer.current) clearTimeout(otaTimer.current)
    if (next && timeoutMs) {
      otaTimer.current = setTimeout(() => {
        const cur = otaRef.current
        if (!cur) return
        const error = cur.state === 'starting' ? 'The device did not start the update' : 'The device did not come back online'
        setOta({ ...cur, state: 'failed', error })
        toast.error(`Update failed: ${error}`)
      }, timeoutMs)
    }
  }, [])

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
      client.subscribe([`${prefix}/state`, `${prefix}/status`, `${prefix}/log`, `${prefix}/ota`], { qos: 1 }, () => {
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

          // Fresh boot after an update: did the new firmware stick?
          const cur = otaRef.current
          if (cur?.state === 'rebooting' && s.uptime < 300) {
            if (s.fw === cur.target) {
              setOta({ ...cur, state: 'done' })
              toast.success(`Updated to firmware v${s.fw}`)
            } else {
              const error = `The new firmware did not start; rolled back to v${s.fw}`
              setOta({ ...cur, state: 'failed', error })
              toast.error(error)
            }
          }
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
      } else if (kind === 'ota') {
        try {
          const raw = JSON.parse(text) as RawOta
          const cur = otaRef.current
          const target = cur?.target ?? '?' // an update started from another browser
          if (raw.state === 'failed') {
            setOta({ target, state: 'failed', error: raw.error })
            toast.error(`Update failed: ${raw.error ?? 'unknown error'}`)
          } else {
            // Watchdog: each progress message restarts the timer
            const timeout = raw.state === 'rebooting' ? OTA_REBOOT_TIMEOUT_MS : OTA_START_TIMEOUT_MS
            setOta({ target, state: raw.state, progress: raw.progress }, timeout)
          }
        } catch {
          // ignore malformed progress
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
      setOta(null)
    }
  }, [settings, setOta])

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

  /** Asks the device to download and install the firmware at `url` */
  const update = useCallback(
    (url: string, version: string) => {
      send({ action: 'update', url })
      setOta({ state: 'starting', target: version }, OTA_START_TIMEOUT_MS)
    },
    [send, setOta],
  )
  const dismissOta = useCallback(() => setOta(null), [setOta])

  return { broker, error, status, state, stateAt, logs, pending, ota, send, update, dismissOta, clearLogs }
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
