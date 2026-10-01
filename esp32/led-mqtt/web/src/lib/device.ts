// Types and helpers matching the firmware protocol in ../../../src/main.cpp

export type Mode = 'solid' | 'blink' | 'breathe'

/** Retained JSON on <prefix>/state */
export interface DeviceState {
  on: boolean
  brightness: number
  mode: Mode
  period: number
  interval: number
  uptime: number
  rssi: number
  ssid: string
  ip: string
  mac: string
  heap: number
  minHeap: number
  chip: string
  cpuMhz: number
  fw: string
  reset: string
  time: number
}

/** JSON accepted on <prefix>/cmd (any subset) */
export type Command = Partial<Pick<DeviceState, 'on' | 'brightness' | 'mode' | 'period' | 'interval'>> & {
  action?: 'restart' | 'logs' | 'state'
}

export type LogLevel = 'I' | 'W' | 'E'

/** JSON on <prefix>/log */
export interface RawLog {
  seq: number
  ts: number // epoch ms, 0 before the device clock synced
  up: number // ms since device boot
  lvl: LogLevel
  msg: string
  hist?: boolean
}

export interface LogEntry {
  key: string
  seq: number
  /** Wall-clock time in ms, or null if unknown */
  at: number | null
  up: number
  sortKey: number
  lvl: LogLevel
  msg: string
  hist: boolean
}

export interface ConnectionSettings {
  url: string
  username: string
  password: string
  prefix: string
}

export const DEFAULT_SETTINGS: ConnectionSettings = {
  url: 'wss://5a476a1b4c814901b54584ec2ba638c7.s1.eu.hivemq.cloud:8884/mqtt',
  username: '',
  password: '',
  prefix: 'mivebe/esp32-1',
}

export function formatDuration(totalSeconds: number) {
  const s = Math.max(0, Math.floor(totalSeconds))
  const d = Math.floor(s / 86400)
  const h = Math.floor((s % 86400) / 3600)
  const m = Math.floor((s % 3600) / 60)
  if (d) return `${d}d ${h}h`
  if (h) return `${h}h ${m}m`
  if (m) return `${m}m ${s % 60}s`
  return `${s}s`
}

export function formatBytes(bytes: number) {
  return bytes >= 1024 ? `${(bytes / 1024).toFixed(0)} KB` : `${bytes} B`
}

/** 0-4 bars + label from WiFi RSSI */
export function signalQuality(rssi: number) {
  if (rssi >= -55) return { bars: 4, label: 'Excellent' }
  if (rssi >= -67) return { bars: 3, label: 'Good' }
  if (rssi >= -75) return { bars: 2, label: 'Fair' }
  if (rssi >= -85) return { bars: 1, label: 'Weak' }
  return { bars: 0, label: 'Very weak' }
}
