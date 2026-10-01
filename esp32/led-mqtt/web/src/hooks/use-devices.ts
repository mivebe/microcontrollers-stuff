import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import mqtt, { type MqttClient } from 'mqtt'
import { toast } from 'sonner'
import {
  type Command,
  type ConnectionSettings,
  type Device,
  type DeviceState,
  type LogEntry,
  type OtaProgress,
  type RawLog,
  type RawOta,
  compareVersions,
  deviceLabel,
} from '@/lib/device'

export type BrokerStatus = 'idle' | 'connecting' | 'connected' | 'disconnected'

const MAX_LOGS = 1000
const COMMAND_TIMEOUT_MS = 5000
const OTA_START_TIMEOUT_MS = 30000 // device must start downloading within this
const OTA_REBOOT_TIMEOUT_MS = 3 * 60 * 1000 // and be back online after rebooting within this
const SELECTED_KEY = 'led-mqtt-selected'

/** "Update all" progress: boards still to do, and the one updating now */
export interface UpdateQueue {
  url: string
  version: string
  todo: string[]
  current: string | null
  done: number
  total: number
}

/**
 * Connects to the MQTT broker, discovers every board under <base>/+/ and tracks their state and
 * updates. Logs are only subscribed for the selected board.
 * Topics per board: <base>/<id>/state, /status, /ota, /log (subscribed), <base>/<id>/cmd (published).
 */
export function useDevices(settings: ConnectionSettings | null) {
  const clientRef = useRef<MqttClient | null>(null)
  const pendingTimer = useRef<ReturnType<typeof setTimeout> | null>(null)
  const otaTimers = useRef(new Map<string, ReturnType<typeof setTimeout>>())
  const devicesRef = useRef<Record<string, Device>>({}) // mirrors `devices` for the message handler
  const selectedRef = useRef<string | null>(null)

  const [broker, setBroker] = useState<BrokerStatus>('idle')
  const [error, setError] = useState<string | null>(null)
  const [devices, setDevices] = useState<Record<string, Device>>({})
  const [selectedId, setSelectedId] = useState<string | null>(() => readSelected())
  const [logs, setLogs] = useState<LogEntry[]>([])
  const [pending, setPending] = useState(false)
  const [queue, setQueue] = useState<UpdateQueue | null>(null)

  selectedRef.current = selectedId

  const patchDevice = useCallback((id: string, patch: Partial<Device>) => {
    const cur = devicesRef.current[id] ?? { id, status: 'unknown', state: null, stateAt: null, ota: null }
    devicesRef.current = { ...devicesRef.current, [id]: { ...cur, ...patch } }
    setDevices(devicesRef.current)
  }, [])

  const removeDevice = useCallback((id: string) => {
    const { [id]: _removed, ...rest } = devicesRef.current
    devicesRef.current = rest
    setDevices(rest)
    clearTimeout(otaTimers.current.get(id))
  }, [])

  const setOta = useCallback(
    function setOtaFor(id: string, ota: OtaProgress | null, timeoutMs?: number) {
      patchDevice(id, { ota })
      clearTimeout(otaTimers.current.get(id))
      if (ota && timeoutMs) {
        otaTimers.current.set(
          id,
          setTimeout(() => {
            const d = devicesRef.current[id]
            if (!d?.ota) return
            const error =
              d.ota.state === 'starting' ? 'The device did not start the update' : 'The device did not come back online'
            setOtaFor(id, { ...d.ota, state: 'failed', error })
            toast.error(`${deviceLabel(d)}: update failed: ${error}`)
          }, timeoutMs),
        )
      }
    },
    [patchDevice],
  )

  // Broker connection + discovery
  useEffect(() => {
    if (!settings) return
    const { url, username, password, base } = settings

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
      client.subscribe([`${base}/+/state`, `${base}/+/status`, `${base}/+/ota`], { qos: 1 })
    })
    client.on('reconnect', () => setBroker('connecting'))
    client.on('close', () => setBroker('disconnected'))
    client.on('error', (e) => setError(e.message))

    client.on('message', (topic, payload) => {
      const parts = topic.slice(base.length + 1).split('/')
      if (!topic.startsWith(base + '/') || parts.length !== 2) return
      const [id, kind] = parts
      const text = payload.toString()

      if (kind === 'status' || kind === 'state') {
        // An empty retained message means the board was forgotten
        if (!text) return removeDevice(id)
      }

      if (kind === 'status') {
        patchDevice(id, { status: text === 'online' ? 'online' : text === 'offline' ? 'offline' : 'unknown' })
      } else if (kind === 'state') {
        try {
          const s = JSON.parse(text) as DeviceState
          patchDevice(id, { state: s, stateAt: Date.now() })
          if (id === selectedRef.current) {
            setPending(false)
            if (pendingTimer.current) clearTimeout(pendingTimer.current)
          }

          // Fresh boot after an update: did the new firmware stick?
          const d = devicesRef.current[id]
          const ota = d.ota
          if (ota?.state === 'rebooting' && s.uptime < 300) {
            if (s.fw === ota.target) {
              setOta(id, { ...ota, state: 'done' })
              toast.success(`${deviceLabel(d)}: updated to firmware v${s.fw}`)
            } else {
              const error = `The new firmware did not start; rolled back to v${s.fw}`
              setOta(id, { ...ota, state: 'failed', error })
              toast.error(`${deviceLabel(d)}: ${error}`)
            }
          }
        } catch {
          // ignore malformed state
        }
      } else if (kind === 'log') {
        if (id !== selectedRef.current) return
        try {
          const raw = JSON.parse(text) as RawLog
          const s = devicesRef.current[id]?.state
          const bootStart = s && s.time > 0 ? (s.time - s.uptime) * 1000 : null
          const entry = toEntry(raw, bootStart)
          setLogs((prev) => mergeLog(prev, entry))
        } catch {
          // ignore malformed log line
        }
      } else if (kind === 'ota') {
        try {
          const raw = JSON.parse(text) as RawOta
          const d = devicesRef.current[id]
          const target = d?.ota?.target ?? '?' // an update started from another browser
          if (raw.state === 'failed') {
            setOta(id, { target, state: 'failed', error: raw.error })
            toast.error(`${d ? deviceLabel(d) : id}: update failed: ${raw.error ?? 'unknown error'}`)
          } else {
            // Watchdog: each progress message restarts the timer
            const timeout = raw.state === 'rebooting' ? OTA_REBOOT_TIMEOUT_MS : OTA_START_TIMEOUT_MS
            setOta(id, { target, state: raw.state, progress: raw.progress }, timeout)
          }
        } catch {
          // ignore malformed progress
        }
      }
    })

    const timers = otaTimers.current
    return () => {
      client.end(true)
      clientRef.current = null
      setBroker('idle')
      devicesRef.current = {}
      setDevices({})
      setLogs([])
      setQueue(null)
      timers.forEach(clearTimeout)
      timers.clear()
    }
  }, [settings, patchDevice, removeDevice, setOta])

  // Pick a board when none is selected yet (first visit, or the selected one was forgotten)
  useEffect(() => {
    if (selectedId) return
    const first = sortDevices(Object.values(devices)).find((d) => d.status === 'online') ?? sortDevices(Object.values(devices))[0]
    if (first) setSelectedId(first.id)
  }, [devices, selectedId])

  // Logs of the selected board
  useEffect(() => {
    writeSelected(selectedId)
    setLogs([])
    setPending(false)
    const client = clientRef.current
    if (!client || !settings || !selectedId || broker !== 'connected') return
    const topic = `${settings.base}/${selectedId}/log`
    client.subscribe(topic, { qos: 1 }, () => {
      // Ask the board to replay its recent log history
      client.publish(`${settings.base}/${selectedId}/cmd`, JSON.stringify({ action: 'logs' }), { qos: 1 })
    })
    return () => {
      client.unsubscribe(topic)
    }
  }, [selectedId, broker, settings])

  const sendTo = useCallback(
    (id: string, cmd: Command) => {
      const client = clientRef.current
      if (!settings || !client?.connected) {
        toast.error('Not connected to the broker')
        return false
      }
      client.publish(`${settings.base}/${id}/cmd`, JSON.stringify(cmd), { qos: 1 }, (err) => {
        if (err) toast.error(`Send failed: ${err.message}`)
      })
      return true
    },
    [settings],
  )

  /** Sends a command to the selected board */
  const send = useCallback(
    (cmd: Command) => {
      if (!selectedId || !sendTo(selectedId, cmd)) return
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
    [selectedId, sendTo],
  )

  /** Asks a board to download and install the firmware at `url` */
  const update = useCallback(
    (id: string, url: string, version: string) => {
      if (sendTo(id, { action: 'update', url })) setOta(id, { state: 'starting', target: version }, OTA_START_TIMEOUT_MS)
    },
    [sendTo, setOta],
  )

  /** Updates every online board that runs an older version, one at a time */
  const updateAll = useCallback((url: string, version: string) => {
    const todo = sortDevices(Object.values(devicesRef.current))
      .filter((d) => d.status === 'online' && d.state && compareVersions(version, d.state.fw) > 0)
      .map((d) => d.id)
    if (todo.length) setQueue({ url, version, todo, current: null, done: 0, total: todo.length })
  }, [])

  const cancelUpdateAll = useCallback(() => setQueue(null), [])

  // Drives the "update all" queue
  useEffect(() => {
    if (!queue) return
    if (queue.current) {
      const ota = devices[queue.current]?.ota
      if (ota?.state === 'done') {
        setQueue({ ...queue, current: null, done: queue.done + 1 })
      } else if (ota?.state === 'failed') {
        toast.error('Update all stopped because an update failed')
        setQueue(null)
      }
      return
    }
    const [next, ...todo] = queue.todo
    if (!next) {
      toast.success(`Update all finished: ${queue.done} board${queue.done === 1 ? '' : 's'} updated`)
      setQueue(null)
    } else if (devices[next]?.status !== 'online') {
      setQueue({ ...queue, todo }) // went offline meanwhile: skip
    } else {
      update(next, queue.url, queue.version)
      setQueue({ ...queue, todo, current: next })
    }
  }, [queue, devices, update])

  /** Removes a board from the list by clearing its retained messages; it comes back if it reconnects */
  const forget = useCallback(
    (id: string) => {
      const client = clientRef.current
      if (!settings || !client?.connected) return
      for (const kind of ['state', 'status']) client.publish(`${settings.base}/${id}/${kind}`, '', { qos: 1, retain: true })
      removeDevice(id)
      if (selectedRef.current === id) setSelectedId(null)
    },
    [settings, removeDevice],
  )

  const dismissOta = useCallback((id: string) => setOta(id, null), [setOta])
  const clearLogs = useCallback(() => setLogs([]), [])

  const list = useMemo(() => sortDevices(Object.values(devices)), [devices])
  const selected = selectedId ? (devices[selectedId] ?? null) : null

  return {
    broker,
    error,
    devices: list,
    selected,
    selectedId,
    select: setSelectedId,
    logs,
    pending,
    send,
    update,
    updateAll,
    cancelUpdateAll,
    queue,
    forget,
    dismissOta,
    clearLogs,
  }
}

/** Online first, then by name */
function sortDevices(list: Device[]) {
  return [...list].sort(
    (a, b) =>
      Number(b.status === 'online') - Number(a.status === 'online') ||
      deviceLabel(a).localeCompare(deviceLabel(b)),
  )
}

function readSelected() {
  try {
    return localStorage.getItem(SELECTED_KEY)
  } catch {
    return null
  }
}

function writeSelected(id: string | null) {
  try {
    if (id) localStorage.setItem(SELECTED_KEY, id)
    else localStorage.removeItem(SELECTED_KEY)
  } catch {
    // private mode etc. - the selection just won't persist
  }
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
